#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "cache.h"
#define PENALTY 100

typedef struct verboso {
	int op_index;
	char* id_case;
	int cache_index;
	int cache_tag;
	int cache_line;
	int line_tag;
	int valid_bit;
	int dirty_bit;
	int last_used;
} verboso_t;

typedef struct linea {
	int v;
	int d;
	int tag;
	int n_linea;
	int last_used;
} linea_t;

typedef struct set {
	unsigned cant_lineas;
	linea_t** lineas;
} set_t;

typedef struct cache {
	unsigned C;
	unsigned E;
    unsigned S;
    set_t** sets;
    int op_index;
    int loads;
    int stores;
    int rmiss;
    int wmiss;
    int dirty_rmiss;
    int dirty_wmiss;
    int bytes_read;
    int bytes_written;
    int write_time;
    int read_time;
} cache_t;

verboso_t* verboso_crear(){
	verboso_t* verboso = malloc(sizeof(verboso_t));
	verboso->op_index = 0;
	verboso->id_case = NULL;
	verboso->cache_index = 0;
	verboso->cache_tag = 0;
	verboso->cache_line = 0;
	verboso->line_tag = 0;
	verboso->valid_bit = 0;
	verboso->dirty_bit = 0;
	verboso->last_used = -1;
	return verboso;
}

linea_t* linea_crear() {
	linea_t* linea = malloc(sizeof(linea_t));
	linea->v = 0;
	linea->d = 0;
	linea->tag = -1;
	linea->last_used = 0;
	return linea;
}

set_t* set_crear(size_t cant_lineas) {
	set_t* set = malloc(sizeof(set_t));
	
	if (set == NULL) return NULL;
	
	set->lineas = malloc(cant_lineas * sizeof(linea_t));
	
	if (set->lineas == NULL) {
		free(set);
		return NULL;
	}
	set->cant_lineas = cant_lineas;
	
	for (size_t i = 0; i < cant_lineas; i++) {
		set->lineas[i] = linea_crear(cant_lineas);
		set->lineas[i]->n_linea = i;
	}
	
	return set;
}

cache_t* cache_crear(size_t S, size_t E, size_t C) {
	cache_t* cache = malloc(sizeof(cache_t));
	
	if (cache == NULL) return NULL;
	
	cache->sets = malloc(S * sizeof(set_t));
	
	if (cache->sets == NULL) {
		free(cache);
		return NULL;
	}
	cache->S = S;
	cache->E = E;
	cache->C = C;
	
	cache->loads = 0;
	cache->stores = 0;
	cache->rmiss = 0;
	cache->wmiss = 0;
	cache->dirty_rmiss = 0;
	cache->dirty_wmiss = 0;
	cache->bytes_read = 0;
	cache->bytes_written = 0;
	cache->read_time = 0;
	cache->write_time = 0;
	cache->op_index = 0;
	
	for (size_t i = 0; i < S; i++) {
		cache->sets[i] = set_crear(E);
	}
	return cache;
}

void cache_destruir(cache_t* cache) {
	for (size_t i = 0; i < cache->S; i++) {
		for (size_t j = 0; j < cache->sets[i]->cant_lineas; j++) {
			free(cache->sets[i]->lineas[j]);
		}
		free(cache->sets[i]->lineas);
		free(cache->sets[i]);
	}
	
	free(cache->sets);
	free(cache);
	return;
}

void verboso_imprimir(verboso_t* verboso, int op_index, int n, int m) {
	if (op_index >= n && op_index <= m+1) {
		if (verboso->line_tag == -1) {
			printf("%d %s %x %x %d %d %d %d", verboso->op_index, verboso->id_case, verboso->cache_index, verboso->cache_tag, verboso->cache_line, verboso->line_tag, verboso->valid_bit, verboso->dirty_bit);
		} else {
			printf("%d %s %x %x %d %x %d %d", verboso->op_index, verboso->id_case, verboso->cache_index, verboso->cache_tag, verboso->cache_line, verboso->line_tag, verboso->valid_bit, verboso->dirty_bit);
		}
		if (verboso->last_used != -1) printf(" %d", verboso->last_used);
		printf("\n");
	}
	return;
}

linea_t* cache_buscar_LRU(cache_t* cache, int set_index) {
	int actual_last_used = cache->op_index;
	linea_t* linea_a_desalojar = NULL;
	linea_t* linea_last_used = NULL;
	for (size_t i = 0; i < cache->sets[set_index]->cant_lineas; i++) {
		if(cache->sets[set_index]->lineas[i]->v == 0) {
			linea_a_desalojar = cache->sets[set_index]->lineas[i];
			break;
		}
		if (cache->sets[set_index]->lineas[i]->last_used < actual_last_used) {
			actual_last_used = cache->sets[set_index]->lineas[i]->last_used;
			linea_last_used = cache->sets[set_index]->lineas[i];
		}
	}
	if (!linea_a_desalojar) return linea_last_used;
	return linea_a_desalojar;
}


void cache_hit(cache_t* cache, linea_t* linea, char op) {
	linea->last_used = cache->op_index;
	if (op == 'R') {
		cache->loads++;
		cache->read_time++;
	}
	if (op == 'W') {
		cache->stores++;
		linea->d = 1;
		cache->write_time++;
	}
	cache->op_index++;
	return;
}

linea_t* cache_buscar_linea(cache_t* cache, int set_index, int tag_dir) {	
	for (size_t i = 0; i < cache->sets[set_index]->cant_lineas; i++) {
		if((cache->sets[set_index]->lineas[i]->v == 1) && (cache->sets[set_index]->lineas[i]->tag == tag_dir)) {
			return cache->sets[set_index]->lineas[i];
		}
	}
	return NULL;
}


void cache_acceso(cache_t* cache, int tag_dir, int set_index, char op, int B, verboso_t* verboso, int n, int m) {
	verboso->op_index = cache->op_index;
	verboso->cache_index = set_index;
	verboso->cache_tag = tag_dir;
	
	linea_t* linea_buscada = cache_buscar_linea(cache, set_index, tag_dir);
	
	if (linea_buscada) {
		verboso->id_case = "1";
		verboso->cache_line = linea_buscada->n_linea;
		verboso->line_tag = linea_buscada->tag;
		verboso->valid_bit = linea_buscada->v;
		verboso->dirty_bit = linea_buscada->d;
		if (cache->E > 1) verboso->last_used = linea_buscada->last_used;
		
		cache_hit(cache, linea_buscada, op);
	} else {
		linea_buscada = cache_buscar_LRU(cache, set_index);
		verboso->line_tag = linea_buscada->tag;
		verboso->valid_bit = linea_buscada->v;
		if (cache->E > 1) verboso->last_used = linea_buscada->last_used;
		verboso->dirty_bit = linea_buscada->d;
		verboso->cache_line = linea_buscada->n_linea;
		linea_buscada->v = 1;
		linea_buscada->tag = tag_dir;
		linea_buscada->last_used = cache->op_index;
		cache->bytes_read += B;
		
		if (op == 'R') {
			cache->rmiss++;
			if (linea_buscada->d == 1) {
				verboso->id_case = "2b";
				cache->bytes_written += B;
				cache->dirty_rmiss++;
				cache->read_time += 2 * PENALTY;
				linea_buscada->d = 0;
			} else {
				verboso->id_case = "2a";
				cache->read_time += PENALTY;
			}
		}
			
		if (op == 'W') {
			cache->wmiss++;
			if (linea_buscada->d == 1) {
				verboso->id_case = "2b";
				cache->bytes_written += B;
				cache->dirty_wmiss++;
				cache->write_time += 2 * PENALTY;
			} else {
				verboso->id_case = "2a";
				cache->write_time += PENALTY;
			}
		}
		cache_hit(cache, linea_buscada, op);
	}
	verboso_imprimir(verboso, cache->op_index, n, m);
	return;
}

void cache_estadisticas(cache_t* cache) {
	if (cache->E == 1) {
		printf("direct-mapped, ");
	} else {
		printf("%d-way, ", cache->E);
	}
	int tamanio = cache->C / 1024;
	printf("%d sets, size = %dKB\n", cache->S, tamanio);
	
	int total_accesos = cache->loads + cache->stores;
	printf("loads %d stores %d total %d\n", cache->loads, cache->stores, total_accesos);
	
	int total_misses = cache->rmiss + cache->wmiss;
	printf("rmiss %d wmiss %d total %d\n", cache->rmiss, cache->wmiss, total_misses);

	printf("dirty rmiss %d dirty wmiss %d\n", cache->dirty_rmiss, cache->dirty_wmiss);
	printf("bytes read %d bytes written %d\n", cache->bytes_read, cache->bytes_written);
	printf("read time %d write time %d\n", cache->read_time, cache->write_time);
	
	float miss_rate = (float)total_misses / (float)total_accesos;
	printf("miss rate %f\n", miss_rate);
}

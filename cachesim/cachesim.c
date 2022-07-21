#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cache.h"
#define TAMANIO_DIRECCION 32

bool es_potencia_de_2(size_t numero) {
	return ((numero != 0) && ((numero & (numero - 1)) == 0));
}

bool procesar_operaciones(const char* ruta_archivo, cache_t* cache, int S, int B, int n, int m) {
	int s = (int)log2(S);
	int b = (int)log2(B);
	int t = TAMANIO_DIRECCION - s - b;

	int tag_mask = (1 << t) - 1;
	int set_mask = (1 << s) - 1;
	
	FILE *archivo = fopen(ruta_archivo, "r");
	
	if (!archivo) {
		printf("ERROR\n");
		return false;
	}
	int ip, dir, n_bytes, valor;
	char op;
	
	verboso_t* verboso = verboso_crear();
	
	char* linea = NULL;
	size_t c = 0;
	while (getline(&linea, &c, archivo) > 0) {
		sscanf(linea, "%x: %c %x %d %x\n", &ip, &op, &dir, &n_bytes, &valor);
		
		int tag = (dir >> (s + b)) & tag_mask;
		int set_index = (dir >> b) & set_mask;
		
		cache_acceso(cache, tag, set_index, op, B, verboso, n, m);
	}
	
	free(verboso);
	free(linea);
	fclose(archivo);
	return true;
}


int main(int argc, char **argv) {
	if (argc < 6 && argc != 5) {
		printf("ERROR\n");
		return 1;
	}
		
	int C = atoi(argv[2]);
	int E = atoi(argv[3]);
	int S = atoi(argv[4]);
	
	int B = C / (E * S);
	
	if (!es_potencia_de_2(C) || !es_potencia_de_2(E) || !es_potencia_de_2(S)) {
		printf("ERROR\n");
		return 1;
	}
	
	int m = -1;
	int n = -1;
	
	if (argc > 5) {
		if (argc != 8) {
			printf("ERROR\n");
			return 1;
		}
		n = atoi(argv[6]);
		m = atoi(argv[7]);
		
		if (n < 0 || m < n) {
			printf("ERROR\n");
			return 1;
		}	
	}
	
	cache_t* cache = cache_crear(S, E, C);
	
	if(!procesar_operaciones(argv[1], cache, S, B, n, m)) {
		printf("ERROR\n");
		return 1;
	}
	
	cache_estadisticas(cache);
	cache_destruir(cache);
	return 0;
}

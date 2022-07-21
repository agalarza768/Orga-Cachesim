typedef struct verboso verboso_t;

typedef struct linea linea_t;

typedef struct set set_t;

typedef struct cache cache_t;

verboso_t* verboso_crear();

cache_t* cache_crear(size_t S, size_t E, size_t C);

void cache_destruir(cache_t* cache);

void cache_acceso(cache_t* cache, int tag_dir, int set_index, char op, int B, verboso_t* verboso, int n, int m);

void cache_estadisticas(cache_t* cache);

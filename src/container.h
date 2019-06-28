#ifndef H_CONTAINER
#define H_CONTAINER


#include <stddef.h>

typedef struct {
    void** data;
    size_t capacity;
    size_t len;
} Vector;

typedef struct {
    Vector* keys;
    Vector* vals;
} Map;

Vector* new_vector();
void vec_push(Vector*, void*);

Map* new_map();
void map_put(Map*, char const*, void*);
void* map_get(Map*, char const*);


#endif

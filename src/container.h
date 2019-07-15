#include <stddef.h>

struct vector {
    void** data;
    size_t capacity;
    size_t len;
};
typedef struct vector Vector;

struct map {
    Vector* keys;
    Vector* vals;
};
typedef struct map Map;

#ifndef SELFHOST_9MM
Vector* new_vector();
void vec_push(Vector*, void*);

Map* new_map();
void map_put(Map*, char const*, void*);
void* map_get(Map*, char const*);
#endif

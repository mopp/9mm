#ifndef H_CONTAINER
#define H_CONTAINER

typedef struct {
    void** data;
    int capacity;
    int len;
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

#include "9mm.h"

Vector* new_vector()
{
    Vector* vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void*) * 16);
    vec->capacity = 16;
    vec->len = 0;
    return vec;
}

void vec_push(Vector* vec, void* elem)
{
    if (vec == NULL) {
        error("The given map is NULL");
        return;
    }

    if (vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void*) * vec->capacity);
    }
    vec->data[vec->len++] = elem;
}

Map* new_map()
{
    Map* map = malloc(sizeof(Map));
    map->keys = new_vector();
    map->vals = new_vector();
    return map;
}

void map_put(Map* map, char const* key, void* val)
{
    vec_push(map->keys, (char*)key);
    vec_push(map->vals, val);
}

void* map_get(Map* map, char const* key)
{
    if (map == NULL) {
        error("The given map is NULL");
        return NULL;
    }

    Vector const* keys = map->keys;
    if (keys->len == 0) {
        return NULL;
    }

    // Prefer newer key by traversing by reverse order.
    for (size_t i = keys->len; 0 < i; i--) {
        if (strcmp(keys->data[i - 1], key) == 0) {
            return map->vals->data[i - 1];
        }
    }

    return NULL;
}

#ifndef SELFHOST_9MM
static void expect(int line, int expected, int actual)
{
    if (expected == actual)
        return;
    fprintf(stderr, "%d: %d expected, but got %d\n",
            line, expected, actual);
    exit(1);
}

static inline void test_map()
{
    Map* map = new_map();
    expect(__LINE__, 0, (long)map_get(map, "foo"));

    map_put(map, "foo", (void*)2);
    expect(__LINE__, 2, (long)map_get(map, "foo"));

    map_put(map, "bar", (void*)4);
    expect(__LINE__, 4, (long)map_get(map, "bar"));

    map_put(map, "foo", (void*)6);
    expect(__LINE__, 6, (long)map_get(map, "foo"));

    free(map);
}

static inline void test_vector()
{
    Vector* vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for (uintptr_t i = 0; i < 100; i++)
        vec_push(vec, (void*)i);

    expect(__LINE__, 100, vec->len);
    expect(__LINE__, 0, (uintptr_t)vec->data[0]);
    expect(__LINE__, 50, (uintptr_t)vec->data[50]);
    expect(__LINE__, 99, (uintptr_t)vec->data[99]);

    free(vec);
}

void runtest()
{
    test_vector();
    test_map();
}
#endif

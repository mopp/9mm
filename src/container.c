#include "container.h"
#include "9mm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    // 逆順に探すことで新しいキーを優先する.
    for (size_t i = map->keys->len - 1; i >= 0; i--)
        if (strcmp(map->keys->data[i], key) == 0)
            return map->vals->data[i];
    return NULL;
}

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

    printf("OK\n");
}

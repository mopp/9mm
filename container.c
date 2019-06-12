#include "9mm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

void vec_push_token(Vector* tokens, int ty, int val, char* input)
{
    Token* t = malloc(sizeof(Token));
    t->ty = ty;
    t->val = val;
    t->input = input;
    vec_push(tokens, t);
}

static void expect(int line, int expected, int actual)
{
    if (expected == actual)
        return;
    fprintf(stderr, "%d: %d expected, but got %d\n",
            line, expected, actual);
    exit(1);
}

void runtest()
{
    Vector* vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for (uintptr_t i = 0; i < 100; i++)
        vec_push(vec, (void*)i);

    expect(__LINE__, 100, vec->len);
    expect(__LINE__, 0, (uintptr_t)vec->data[0]);
    expect(__LINE__, 50, (uintptr_t)vec->data[50]);
    expect(__LINE__, 99, (uintptr_t)vec->data[99]);

    printf("OK\n");
}

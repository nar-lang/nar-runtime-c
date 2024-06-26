#ifndef NAR_RUNTIME_VECTOR_H
#define NAR_RUNTIME_VECTOR_H

#include <stddef.h>
#include <string.h>
#include "nar.h"
#include "nar-runtime.h"

typedef nar_ptr_t (*realloc_fn_t)(nar_ptr_t mem, nar_size_t size);
typedef void (*free_fn_t)(nar_ptr_t mem);
typedef void (*fail_fn_t)(nar_runtime_t rt, nar_cstring_t msg);

#define nvector_new(item_size, capacity, nar) \
    vector_new(item_size, capacity, nar->realloc, nar->free, nar->fail)

#define rvector_new(item_size, capacity) \
    vector_new(item_size, capacity, \
    (realloc_fn_t)nar_realloc, (free_fn_t)nar_free, (fail_fn_t)nar_fail)

typedef struct {
    void *data;
    size_t size;
    size_t capacity;
    size_t item_size;
    realloc_fn_t realloc;
    free_fn_t free;
    fail_fn_t fail;
} vector_t;

static void __ensure_capacity(vector_t *v, size_t new_len) {
    if (v->capacity < new_len) {
        size_t old_cap = v->capacity;
        size_t new_cap = old_cap;
        size_t double_cap = new_cap + new_cap;
        if (new_len > double_cap) {
            new_cap = new_len;
        } else {
            const size_t threshold = 256;
            if (old_cap < threshold) {
                new_cap = double_cap;
            } else {
                while (1) {
                    new_cap += (new_cap + 3 * threshold) >> 2;
                    if (new_cap >= new_len) {
                        break;
                    }
                }
            }
        }
        v->capacity = new_cap;
        if (v->capacity > 0) {
            v->data = v->realloc(v->data, v->capacity * v->item_size);
        }
    }
}

static vector_t *vector_new(
        size_t item_size, size_t capacity, realloc_fn_t realloc, free_fn_t free, fail_fn_t fail) {
    vector_t *v = realloc(NULL, sizeof(vector_t));
    v->data = NULL;
    v->size = 0;
    v->capacity = 0;
    v->item_size = item_size;
    v->realloc = realloc;
    v->free = free;
    v->fail = fail;
    __ensure_capacity(v, capacity);
    return v;
}

static void vector_free(vector_t *v) {
    if (v == NULL) {
        return;
    }
    v->free(v->data);
    v->free(v);
}

static void vector_push(vector_t *v, size_t n, const void *items) {
    if (n == 0) {
        return;
    }
    __ensure_capacity(v, v->size + n);
    memcpy((char *) v->data + v->size * v->item_size, items, n * v->item_size);
    v->size += n;
}

static void vector_insert(vector_t *v, size_t index, size_t n, const void *items) {
    if (index > v->size) {
        v->fail(NULL, "vector_insert: index > v->size");
        return;
    }
    if (n == 0) {
        return;
    }
    __ensure_capacity(v, v->size + n);
    if (index < v->size) {
        memmove((char *) v->data + (index + n) * v->item_size,
                (char *) v->data + index * v->item_size, (v->size - index) * v->item_size);
    }
    memcpy((char *) v->data + index * v->item_size, items, n * v->item_size);
    v->size += n;
}

static void vector_pop(vector_t *v, size_t n, void *items) {
    if (n == 0) {
        return;
    }

    if (n > v->size) {
        v->fail(NULL, "vector_pop: n >= v->size");
        return;
    }

    v->size -= n;
    if (items != NULL) {
        memcpy(items, (char *) v->data + v->size * v->item_size, n * v->item_size);
    }
}

static void vector_pop_vec(vector_t *v, size_t n, vector_t *items) {
    if (n == 0) {
        return;
    }

    if (n > v->size) {
        v->fail(NULL, "vector_pop_vec: n >= v->size");
        return;
    }

    v->size -= n;
    if (items != NULL) {
        vector_push(items, n, (char *) v->data + v->size * v->item_size);
    }
}

static void *vector_at(vector_t *v, size_t index) {
    if (index >= v->size) {
        v->fail(NULL, "vector_at: index >= v->size");
        return NULL;
    }

    return (char *) v->data + index * v->item_size;
}

static size_t vector_size(vector_t *v) {
    return v->size;
}

static void *vector_data(vector_t *v) {
    return v->data;
}

static void *vector_begin(vector_t *v) {
    return v->data;
}

static void *vector_end(vector_t *v) {
    if (v->data == NULL) {
        return NULL;
    }
    return (char *) v->data + v->size * v->item_size;
}

static void vector_clear(vector_t *v) {
    if (v->size == 0) {
        return;
    }
    v->size = 0;
}

static void vector_remove(vector_t *v, size_t index) {
    if (index >= v->size) {
        v->fail(NULL, "vector_remove: index >= v->size");
        return;
    }
    if (index < v->size - 1) {
        memmove((char *) v->data + index * v->item_size,
                (char *) v->data + (index + 1) * v->item_size, (v->size - index - 1) * v->item_size);
    }
    v->size--;
}

static void vector_remove_fast_loosing_order(vector_t *v, size_t index) {
    if (index >= v->size) {
        v->fail(NULL, "vector_remove_fast_loosing_order: index >= v->size");
        return;
    }
    if (index < v->size - 1) {
        memcpy((char *) v->data + index * v->item_size,
                (char *) v->data + (v->size - 1) * v->item_size, v->item_size);
    }
    v->size--;
}

#endif //NAR_RUNTIME_VECTOR_H

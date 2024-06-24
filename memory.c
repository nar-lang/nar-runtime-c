#include <stdlib.h>
#include <assert.h>
#include <printf.h>
#include "include/nar-runtime.h"
#include "runtime.h"

#define MEMORY_DEBUG

#ifdef MEMORY_DEBUG
vector_t *memory_indices;
size_t memory_index = 0;
#endif

nar_ptr_t nar_alloc(nar_size_t size) {
    if (size == 0) {
        return NULL;
    }

#ifdef MEMORY_DEBUG
    if (memory_indices == NULL) {
        memory_indices = vector_new(sizeof(size_t), 0, realloc, free, &nar_fail);
    }
    size_t index = memory_index++;
    vector_push(memory_indices, 1, &index);
    nar_ptr_t mem = malloc(size + sizeof(size_t));
    assert(mem && "Out of memory");
    *((size_t *) mem) = index;
    mem += sizeof(size_t);
#else
    nar_ptr_t mem = malloc(size + sizeof(size_t));
    assert(mem && "Out of memory");
#endif

    return mem;
}

nar_ptr_t nar_realloc(nar_ptr_t mem, nar_size_t size) {
    if (size == 0) {
        nar_free(mem);
        return NULL;
    }
    if (mem == NULL) {
        return nar_alloc(size);
    }
#ifdef MEMORY_DEBUG
    nar_ptr_t new_mem = realloc(mem - sizeof(size_t), size + sizeof(size_t));
    assert(new_mem && "Out of memory");
    new_mem += sizeof(size_t);
#else
    nar_ptr_t new_mem = realloc(mem, size + sizeof(size_t));
    assert(new_mem && "Out of memory");
#endif
    return new_mem;
}

void nar_free(nar_ptr_t mem) {
    if (mem == NULL) {
        return;
    }
#ifdef MEMORY_DEBUG
    mem -= sizeof(size_t);
    bool found = false;
    size_t index = *((size_t *) mem);
    for (size_t i = 0; i < vector_size(memory_indices); i++) {
        size_t *it = vector_at(memory_indices, i);
        if (*it == index) {
            vector_remove_fast_loosing_order(memory_indices, i);
            found = true;
            break;
        }
    }
    assert(found && "trying to free memory that was not allocated");
#endif
    free(mem);
}

void nar_print_memory() {
#ifdef MEMORY_DEBUG
    for (size_t i = 0; i < vector_size(memory_indices); i++) {
        size_t *it = vector_at(memory_indices, i);
        printf("memory leak index: %d\n", (int) *((size_t *) it));
    }
#endif
}

nar_ptr_t nar_frame_alloc(nar_runtime_t rt, nar_size_t size) {
    nar_ptr_t ptr = nar_alloc(size);
    vector_push(((runtime_t *) rt)->frame_memory, 1, &ptr);
    return ptr;
}

void frame_free(runtime_t *rt, bool create_defaults) {
    vector_t *mem = rt->frame_memory;
    for (nar_ptr_t *it = vector_begin(mem); it != vector_end(mem); it++) {
        nar_free(*it);
    }
    vector_clear(mem);

    for (size_t i = 0; i < NAR_OBJECT_KIND__COUNT; i++) {
        vector_t *arena = rt->arenas[i];
        if (arena != NULL) {
            vector_clear(arena);
        }
    }

    vector_clear(rt->locals);
    vector_clear(rt->call_stack);
    hashmap_clear(rt->string_hashes, false);

    if (create_defaults) {
        nar_make_string(rt, "");
        nar_make_option(rt, OPTION_NAME_FALSE, 0, NULL);
        nar_make_option(rt, OPTION_NAME_TRUE, 0, NULL);
    }
}

void nar_frame_free(nar_runtime_t rt) {
    if (rt != NULL) {
        frame_free((runtime_t *) rt, true);
    }
}
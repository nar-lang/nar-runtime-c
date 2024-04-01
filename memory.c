#include <stdlib.h>
#include <assert.h>
#include "include/nar-runtime.h"
#include "runtime.h"

size_t allocated_memory = 0;

nar_ptr_t nar_alloc(nar_size_t size) {
    if (size == 0) {
        return NULL;
    }
    nar_ptr_t mem = malloc(size + sizeof(size_t));
    assert(mem && "Out of memory");

    *((size_t *) mem) = size;
    allocated_memory += size;
    mem += sizeof(size_t);

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
    mem -= sizeof(size_t);
    allocated_memory -= *((size_t *) mem);
    nar_ptr_t new_mem = realloc(mem, size + sizeof(size_t));
    assert(new_mem && "Out of memory");
    *((size_t *) new_mem) = size;
    allocated_memory += size;
    new_mem += sizeof(size_t);
    return new_mem;
}

void nar_free(nar_ptr_t mem) {
    if (mem == NULL) {
        return;
    }
    mem -= sizeof(size_t);
    allocated_memory -= *((size_t *) mem);
    free(mem);
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
        nar_new_string(rt, "");
        nar_new_option(rt, OPTION_NAME_FALSE, 0, NULL);
        nar_new_option(rt, OPTION_NAME_TRUE, 0, NULL);
    }
}

void nar_frame_free(nar_runtime_t rt) {
    if (rt != NULL) {
        frame_free((runtime_t *) rt, true);
    }
}
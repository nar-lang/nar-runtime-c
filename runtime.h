#ifndef NAR_RUNTIME_RUNTIME_H
#define NAR_RUNTIME_RUNTIME_H

#include "include/nar-package.h"
#include "include/hashmap/hashmap.h"
#include "bytecode.h"
#include "include/vector.h"

#define build_object(kind, index)  (((nar_object_t)kind << 56) | (nar_object_t)index)
#define OPTION_NAME_TRUE "Nar.Base.Basics.Bool#True"
#define OPTION_NAME_FALSE "Nar.Base.Basics.Bool#False"

typedef struct {
    nar_cstring_t name;
    nar_object_t value;
} local_t;

typedef struct {
    nar_cstring_t name;
    nar_cptr_t fn;
    nar_size_t arity;
} native_def_item_t;

typedef struct {
    nar_string_t name;
    nar_cptr_t value;
} metadata_item_t;

typedef struct {
    pattern_kind_t kind;
    nar_cstring_t name;
    nar_object_t values;
} nar_pattern_t;

typedef struct {
    nar_cstring_t string;
    nar_object_t index;
} string_hast_t;

typedef struct {
    bytecode_t *program;
    hashmap_t *native_defs; // of native_def_item_t
    hashmap_t *string_hashes; // of string_hast_t
    vector_t **arenas; // vector_t of nar_object_t
    vector_t *locals; // of local_t
    vector_t *frame_memory; // of nar_ptr_t
    vector_t *call_stack; // of nar_string_t
    vector_t *lib_handles; // of nar_ptr_t
    void* last_lib_handle;
    nar_t *package_pointers;
    nar_string_t last_error;
    hashmap_t *metadata; // of metadata_item_t
    nar_stdout_fn_t stdout;
    //TODO: vector_t stack; // of nar_object_t -- introduce single stack for objects
    //TODO: vector_t patterns; // of nar_object_t -- introduce single stack for patterns
} runtime_t;

void frame_free(runtime_t *rt, bool create_defaults);
nar_object_t execute(runtime_t *rt, const func_t *fn, vector_t *stack);
nar_object_t nar_make_pattern(
        nar_runtime_t rt, pattern_kind_t kind,
        nar_cstring_t name, size_t num_items, nar_object_t *items);
nar_pattern_t nar_to_pattern(nar_runtime_t rt, nar_object_t pattern);
nar_string_t frame_string_dup(runtime_t *rt, nar_cstring_t str);
nar_string_t string_dup(nar_cstring_t str);
void nar_register_def_dynamic(
        nar_runtime_t rt, nar_cstring_t module_name, nar_cstring_t def_name,
        nar_cstring_t func_name, nar_size_t arity);

#endif //NAR_RUNTIME_RUNTIME_H

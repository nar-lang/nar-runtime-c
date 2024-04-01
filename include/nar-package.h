#ifndef NAR_PACKAGE_H
#define NAR_PACKAGE_H

#include "nar.h"

typedef struct {
    // Memory API

    nar_ptr_t (*alloc)(nar_size_t size);

    nar_ptr_t (*realloc)(nar_ptr_t mem, nar_size_t size);

    void (*free)(nar_ptr_t mem);

    void *(*frame_alloc)(nar_runtime_t rt, nar_size_t size);

    // Runtime API

    void (*set_metadata)(nar_runtime_t rt, nar_cstring_t key, nar_cptr_t value);

    nar_cptr_t (*get_metadata)(nar_runtime_t rt, nar_cstring_t key);

    nar_object_t (*apply)(
            nar_runtime_t rt, nar_cstring_t name, nar_size_t num_args, const nar_object_t *args);

    void (*register_def)(
            nar_runtime_t rt, nar_cstring_t module_name, nar_cstring_t def_name,
            nar_cptr_t fn, nar_size_t arity);

    void (*register_def_dynamic)(
            nar_runtime_t rt, nar_cstring_t module_name, nar_cstring_t def_name,
            nar_cstring_t func_name, nar_size_t arity);

    nar_object_t (*apply_func)(
            nar_runtime_t rt, nar_object_t fn, nar_size_t num_args, const nar_object_t *args);

    void (*print)(nar_runtime_t rt, nar_cstring_t message);

    void (*fail)(nar_runtime_t rt, nar_cstring_t message);

    nar_cstring_t (*get_last_error)(nar_runtime_t rt);

    // Object API

    nar_object_kind_t (*object_get_kind)(nar_runtime_t rt, nar_object_t obj);

    nar_bool_t (*object_is_valid)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_unit)(nar_runtime_t rt);

    void (*to_unit)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_char)(nar_runtime_t rt, nar_char_t value);

    nar_char_t (*to_char)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_int)(nar_runtime_t rt, nar_int_t value);

    nar_int_t (*to_int)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_float)(nar_runtime_t rt, nar_float_t value);

    nar_float_t (*to_float)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_string)(nar_runtime_t rt, nar_cstring_t value);

    nar_cstring_t (*to_string)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_record)(
            nar_runtime_t rt, nar_size_t size, const nar_string_t *keys,
            const nar_object_t *values);

    nar_object_t (*new_record_field)(
            nar_runtime_t rt, nar_object_t record, nar_cstring_t key, nar_object_t value);

    nar_object_t (*new_record_field_obj)(
            nar_runtime_t rt, nar_object_t record, nar_object_t key, nar_object_t value);

    nar_object_t (*new_record_raw)(nar_runtime_t rt, size_t size, const nar_object_t *stack);

    nar_record_t (*to_record)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*to_record_field)(nar_runtime_t rt, nar_object_t obj, nar_cstring_t key);

    nar_record_item_t (*to_record_item)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_list_cons)(nar_runtime_t rt, nar_object_t head, nar_object_t tail);

    nar_object_t (*new_list)(nar_runtime_t rt, nar_size_t size, const nar_object_t *items);

    nar_list_t (*to_list)(nar_runtime_t rt, nar_object_t obj);

    nar_list_item_t (*to_list_item)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_tuple)(nar_runtime_t rt, nar_size_t size, const nar_object_t *items);

    nar_tuple_t (*to_tuple)(nar_runtime_t rt, nar_object_t obj);

    nar_tuple_item_t (*to_tuple_item)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_option)(
            nar_runtime_t rt, nar_cstring_t name, nar_size_t size, const nar_object_t *items);

    nar_option_t (*to_option)(nar_runtime_t rt, nar_object_t obj);

    nar_option_item_t (*to_option_item)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_bool)(nar_runtime_t rt, nar_bool_t value);

    nar_bool_t (*to_bool)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_func)(nar_runtime_t rt, nar_ptr_t fn, nar_size_t arity);

    nar_func_t (*to_func)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_native)(nar_runtime_t rt, nar_ptr_t ptr, nar_cmp_native_fn_t cmp);

    nar_native_t (*to_native)(nar_runtime_t rt, nar_object_t obj);

    nar_object_t (*new_closure)(
            nar_runtime_t rt, size_t fn_index, size_t num_args, const nar_object_t *args);

    nar_closure_t (*to_closure)(nar_runtime_t rt, nar_object_t obj);
} nar_t;

typedef nar_int_t (*init_fn_t)(const nar_t *, nar_runtime_t);

#endif //NAR_PACKAGE_H

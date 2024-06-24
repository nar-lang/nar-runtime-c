#ifndef NAR_RUNTIME_H
#define NAR_RUNTIME_H

#include "nar.h"

// Memory API

nar_ptr_t nar_alloc(nar_size_t size);

nar_ptr_t nar_realloc(nar_ptr_t mem, nar_size_t size);

void nar_free(nar_ptr_t mem);

nar_ptr_t nar_frame_alloc(nar_runtime_t rt, nar_size_t size);

void nar_frame_free(nar_runtime_t rt);

// Bytecode API
nar_bytecode_t nar_bytecode_new(nar_size_t size, const nar_byte_t *data);

nar_cstring_t nar_bytecode_get_entry(nar_bytecode_t btc);

void nar_bytecode_free(nar_bytecode_t bc);

// Runtime API

nar_runtime_t nar_runtime_new(nar_bytecode_t btc);

void nar_runtime_replace_program(nar_runtime_t rt, nar_bytecode_t btc);

void nar_runtime_free(nar_runtime_t rt);

void nar_set_metadata(nar_runtime_t rt, nar_cstring_t key, nar_cptr_t value);

nar_cptr_t nar_get_metadata(nar_runtime_t rt, nar_cstring_t key);

void nar_set_stdout(nar_runtime_t rt, nar_stdout_fn_t stdout);

nar_bool_t nar_register_libs(nar_runtime_t rt, nar_cstring_t libs_path);

void nar_register_def(
        nar_runtime_t rt, nar_cstring_t module_name, nar_cstring_t def_name,
        nar_cptr_t fn, nar_size_t arity);

nar_object_t nar_apply(
        nar_runtime_t rt, nar_cstring_t name, nar_size_t num_args, const nar_object_t *args);

nar_object_t nar_apply_func(
        nar_runtime_t rt, nar_object_t fn, nar_size_t num_args, const nar_object_t *args);

void nar_print(__attribute__((unused)) nar_runtime_t rt, nar_cstring_t message);

void nar_fail(nar_runtime_t rt, nar_cstring_t message);

nar_cstring_t nar_get_error(nar_runtime_t rt);

void nar_clear_error(nar_runtime_t rt);

// Object API

nar_object_kind_t nar_object_get_kind(__attribute__((unused)) nar_runtime_t rt, nar_object_t obj);

nar_bool_t nar_object_is_valid(__attribute__((unused)) nar_runtime_t rt, nar_object_t obj);

nar_bool_t nar_index_is_valid(__attribute__((unused)) nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_unit(__attribute__((unused)) nar_runtime_t rt);

void nar_to_unit(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_char(nar_runtime_t rt, nar_char_t value);

nar_char_t nar_to_char(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_int(nar_runtime_t rt, nar_int_t value);

nar_int_t nar_to_int(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_float(nar_runtime_t rt, nar_float_t value);

nar_float_t nar_to_float(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_string(nar_runtime_t rt, nar_cstring_t value);

nar_cstring_t nar_to_string(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_record(
        nar_runtime_t rt, nar_size_t size, const nar_cstring_t *keys, const nar_object_t *values);

nar_object_t nar_make_record_field(
        nar_runtime_t rt, nar_object_t record, nar_cstring_t key, nar_object_t value);

nar_object_t nar_make_record_field_obj(
        nar_runtime_t rt, nar_object_t record, nar_object_t key, nar_object_t value);

nar_object_t nar_make_record_raw(nar_runtime_t rt, size_t num_fields, const nar_object_t *stack);

nar_record_t nar_to_record(nar_runtime_t rt, nar_object_t obj);

void nar_map_record(nar_runtime_t rt, nar_object_t obj, void *result, nar_map_record_cb_fn_t map);

nar_object_t nar_to_record_field(nar_runtime_t rt, nar_object_t obj, nar_cstring_t key);

nar_record_item_t nar_to_record_item(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_list_cons(nar_runtime_t rt, nar_object_t head, nar_object_t tail);

nar_object_t nar_make_list(nar_runtime_t rt, nar_size_t size, const nar_object_t *items);

nar_list_t nar_to_list(nar_runtime_t rt, nar_object_t obj);

nar_list_item_t nar_to_list_item(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_tuple(nar_runtime_t rt, nar_size_t size, const nar_object_t *items);

nar_tuple_t nar_to_tuple(nar_runtime_t rt, nar_object_t obj);

nar_tuple_item_t nar_to_tuple_item(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_option(
        nar_runtime_t rt, nar_cstring_t name, nar_size_t size, const nar_object_t *items);

nar_option_t nar_to_option(nar_runtime_t rt, nar_object_t obj);

nar_option_item_t nar_to_option_item(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_bool(__attribute__((unused)) nar_runtime_t rt, nar_bool_t value);

nar_bool_t nar_to_bool(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_func(nar_runtime_t rt, nar_cptr_t fn, nar_size_t arity);

nar_func_t nar_to_func(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_native(nar_runtime_t rt, nar_cptr_t ptr, nar_cmp_native_fn_t cmp);

nar_native_t nar_to_native(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_make_closure(
        nar_runtime_t rt, size_t fn_index, size_t num_args, const nar_object_t *args);

nar_closure_t nar_to_closure(nar_runtime_t rt, nar_object_t obj);

nar_serialized_object_t nar_new_serialized_object(nar_runtime_t rt, nar_object_t obj);

nar_object_t nar_deserialize_object(nar_runtime_t rt, nar_serialized_object_t obj);

// various helpers

nar_bool_t nar_to_enum_option_s(nar_runtime_t rt, nar_object_t opt, nar_int_t *value);

nar_int_t nar_to_enum_option(nar_runtime_t rt, nar_object_t opt);

nar_int_t nar_to_enum_option_flags(nar_runtime_t rt, nar_object_t list);

nar_object_t nar_make_enum_option(nar_runtime_t rt,
        nar_cstring_t type, nar_int_t value, nar_size_t size, const nar_object_t *items);

nar_object_t nar_make_enum_option_flags(nar_runtime_t rt,
        nar_cstring_t type, nar_int_t flags);

void nar_enum_def(nar_cstring_t type, nar_cstring_t option, nar_int_t value);

#endif //NAR_RUNTIME_H

#if defined (__unix__)
#define NAR_UNIX
#include <dlfcn.h>
#elif defined (_WIN32) || defined (_WIN64)
#define NAR_WINDOWS
#include <windows.h>
#elif defined (__APPLE__)
#define NAR_APPLE
#include <dlfcn.h>
#endif

#include <string.h>
#include <stdio.h>
#include "include/nar.h"
#include "bytecode.h"
#include "runtime.h"
#include "include/nar-runtime.h"

nar_string_t general_last_error = NULL;

int native_def_item_compare(const void *a, const void *b, __attribute__((unused)) void *data) {
    const native_def_item_t *ia = a;
    const native_def_item_t *ib = b;
    return strcmp(ia->name, ib->name);
}

uint64_t native_def_item_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const native_def_item_t *i = item;
    return hashmap_sip(i->name, strlen(i->name), seed0, seed1);
}

void native_def_item_free(void *item) {
    native_def_item_t *i = item;
    nar_free((nar_string_t) i->name);
}

int string_hast_compare(const void *a, const void *b, __attribute__((unused)) void *data) {
    const string_hast_t *ia = a;
    const string_hast_t *ib = b;
    return strcmp(ia->string, ib->string);
}

uint64_t string_hast_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const string_hast_t *i = item;
    return hashmap_sip(i->string, strlen(i->string), seed0, seed1);
}

int metadata_item_compare(const void *a, const void *b, __attribute__((unused)) void *data) {
    const metadata_item_t *ia = a;
    const metadata_item_t *ib = b;
    return strcmp(ia->name, ib->name);
}

uint64_t metadata_item_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const metadata_item_t *i = item;
    return hashmap_sip(i->name, strlen(i->name), seed0, seed1);
}

void metadata_item_free(void *item) {
    metadata_item_t *i = item;
    nar_free((nar_string_t) i->name);
}

void default_stdout(__attribute__((unused)) nar_runtime_t rt, nar_cstring_t msg) {
    printf("%s\n", msg);
}

nar_runtime_t nar_runtime_new(nar_bytecode_t btc) {
    runtime_t *rt = nar_alloc(sizeof(runtime_t));
    memset(rt, 0, sizeof(runtime_t));

    rt->package_pointers = nar_alloc(sizeof(nar_t));
    rt->package_pointers->alloc = &nar_alloc;
    rt->package_pointers->realloc = &nar_realloc;
    rt->package_pointers->free = &nar_free;
    rt->package_pointers->frame_alloc = &nar_frame_alloc;
    rt->package_pointers->frame_free = &nar_frame_free;
    rt->package_pointers->set_metadata = &nar_set_metadata;
    rt->package_pointers->get_metadata = &nar_get_metadata;
    rt->package_pointers->register_def = &nar_register_def;
    rt->package_pointers->register_def_dynamic = &nar_register_def_dynamic;
    rt->package_pointers->apply = &nar_apply;
    rt->package_pointers->apply_func = &nar_apply_func;
    rt->package_pointers->print = &nar_print;
    rt->package_pointers->fail = &nar_fail;
    rt->package_pointers->get_last_error = &nar_get_error;
    rt->package_pointers->object_get_kind = &nar_object_get_kind;
    rt->package_pointers->object_is_valid = &nar_object_is_valid;
    rt->package_pointers->index_is_valid = &nar_index_is_valid;
    rt->package_pointers->make_unit = &nar_make_unit;
    rt->package_pointers->to_unit = &nar_to_unit;
    rt->package_pointers->make_char = &nar_make_char;
    rt->package_pointers->to_char = &nar_to_char;
    rt->package_pointers->make_int = &nar_make_int;
    rt->package_pointers->to_int = &nar_to_int;
    rt->package_pointers->make_float = &nar_make_float;
    rt->package_pointers->to_float = &nar_to_float;
    rt->package_pointers->make_string = &nar_make_string;
    rt->package_pointers->to_string = &nar_to_string;
    rt->package_pointers->make_record = &nar_make_record;
    rt->package_pointers->make_record_field = &nar_make_record_field;
    rt->package_pointers->make_record_field_obj = &nar_make_record_field_obj;
    rt->package_pointers->make_record_raw = &nar_make_record_raw;
    rt->package_pointers->to_record = &nar_to_record;
    rt->package_pointers->map_record = &nar_map_record;
    rt->package_pointers->to_record_field = &nar_to_record_field;
    rt->package_pointers->to_record_item = &nar_to_record_item;
    rt->package_pointers->make_list_cons = &nar_make_list_cons;
    rt->package_pointers->make_list = &nar_make_list;
    rt->package_pointers->to_list = &nar_to_list;
    rt->package_pointers->to_list_item = &nar_to_list_item;
    rt->package_pointers->make_tuple = &nar_make_tuple;
    rt->package_pointers->to_tuple = &nar_to_tuple;
    rt->package_pointers->to_tuple_item = &nar_to_tuple_item;
    rt->package_pointers->make_option = &nar_make_option;
    rt->package_pointers->to_option = &nar_to_option;
    rt->package_pointers->to_option_item = &nar_to_option_item;
    rt->package_pointers->make_bool = &nar_make_bool;
    rt->package_pointers->to_bool = &nar_to_bool;
    rt->package_pointers->make_func = &nar_make_func;
    rt->package_pointers->to_func = &nar_to_func;
    rt->package_pointers->make_native = &nar_make_native;
    rt->package_pointers->to_native = &nar_to_native;
    rt->package_pointers->make_closure = &nar_make_closure;
    rt->package_pointers->to_closure = &nar_to_closure;
    rt->package_pointers->new_serialized_object = &nar_new_serialized_object;
    rt->package_pointers->deserialize_object = &nar_deserialize_object;

    rt->package_pointers->to_enum_option_s = &nar_to_enum_option_s;
    rt->package_pointers->to_enum_option = &nar_to_enum_option;
    rt->package_pointers->to_enum_option_flags = &nar_to_enum_option_flags;
    rt->package_pointers->make_enum_option = &nar_make_enum_option;
    rt->package_pointers->make_enum_option_flags = &nar_make_enum_option_flags;
    rt->package_pointers->enum_def = &nar_enum_def;

    rt->program = btc;
    rt->native_defs = hashmap_new(sizeof(native_def_item_t), 128, 0, 0,
            &native_def_item_hash, &native_def_item_compare, &native_def_item_free, NULL);
    rt->string_hashes = hashmap_new(sizeof(string_hast_t), 128, 0, 0,
            &string_hast_hash, &string_hast_compare, NULL, NULL);

    rt->arenas = nar_alloc(NAR_OBJECT_KIND__COUNT * sizeof(void *));
    memset(rt->arenas, 0, NAR_OBJECT_KIND__COUNT * sizeof(void *));
    rt->arenas[NAR_OBJECT_KIND_CHAR] = rvector_new(sizeof(nar_char_t), 128);
    rt->arenas[NAR_OBJECT_KIND_INT] = rvector_new(sizeof(nar_int_t), 128);
    rt->arenas[NAR_OBJECT_KIND_FLOAT] = rvector_new(sizeof(nar_float_t), 128);
    rt->arenas[NAR_OBJECT_KIND_STRING] = rvector_new(sizeof(nar_string_t), 128);
    rt->arenas[NAR_OBJECT_KIND_RECORD] = rvector_new(sizeof(nar_record_t), 128);
    rt->arenas[NAR_OBJECT_KIND_TUPLE] = rvector_new(sizeof(nar_tuple_t), 128);
    rt->arenas[NAR_OBJECT_KIND_LIST] = rvector_new(sizeof(nar_list_t), 128);
    rt->arenas[NAR_OBJECT_KIND_OPTION] = rvector_new(sizeof(nar_option_t), 128);
    rt->arenas[NAR_OBJECT_KIND_FUNCTION] = rvector_new(sizeof(nar_func_t), 128);
    rt->arenas[NAR_OBJECT_KIND_CLOSURE] = rvector_new(sizeof(nar_closure_t), 128);
    rt->arenas[NAR_OBJECT_KIND_NATIVE] = rvector_new(sizeof(nar_native_t), 128);
    rt->arenas[NAR_OBJECT_KIND_PATTERN] = rvector_new(sizeof(nar_pattern_t), 128);

    rt->locals = rvector_new(sizeof(local_t), 64);
    rt->frame_memory = rvector_new(sizeof(nar_ptr_t), 512);
    rt->call_stack = rvector_new(sizeof(nar_string_t), 32);
    rt->lib_handles = rvector_new(sizeof(nar_ptr_t), 0);
    rt->last_error = NULL;
    rt->metadata = hashmap_new(sizeof(metadata_item_t), 128, 0, 0,
            &metadata_item_hash, &metadata_item_compare, &metadata_item_free, NULL);

    nar_set_stdout(rt, NULL);

    nar_frame_free(rt);
    return rt;
}

void nar_runtime_replace_program(nar_runtime_t rt, nar_bytecode_t btc) {
    runtime_t * r = ((runtime_t *) rt);
    nar_bytecode_free(r->program);
    r->program = btc;
}

void library_free(void *handle) {
#if defined(NAR_UNIX) || defined(NAR_APPLE)
    dlclose(handle);
#endif
#if defined(NAR_WINDOWS)
    FreeLibrary(handle);
#endif
}

void nar_runtime_free(nar_runtime_t rt) {
    if (rt != NULL) {
        frame_free(rt, false);
        runtime_t *r = (runtime_t *) rt;
        hashmap_free(r->native_defs);
        hashmap_free(r->string_hashes);
        for (size_t i = 0; i < NAR_OBJECT_KIND__COUNT; i++) {
            vector_free(r->arenas[i]);
        }
        nar_free(r->arenas);
        vector_free(r->locals);
        vector_free(r->frame_memory);
        vector_free(r->call_stack);
        nar_free(r->last_error);
        for (nar_ptr_t *it = vector_begin(r->lib_handles); it != vector_end(r->lib_handles); it++) {
            library_free(*it);
        }
        vector_free(r->lib_handles);
        nar_free(r->package_pointers);
        hashmap_free(r->metadata);
        nar_bytecode_free(r->program);
        nar_free(rt);
    }
}

void nar_register_def(
        nar_runtime_t rt, nar_cstring_t module_name, nar_cstring_t def_name,
        nar_cptr_t fn, nar_size_t arity) {
    nar_string_t key = nar_alloc((strlen(module_name) + strlen(def_name) + 2));
    strcpy(key, module_name);
    strcat(key, ".");
    strcat(key, def_name);
    hashmap_set(((runtime_t *) rt)->native_defs,
            &(native_def_item_t) {.name = key, .fn = fn, .arity = arity});
}

void nar_register_def_dynamic(
        nar_runtime_t rt, nar_cstring_t module_name, nar_cstring_t def_name,
        nar_cstring_t func_name, nar_size_t arity) {
    nar_cptr_t fn;
#if defined(NAR_UNIX) || defined(NAR_APPLE)
    fn = dlsym(((runtime_t *) rt)->last_lib_handle, func_name);
#endif
#if defined(NAR_WINDOWS)
    init_fn = GetProcAddress(((runtime_t*)rt)->last_lib_handle, func_name);
#endif

    if (fn == NULL) {
        char err[1024];
        snprintf(err, 1024, "failed to find function %s in library", func_name);
        nar_fail(rt, err);
    }

    nar_register_def(rt, module_name, def_name, fn, arity);
}

nar_object_t nar_apply(
        nar_runtime_t rt, nar_cstring_t name, nar_size_t num_args, const nar_object_t *args) {
    runtime_t *r = (runtime_t *) rt;
    if (vector_size(r->call_stack) > 0) {
        nar_fail(rt, "runtime supports only singe threaded execution");
        return NAR_INVALID_OBJECT;
    }
    const exports_item_t *export_item = hashmap_get(r->program->exports,
            &(exports_item_t) {.name = name});
    if (export_item == NULL) {
        nar_fail(rt, "definition not exported in bytecode");
        return NAR_INVALID_OBJECT;
    }
    nar_object_t afn = nar_make_closure(rt, export_item->index, 0, NULL);
    return nar_apply_func(rt, afn, num_args, args);
}

nar_object_t nar_apply_func( // NOLINT(*-no-recursion)
        nar_runtime_t rt, nar_object_t fn, nar_size_t num_args, const nar_object_t *args) {
    nar_closure_t afn = nar_to_closure(rt, fn);
    nar_list_t curried = nar_to_list(rt, afn.curried);
    size_t num_all_args = (num_args + curried.size);
    vector_t *all_args = rvector_new(sizeof(nar_object_t), num_all_args);

    vector_push(all_args, curried.size, curried.items);
    vector_push(all_args, num_args, args);
    func_t *f = &((runtime_t *) rt)->program->functions[afn.fn_index];
    nar_object_t result;

    if (vector_size(all_args) == f->num_args) {
        result = execute(rt, f, all_args);
    } else if (f->num_args < num_all_args) {
        size_t num_rest = num_all_args - f->num_args;
        nar_object_t *rest = nar_alloc(sizeof(nar_object_t) * num_rest);
        vector_pop(all_args, num_rest, rest);
        result = execute(rt, f, all_args);
        if (nar_object_is_valid(rt, result)) {
            result = nar_apply_func(rt, result, num_rest, rest);
        }
        nar_free(rest);
    } else {
        result = nar_make_closure(rt, afn.fn_index, vector_size(all_args), vector_data(all_args));
    }
    vector_free(all_args);
    return result;
}

void nar_print(nar_runtime_t rt, nar_cstring_t message) {
    ((runtime_t *) rt)->stdout(rt, message);
}

void nar_fail(nar_runtime_t rt, nar_cstring_t message) {
    if (rt == NULL) {
        if (general_last_error != NULL) {
            free(general_last_error);
        }
        general_last_error = strdup(message);
        return;
    }
    runtime_t *r = (runtime_t *) rt;

    size_t len = strlen(message) + 1 + 1;
    vector_t *stack = ((runtime_t *) r)->call_stack;
    for (size_t i = vector_size(stack); i > 0; --i) {
        len += strlen(*(nar_string_t *) vector_at(stack, i - 1)) + 1;
    }

    nar_string_t msg_with_stack = nar_alloc(len);
    strcat(msg_with_stack, message);
    strcat(msg_with_stack, "\n");
    for (size_t i = vector_size(stack); i > 0; --i) {
        strcat(msg_with_stack, *(nar_string_t *) vector_at(stack, i - 1));
        strcat(msg_with_stack, "\n");
    }

    if (r->last_error != NULL) {
        nar_string_t combined = nar_alloc(strlen(r->last_error) + strlen(msg_with_stack) + 21);
        strcpy(combined, r->last_error);
        strcat(combined, "\n----------------\n");
        strcat(combined, message);
        nar_free(r->last_error);
        r->last_error = combined;
    } else {
        r->last_error = string_dup(msg_with_stack);
    }
    printf("%s", msg_with_stack);
    nar_free(msg_with_stack);
}

nar_cstring_t nar_get_error(nar_runtime_t rt) {
    if (rt == NULL) {
        return general_last_error;
    }
    nar_cstring_t err = ((runtime_t *) rt)->last_error;
    if (err) {
        return err;
    }
    return general_last_error;
}

void nar_clear_error(nar_runtime_t rt) {
    free(general_last_error);
    general_last_error = NULL;

    runtime_t *r = (runtime_t *) rt;
    if (r->last_error != NULL) {
        nar_free(r->last_error);
        r->last_error = NULL;
    }
}

void nar_set_metadata(nar_runtime_t rt, nar_cstring_t key, nar_cptr_t value) {
    runtime_t *r = (runtime_t *) rt;
    metadata_item_t *old = (metadata_item_t *) hashmap_set(
            r->metadata,
            &(metadata_item_t) {.name = (nar_string_t) string_dup(key), .value = value});
    if (old != NULL) {
        metadata_item_free(old);
    }
}

nar_cptr_t nar_get_metadata(nar_runtime_t rt, nar_cstring_t key) {
    runtime_t *r = (runtime_t *) rt;
    const metadata_item_t *item = hashmap_get(
            r->metadata, &(metadata_item_t) {.name = (nar_string_t) key});
    return item == NULL ? NULL : item->value;
}

void nar_set_stdout(nar_runtime_t rt, nar_stdout_fn_t stdout) {
    runtime_t *r = (runtime_t *) rt;
    if (stdout == NULL) {
        stdout = default_stdout;
    }
    r->stdout = stdout;
}

nar_int_t library_register(
        runtime_t *rt, nar_string_t name, version_t version, nar_cstring_t libs_path) {
    rt->last_lib_handle = NULL;
    void *init_fn = NULL;
    size_t buf_size = strlen(libs_path) + strlen(name) + 20;
    char path[buf_size];

    void *handle = NULL;
#if defined(NAR_UNIX)
    snprintf(path, buf_size, "%s/lib%s.%d.so", libs_path, name, version);
    handle = dlopen(path, RTLD_LAZY);
#endif
#if defined(NAR_APPLE)
    snprintf(path, buf_size, "%s/lib%s.%d.dylib", libs_path, name, version);
    handle = dlopen(path, RTLD_LAZY);
#endif
#if defined(NAR_WINDOWS)
    snprintf(path, buf_size, "%s\\%s.%d.dll", libs_path, name, version);
    handle = LoadLibrary(path);
#endif

    if (handle == NULL) {
        return 0;
    }

    rt->last_lib_handle = handle;

#if defined(NAR_UNIX) || defined(NAR_APPLE)
    init_fn = dlsym(handle, "init");
#endif
#if defined(NAR_WINDOWS)
    init_fn = GetProcAddress(handle, "init");
#endif

    if (init_fn == NULL) {
        char err[1024];
        snprintf(err, 1024, "failed to find init function in library %s", path);
        nar_fail(rt, err);
        return -1;
    }
    nar_int_t result = ((init_fn_t) init_fn)(rt->package_pointers, rt);
    return result;
}

nar_bool_t nar_register_libs(nar_runtime_t rt, nar_cstring_t libs_path) {
    runtime_t *r = (runtime_t *) rt;
    size_t it = 0;
    void *item;
    while (hashmap_iter(r->program->packages, &it, &item)) {
        packages_item_t *pi = item;
        if (0 != library_register(r, pi->name, pi->version, libs_path)) {
            library_free(r->last_lib_handle);
            r->last_lib_handle = NULL;

            char err[1024];
            snprintf(err, 1024, "failed to register library %s", pi->name);
            nar_fail(rt, err);
            return false;
        }
        vector_push(r->lib_handles, 1, &r->last_lib_handle);
        r->last_lib_handle = NULL;
    }
    return true;
}

nar_string_t frame_string_dup(runtime_t *rt, nar_cstring_t str) {
    size_t sz = (strlen(str) + 1);
    nar_string_t dup = nar_frame_alloc(rt, sz);
    memcpy(dup, str, sz);
    return dup;
}

nar_string_t string_dup(nar_cstring_t str) {
    size_t sz = (strlen(str) + 1);
    nar_string_t dup = nar_alloc(sz);
    memcpy(dup, str, sz);
    return dup;
}
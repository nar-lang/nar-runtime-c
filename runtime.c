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

bool library_register(
        runtime_t *rt, nar_string_t name, version_t version, nar_cstring_t libs_path,
        nar_ptr_t *out_handle) {
    void *init_fn = NULL;
    size_t buf_size = strlen(libs_path) + strlen(name) + 20;
    char path[buf_size];

#if defined(NAR_UNIX)
    snprintf(path, buf_size, "%s/lib%s.%d.so", libs_path, name, version);
    out_handle = dlopen(path, RTLD_LAZY);
#endif
#if defined(NAR_APPLE)
    snprintf(path, buf_size, "%s/lib%s.%d.dylib", libs_path, name, version);
    out_handle = dlopen(path, RTLD_LAZY);
#endif
#if defined(NAR_WINDOWS)
    snprintf(path, buf_size, "%s\\%s.%d.dll", libs_path, name, version);
    out_handle = LoadLibrary(path);
#endif

    if (out_handle == NULL) {
        return true;
    }

#if defined(NAR_UNIX) || defined(NAR_APPLE)
    init_fn = dlsym(out_handle, "init");
#endif
#if defined(NAR_WINDOWS)
    init_fn = GetProcAddress(out_handle, "nar_init");
#endif

    if (init_fn == NULL) {
        char err[1024];
        snprintf(err, 1024, "failed to find init function in library %s", path);
        nar_fail(rt, err);
        return false;
    }
    return 0 != ((init_fn_t) init_fn)(&package_pointers, rt);
}

void library_free(void *handle) {
#if defined(NAR_UNIX) || defined(NAR_APPLE)
    dlclose(handle);
#endif
#if defined(NAR_WINDOWS)
    FreeLibrary(handle);
#endif
}

nar_runtime_t nar_runtime_new(nar_bytecode_t btc, nar_cstring_t libs_path) {
    runtime_t *rt = nar_alloc(sizeof(runtime_t));
    memset(rt, 0, sizeof(runtime_t));

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

    nar_frame_free(rt);

    size_t it = 0;
    void *item;
    while (hashmap_iter(rt->program->packages, &it, &item)) {
        packages_item_t *pi = item;
        nar_ptr_t handle;
        if (!library_register(rt, pi->name, pi->version, libs_path, &handle)) {
            library_free(handle);
            break;
        }
        vector_push(rt->lib_handles, 1, &handle);
    }

    return rt;
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
        nar_free(rt);
    }
}

void nar_register_def(
        nar_runtime_t rt, nar_cstring_t module_name, nar_cstring_t def_name, nar_object_t def) {
    nar_string_t key = nar_alloc((strlen(module_name) + strlen(def_name) + 2));
    strcpy(key, module_name);
    strcat(key, ".");
    strcat(key, def_name);
    hashmap_set(((runtime_t *) rt)->native_defs, &(native_def_item_t) {.name = key, .value = def});
}

nar_object_t nar_apply(
        nar_runtime_t rt, nar_cstring_t name, nar_size_t num_args, const nar_object_t *args) {
    runtime_t *r = (runtime_t *) rt;
    if (vector_size(r->call_stack) > 0) {
        nar_fail(rt, "runtime supports only singe threaded execution");
        return INVALID_OBJECT;
    }
    const exports_item_t *export_item = hashmap_get(r->program->exports,
            &(exports_item_t) {.name = name});
    if (export_item == NULL) {
        nar_fail(rt, "definition not exported in bytecode");
        return INVALID_OBJECT;
    }
    nar_object_t afn = nar_new_closure(rt, export_item->index, 0, NULL);
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
        result = nar_new_closure(rt, afn.fn_index, vector_size(all_args), vector_data(all_args));
    }
    vector_free(all_args);
    return result;
}

void nar_print(__attribute__((unused)) nar_runtime_t rt, nar_cstring_t message) {
    printf("%s\n", message);
}

void nar_fail(nar_runtime_t rt, nar_cstring_t message) {
    runtime_t *r = (runtime_t *) rt;
    if (r->last_error != NULL) {
        nar_free(r->last_error);
    }
    r->last_error = string_dup(message);
#ifndef NAR_UNSAFE
    printf("%s\n", message);
    vector_t *stack = ((runtime_t *) r)->call_stack;
    for (size_t i = vector_size(stack); i > 0; --i) {
        printf("%s\n", *(nar_string_t *) vector_at(stack, i - 1));
    }
    nar_assert(false);
#endif
}

nar_cstring_t nar_get_last_error(nar_runtime_t rt) {
    return ((runtime_t *) rt)->last_error;
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
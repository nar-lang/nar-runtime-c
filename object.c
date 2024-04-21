#include <string.h>
#include <stdio.h>
#include "include/nar.h"
#include "runtime.h"
#include "include/nar-runtime.h"

#define object_get_index(obj) (obj & 0x00FFFFFFFFFFFFFF)
#define UNIT_OBJECT build_object(NAR_OBJECT_KIND_UNIT, 0)

nar_object_t insert(nar_runtime_t rt, nar_object_kind_t kind, void *value) {
    vector_t *arena = ((runtime_t *) rt)->arenas[kind];
    size_t index = vector_size(arena);
    vector_push(arena, 1, value);
    return build_object(kind, index);
}

nar_cstring_t kind_to_string(nar_object_kind_t kind) {
    switch (kind) {
        case NAR_OBJECT_KIND_UNKNOWN:
            return "unknown";
        case NAR_OBJECT_KIND_UNIT:
            return "unit";
        case NAR_OBJECT_KIND_CHAR:
            return "char";
        case NAR_OBJECT_KIND_INT:
            return "int";
        case NAR_OBJECT_KIND_FLOAT:
            return "float";
        case NAR_OBJECT_KIND_STRING:
            return "string";
        case NAR_OBJECT_KIND_RECORD:
            return "record";
        case NAR_OBJECT_KIND_LIST:
            return "list";
        case NAR_OBJECT_KIND_TUPLE:
            return "tuple";
        case NAR_OBJECT_KIND_OPTION:
            return "option";
        case NAR_OBJECT_KIND_FUNCTION:
            return "function";
        case NAR_OBJECT_KIND_NATIVE:
            return "native";
        case NAR_OBJECT_KIND_CLOSURE:
            return "closure";
        case NAR_OBJECT_KIND_PATTERN:
            return "pattern";
        default:
            return "invalid kind";
    }
}

bool check_type(nar_runtime_t rt, nar_object_t obj, nar_object_kind_t kind) {
    if (nar_object_get_kind(rt, obj) != kind) {
        char buf[256];
        sprintf(buf, "expected object kind %d, got %d", kind, nar_object_get_kind(rt, obj));
        nar_fail(rt, buf);
        return false;
    }
    return true;
}

void *find(nar_runtime_t rt, nar_object_kind_t kind, nar_object_t obj) {
    if (!check_type(rt, obj, kind)) {
        return NULL;
    }

    vector_t *arena = ((runtime_t *) rt)->arenas[kind];
    size_t index = object_get_index(obj);
    return vector_at(arena, index);
}

typedef struct {
    nar_cstring_t key;
    nar_object_t value;
} key_value_t;

int key_value_compare(const void *a, const void *b, __attribute__((unused)) void *data) {
    const key_value_t *ia = a;
    const key_value_t *ib = b;
    return strcmp(ia->key, ib->key);
}

uint64_t key_value_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const key_value_t *i = item;
    return hashmap_sip(i->key, strlen(i->key), seed0, seed1);
}

nar_object_kind_t nar_object_get_kind(__attribute__((unused)) nar_runtime_t rt, nar_object_t obj) {
    return (nar_object_kind_t) (obj >> 56) & 0xff;
}

nar_bool_t nar_object_is_valid(__attribute__((unused)) nar_runtime_t rt, nar_object_t obj) {
    return obj != 0;
}

nar_bool_t nar_index_is_valid(__attribute__((unused)) nar_runtime_t rt, nar_object_t obj) {
    return ((obj != 0) && ((obj & NAR_INVALID_INDEX) == 0));
}

nar_object_t nar_make_unit(__attribute__((unused)) nar_runtime_t rt) {
    return UNIT_OBJECT;
}

void nar_to_unit(nar_runtime_t rt, nar_object_t obj) {
    check_type(rt, obj, NAR_OBJECT_KIND_UNIT);
}

nar_object_t nar_make_char(nar_runtime_t rt, nar_char_t value) {
    return insert(rt, NAR_OBJECT_KIND_CHAR, &value);
}

nar_char_t nar_to_char(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_char_t *) find(rt, NAR_OBJECT_KIND_CHAR, obj);
}

nar_object_t nar_make_int(nar_runtime_t rt, nar_int_t value) {
    return insert(rt, NAR_OBJECT_KIND_INT, &value);
}

nar_int_t nar_to_int(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_int_t *) find(rt, NAR_OBJECT_KIND_INT, obj);
}

nar_object_t nar_make_float(nar_runtime_t rt, nar_float_t value) {
    return insert(rt, NAR_OBJECT_KIND_FLOAT, &value);
}

nar_float_t nar_to_float(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_float_t *) find(rt, NAR_OBJECT_KIND_FLOAT, obj);
}

nar_object_t nar_make_string(nar_runtime_t rt, nar_cstring_t value) {
    runtime_t *r = (runtime_t *) rt;
    const string_hast_t *hash = hashmap_get(r->string_hashes, &(string_hast_t) {.string = value});

    if (hash == NULL) {
        size_t index = vector_size(r->arenas[NAR_OBJECT_KIND_STRING]);
        nar_string_t dup = frame_string_dup(rt, value);
        vector_push(r->arenas[NAR_OBJECT_KIND_STRING], 1, &dup);
        hash = &(string_hast_t) {
                .string = dup,
                .index = build_object(NAR_OBJECT_KIND_STRING, index)
        };
        hashmap_set(r->string_hashes, hash);
    }
    return hash->index;
}

nar_cstring_t nar_to_string(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_string_t *) find(rt, NAR_OBJECT_KIND_STRING, obj);
}

nar_object_t nar_make_record(
        nar_runtime_t rt, nar_size_t size, const nar_string_t *keys, const nar_object_t *values) {
    nar_object_t prev = build_object(NAR_OBJECT_KIND_RECORD, NAR_INVALID_INDEX);
    for (size_t i = 0; i < size; i++) {
        prev = nar_make_record_field_obj(rt, prev, nar_make_string(rt, keys[i]), values[i]);
    }
    return prev;
}

nar_object_t nar_make_record_field(
        nar_runtime_t rt, nar_object_t record, nar_cstring_t key, nar_object_t value) {
    return nar_make_record_field_obj(rt, record, nar_make_string(rt, key), value);
}

nar_object_t nar_make_record_field_obj(
        nar_runtime_t rt, nar_object_t record, nar_object_t key, nar_object_t value) {
    if (!check_type(rt, record, NAR_OBJECT_KIND_RECORD)) {
        return NAR_INVALID_OBJECT;
    }

    return insert(rt, NAR_OBJECT_KIND_RECORD,
            &(nar_record_item_t) {.key=key, .value = value, .parent = record});
}

nar_object_t nar_make_record_raw(nar_runtime_t rt, size_t num_fields, const nar_object_t *stack) {
    num_fields *= 2;
    nar_object_t prev = build_object(NAR_OBJECT_KIND_RECORD, NAR_INVALID_INDEX);
    for (size_t i = 1; i < num_fields; i += 2) {
        prev = nar_make_record_field_obj(rt, prev, stack[i], stack[i - 1]);
    }
    return prev;
}

nar_record_t nar_to_record(nar_runtime_t rt, nar_object_t obj) {
    if (!nar_index_is_valid(rt, obj)) {
        return (nar_record_t) {0};
    }

    hashmap_t *map = hashmap_new(sizeof(key_value_t), 0, 0, 0,
            &key_value_hash, &key_value_compare, NULL, NULL);
    while (nar_index_is_valid(rt, obj)) {
        nar_record_item_t f = nar_to_record_item(rt, obj);
        nar_cstring_t key = nar_to_string(rt, f.key);
        hashmap_set(map, &(key_value_t) {.key = key, .value = f.value});
        obj = f.parent;
    }

    nar_record_t record;
    size_t sz = hashmap_count(map);
    if (sz == 0) {
        record = (nar_record_t) {0};
    } else {
        record = (nar_record_t) {
                .size = sz,
                .keys = nar_frame_alloc(rt, sz * sizeof(nar_string_t)),
                .values = nar_frame_alloc(rt, sz * sizeof(nar_object_t))
        };

        size_t index = 0;
        size_t it = 0;
        void *item;
        while (hashmap_iter(map, &it, &item)) {
            const key_value_t *kv = item;
            record.keys[index] = kv->key;
            record.values[index] = kv->value;
            index++;
        }
    }
    hashmap_free(map);
    return record;
}

nar_object_t nar_to_record_field(nar_runtime_t rt, nar_object_t obj, nar_cstring_t key) {
    if (!check_type(rt, obj, NAR_OBJECT_KIND_RECORD)) {
        return NAR_INVALID_OBJECT;
    }

    while (nar_index_is_valid(rt, obj)) {
        nar_record_item_t f = nar_to_record_item(rt, obj);
        nar_cstring_t it_key = nar_to_string(rt, f.key);
        if (strcmp(it_key, key) == 0) {
            return f.value;
        }
        obj = f.parent;
    }
    return NAR_INVALID_OBJECT;
}

nar_record_item_t nar_to_record_item(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_record_item_t *) find(rt, NAR_OBJECT_KIND_RECORD, obj);
}

nar_object_t nar_make_list_cons(nar_runtime_t rt, nar_object_t head, nar_object_t tail) {
    return insert(rt, NAR_OBJECT_KIND_LIST,
            &(nar_list_item_t) {.value = head, .next = tail});
}

nar_object_t nar_make_list(nar_runtime_t rt, nar_size_t size, const nar_object_t *items) {
    nar_object_t first = build_object(NAR_OBJECT_KIND_LIST, NAR_INVALID_INDEX);
    for (size_t i = size; i > 0; i--) {
        first = nar_make_list_cons(rt, items[i - 1], first);
    }
    return first;
}

nar_list_t nar_to_list(nar_runtime_t rt, nar_object_t obj) {
    if (!check_type(rt, obj, NAR_OBJECT_KIND_LIST)) {
        return (nar_list_t) {0};
    }

    vector_t *vec = rvector_new(sizeof(nar_object_t), 0);

    while (nar_index_is_valid(rt, obj)) {
        nar_list_item_t item = nar_to_list_item(rt, obj);
        vector_push(vec, 1, &item.value);
        obj = item.next;
    }

    nar_list_t list;
    size_t sz = vector_size(vec);
    if (sz == 0) {
        list = (nar_list_t) {0};
    } else {
        size_t mem_size = sz * sizeof(nar_object_t);
        list = (nar_list_t) {.size = sz, .items = nar_frame_alloc(rt, mem_size)};
        vector_pop(vec, sz, list.items);
    }
    vector_free(vec);
    return list;
}

nar_list_item_t nar_to_list_item(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_list_item_t *) find(rt, NAR_OBJECT_KIND_LIST, obj);
}

nar_object_t nar_make_tuple_item(nar_runtime_t rt, nar_object_t value, nar_object_t next) {
    return insert(rt, NAR_OBJECT_KIND_TUPLE,
            &(nar_tuple_item_t) {.value = value, .next = next});
}

nar_object_t nar_make_tuple(nar_runtime_t rt, nar_size_t size, const nar_object_t *items) {
    nar_object_t first = build_object(NAR_OBJECT_KIND_TUPLE, NAR_INVALID_INDEX);
    for (size_t i = size; i > 0; i--) {
        first = nar_make_tuple_item(rt, items[i - 1], first);
    }
    return first;
}

nar_tuple_t nar_to_tuple(nar_runtime_t rt, nar_object_t obj) {
    if (!check_type(rt, obj, NAR_OBJECT_KIND_TUPLE)) {
        return (nar_tuple_t) {0};
    }

    vector_t *vec = rvector_new(sizeof(nar_object_t), 0);

    while (nar_index_is_valid(rt, obj)) {
        nar_tuple_item_t item = nar_to_tuple_item(rt, obj);
        vector_push(vec, 1, &item.value);
        obj = item.next;
    }

    nar_tuple_t tuple;
    size_t sz = vector_size(vec);
    if (sz == 0) {
        tuple = (nar_tuple_t) {0};
    } else {
        size_t mem_size = sz * sizeof(nar_object_t);
        tuple = (nar_tuple_t) {.size = sz, .values = nar_frame_alloc(rt, mem_size)};
        vector_pop(vec, sz, tuple.values);
    }
    vector_free(vec);
    return tuple;
}

nar_tuple_item_t nar_to_tuple_item(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_tuple_item_t *) find(rt, NAR_OBJECT_KIND_TUPLE, obj);
}

nar_object_t nar_make_option_with_list(
        nar_runtime_t rt, nar_cstring_t name, nar_object_t item_list) {
    return insert(rt, NAR_OBJECT_KIND_OPTION,
            &(nar_option_item_t) {
                    .name = nar_make_string(rt, name),
                    .values = item_list
            });
}

nar_object_t nar_make_option(
        nar_runtime_t rt, nar_cstring_t name, nar_size_t size, const nar_object_t *items) {
    return nar_make_option_with_list(rt, name, nar_make_list(rt, size, items));
}

nar_option_t nar_to_option(nar_runtime_t rt, nar_object_t obj) {
    nar_option_item_t opt = nar_to_option_item(rt, obj);
    nar_list_t list = nar_to_list(rt, opt.values);
    return (nar_option_t) {
            .name = nar_to_string(rt, opt.name),
            .size = list.size,
            .values = list.items
    };
}

nar_option_item_t nar_to_option_item(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_option_item_t *) find(rt, NAR_OBJECT_KIND_OPTION, obj);
}

nar_object_t nar_make_bool(__attribute__((unused)) nar_runtime_t rt, nar_bool_t value) {
    if (value) {
        return build_object(NAR_OBJECT_KIND_OPTION, 1);
    }
    return build_object(NAR_OBJECT_KIND_OPTION, 0);
}

nar_bool_t nar_to_bool(nar_runtime_t rt, nar_object_t obj) {
    if (!check_type(rt, obj, NAR_OBJECT_KIND_OPTION)) {
        return 0;
    }

    if (object_get_index(obj) == 0) {
        return nar_false;
    }
    if (object_get_index(obj) == 1) {
        return nar_true;
    }
    nar_option_t opt = nar_to_option(rt, obj);
    if (strcmp(opt.name, OPTION_NAME_TRUE) == 0) {
        return nar_true;
    }
    if (strcmp(opt.name, OPTION_NAME_FALSE) == 0) {
        return nar_false;
    }
    nar_fail(rt, "expected Nar.Base.Basics.Bool option");
    return 0;
}

nar_object_t nar_make_func(nar_runtime_t rt, nar_ptr_t fn, nar_size_t arity) {
    if (arity > 8) {
        nar_fail(rt, "maximum function arity is 8");
        return NAR_INVALID_OBJECT;
    }
    return insert(rt, NAR_OBJECT_KIND_FUNCTION, &(nar_func_t) {.ptr = fn, .arity = arity});
}

nar_func_t nar_to_func(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_func_t *) find(rt, NAR_OBJECT_KIND_FUNCTION, obj);
}

nar_object_t nar_make_native(nar_runtime_t rt, nar_ptr_t ptr, nar_cmp_native_fn_t cmp) {
    return insert(rt, NAR_OBJECT_KIND_NATIVE, &(nar_native_t) {.ptr = ptr, .cmp = cmp});
}

nar_native_t nar_to_native(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_native_t *) find(rt, NAR_OBJECT_KIND_NATIVE, obj);
}

nar_object_t nar_make_closure_with_list(
        nar_runtime_t rt, size_t fn_index, nar_object_t curried_list) {
    return insert(rt, NAR_OBJECT_KIND_CLOSURE,
            &(nar_closure_t) {.fn_index = fn_index, .curried = curried_list});
}

nar_object_t nar_make_closure(
        nar_runtime_t rt, size_t fn_index, size_t num_args, const nar_object_t *args) {
    return nar_make_closure_with_list(rt, fn_index, nar_make_list(rt, num_args, args));
}

nar_closure_t nar_to_closure(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_closure_t *) find(rt, NAR_OBJECT_KIND_CLOSURE, obj);
}

nar_object_t nar_make_pattern_with_list(
        nar_runtime_t rt, pattern_kind_t kind,
        nar_cstring_t name, nar_object_t value_list) {
    return insert(rt, NAR_OBJECT_KIND_PATTERN,
            &(nar_pattern_t) {
                    .kind = kind,
                    .name = name,
                    .values = value_list,
            });
}

nar_object_t nar_make_pattern(
        nar_runtime_t rt, pattern_kind_t kind,
        nar_cstring_t name, size_t num_items, nar_object_t *items) {
    return nar_make_pattern_with_list(rt, kind, name, nar_make_list(rt, num_items, items));
}

nar_pattern_t nar_to_pattern(nar_runtime_t rt, nar_object_t pattern) {
    return *(nar_pattern_t *) find(rt, NAR_OBJECT_KIND_PATTERN, pattern);
}

void serialize_object(runtime_t *rt, nar_object_t obj, vector_t *mem) {
    nar_byte_t kind = (nar_byte_t) nar_object_get_kind(rt, obj);
    vector_push(mem, sizeof(nar_byte_t), &kind);
    switch (kind) {
        case NAR_OBJECT_KIND_UNKNOWN: {
            break;
        }
        case NAR_OBJECT_KIND_UNIT: {
            break;
        }
        case NAR_OBJECT_KIND_CHAR: {
            nar_char_t value = nar_to_char(rt, obj);
            vector_push(mem, sizeof(nar_char_t), &value);
            break;
        }
        case NAR_OBJECT_KIND_INT: {
            nar_int_t value = nar_to_int(rt, obj);
            vector_push(mem, sizeof(nar_int_t), &value);
            break;
        }
        case NAR_OBJECT_KIND_FLOAT: {
            nar_float_t value = nar_to_float(rt, obj);
            vector_push(mem, sizeof(nar_float_t), &value);
            break;
        }
        case NAR_OBJECT_KIND_STRING: {
            nar_cstring_t str = nar_to_string(rt, obj);
            nar_size_t len = strlen(str) + 1;
            vector_push(mem, sizeof(nar_size_t), &len);
            vector_push(mem, len, str);
            break;
        }
        case NAR_OBJECT_KIND_RECORD: {
            nar_bool_t has_next = nar_index_is_valid(rt, obj);
            vector_push(mem, sizeof(nar_bool_t), &has_next);
            if (has_next) {
                nar_record_item_t item = nar_to_record_item(rt, obj);
                serialize_object(rt, item.key, mem);
                serialize_object(rt, item.value, mem);
                serialize_object(rt, item.parent, mem);
            }
            break;
        }
        case NAR_OBJECT_KIND_LIST: {
            //TODO: unroll recursion for lists
            nar_bool_t has_next = nar_index_is_valid(rt, obj);
            vector_push(mem, sizeof(nar_bool_t), &has_next);
            if (has_next) {
                nar_list_item_t item = nar_to_list_item(rt, obj);
                serialize_object(rt, item.value, mem);
                serialize_object(rt, item.next, mem);
            }
            break;
        }
        case NAR_OBJECT_KIND_TUPLE: {
            nar_bool_t has_next = nar_index_is_valid(rt, obj);
            vector_push(mem, sizeof(nar_bool_t), &has_next);
            if (has_next) {
                nar_tuple_item_t item = nar_to_tuple_item(rt, obj);
                serialize_object(rt, item.value, mem);
                serialize_object(rt, item.next, mem);
            }
            break;
        }
        case NAR_OBJECT_KIND_OPTION: {
            nar_option_item_t item = nar_to_option_item(rt, obj);
            serialize_object(rt, item.name, mem);
            serialize_object(rt, item.values, mem);
            break;
        }
        case NAR_OBJECT_KIND_FUNCTION: {
            nar_func_t item = nar_to_func(rt, obj);
            vector_push(mem, sizeof(nar_cptr_t), &item.ptr);
            vector_push(mem, sizeof(nar_size_t), &item.arity);
            break;
        }
        case NAR_OBJECT_KIND_NATIVE: {
            nar_native_t item = nar_to_native(rt, obj);
            vector_push(mem, sizeof(nar_cptr_t), &item.ptr);
            vector_push(mem, sizeof(nar_cmp_native_fn_t), &item.cmp);
            break;
        }
        case NAR_OBJECT_KIND_CLOSURE: {
            nar_closure_t item = nar_to_closure(rt, obj);
            vector_push(mem, sizeof(nar_size_t), &item.fn_index);
            serialize_object(rt, item.curried, mem);
            break;
        }
        case NAR_OBJECT_KIND_PATTERN: {
            nar_pattern_t item = nar_to_pattern(rt, obj);
            vector_push(mem, sizeof(pattern_kind_t), &item.kind);
            serialize_object(rt, nar_make_string(rt, item.name), mem);
            serialize_object(rt, item.values, mem);
            break;
        }
        default:
            nar_fail(rt, "unknown object kind");
    }
}

void *nar_new_serialized_object(nar_runtime_t rt, nar_object_t obj) {
    runtime_t *r = (runtime_t *) rt;
    vector_t *mem = rvector_new(sizeof(nar_byte_t), 0);
    serialize_object(r, obj, mem);

    void *data = mem->data;
    mem->data = NULL;
    vector_free(mem);
    return data;
}

nar_object_t deserialize_object(runtime_t *rt, nar_byte_t **mem) {
    nar_byte_t kind = *(nar_byte_t *) (*mem);
    (*mem) += sizeof(nar_byte_t);

    switch ((nar_object_kind_t) kind) {
        case NAR_OBJECT_KIND_UNKNOWN: {
            return NAR_INVALID_OBJECT;
        }
        case NAR_OBJECT_KIND_UNIT: {
            return UNIT_OBJECT;
        }
        case NAR_OBJECT_KIND_CHAR: {
            nar_char_t value = *(nar_char_t *) (*mem);
            (*mem) += sizeof(nar_char_t);
            return nar_make_char(rt, value);
        }
        case NAR_OBJECT_KIND_INT: {
            nar_int_t value = *(nar_int_t *) (*mem);
            (*mem) += sizeof(nar_int_t);
            return nar_make_int(rt, value);
        }
        case NAR_OBJECT_KIND_FLOAT: {
            nar_float_t value = *(nar_float_t *) (*mem);
            (*mem) += sizeof(nar_float_t);
            return nar_make_float(rt, value);
        }
        case NAR_OBJECT_KIND_STRING: {
            nar_size_t len = *(nar_size_t *) (*mem);
            (*mem) += sizeof(nar_size_t);
            nar_cstring_t str = (nar_cstring_t) (*mem);
            (*mem) += len;
            return nar_make_string(rt, str);
        }
        case NAR_OBJECT_KIND_RECORD: {
            nar_bool_t has_next = *(nar_bool_t *) (*mem);
            (*mem) += sizeof(nar_bool_t);
            if (!has_next) {
                return build_object(NAR_OBJECT_KIND_RECORD, NAR_INVALID_INDEX);
            }
            nar_object_t key = deserialize_object(rt, mem);
            nar_object_t value = deserialize_object(rt, mem);
            nar_object_t parent = deserialize_object(rt, mem);
            return nar_make_record_field_obj(rt, parent, key, value);
        }
        case NAR_OBJECT_KIND_LIST: {
            //TODO: unroll recursion for lists
            nar_bool_t has_next = *(nar_bool_t *) (*mem);
            (*mem) += sizeof(nar_bool_t);
            if (!has_next) {
                return build_object(NAR_OBJECT_KIND_LIST, NAR_INVALID_INDEX);
            }
            return nar_make_list_cons(rt, deserialize_object(rt, mem), deserialize_object(rt, mem));
        }
        case NAR_OBJECT_KIND_TUPLE: {
            nar_bool_t has_next = *(nar_bool_t *) (*mem);
            (*mem) += sizeof(nar_bool_t);
            if (!has_next) {
                return build_object(NAR_OBJECT_KIND_TUPLE, NAR_INVALID_INDEX);
            }
            return nar_make_tuple_item(
                    rt, deserialize_object(rt, mem), deserialize_object(rt, mem));
        }
        case NAR_OBJECT_KIND_OPTION: {
            nar_object_t name = deserialize_object(rt, mem);
            nar_object_t values = deserialize_object(rt, mem);
            return nar_make_option_with_list(rt, nar_to_string(rt, name), values);
        }
        case NAR_OBJECT_KIND_FUNCTION: {
            nar_cptr_t ptr = *(nar_cptr_t *) (*mem);
            (*mem) += sizeof(nar_cptr_t);
            nar_size_t arity = *(nar_size_t *) (*mem);
            (*mem) += sizeof(nar_size_t);
            return nar_make_func(rt, ptr, arity);
        }
        case NAR_OBJECT_KIND_NATIVE: {
            nar_cptr_t ptr = *(nar_cptr_t *) (*mem);
            (*mem) += sizeof(nar_cptr_t);
            nar_cmp_native_fn_t cmp = *(nar_cmp_native_fn_t *) (*mem);
            (*mem) += sizeof(nar_cmp_native_fn_t);
            return nar_make_native(rt, ptr, cmp);
        }
        case NAR_OBJECT_KIND_CLOSURE: {
            nar_size_t fn_index = *(nar_size_t *) (*mem);
            (*mem) += sizeof(nar_size_t);
            nar_object_t curried = deserialize_object(rt, mem);
            return nar_make_closure_with_list(rt, fn_index, curried);
        }
        case NAR_OBJECT_KIND_PATTERN: {
            pattern_kind_t pattern_kind = *(pattern_kind_t *) (*mem);
            (*mem) += sizeof(pattern_kind_t);
            nar_object_t name = deserialize_object(rt, mem);
            nar_object_t values = deserialize_object(rt, mem);
            return nar_make_pattern_with_list(rt, pattern_kind, nar_to_string(rt, name), values);
        }
        default:
            nar_fail(rt, "unknown object kind");
            return NAR_INVALID_OBJECT;
    }
}

nar_object_t nar_deserialize_object(nar_runtime_t rt, void *mem) {
    return deserialize_object((runtime_t *) rt, (nar_byte_t **) &mem);
}
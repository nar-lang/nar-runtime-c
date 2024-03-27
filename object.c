#include <string.h>
#include "include/nar.h"
#include "runtime.h"
#include "include/nar-runtime.h"

#define object_get_index(obj) (obj & 0x00FFFFFFFFFFFFFF)
#define UNIT_OBJECT new_object(NAR_OBJECT_KIND_UNIT, 0)

nar_object_t insert(nar_runtime_t rt, nar_object_kind_t kind, void *value) {
    vector_t *arena = ((runtime_t *) rt)->arenas[kind];
    size_t index = vector_size(arena);
    vector_push(arena, 1, value);
    return new_object(kind, index);
}

void *find(nar_runtime_t rt, nar_object_kind_t kind, nar_object_t obj) {
    nar_assert(nar_object_get_kind(rt, obj) == (kind));
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

nar_object_t nar_new_unit(__attribute__((unused)) nar_runtime_t rt) {
    return UNIT_OBJECT;
}

void nar_to_unit(nar_runtime_t rt, nar_object_t obj) {
    nar_assert(nar_object_get_kind(rt, obj) == NAR_OBJECT_KIND_UNIT);
}

nar_object_t nar_new_char(nar_runtime_t rt, nar_char_t value) {
    return insert(rt, NAR_OBJECT_KIND_CHAR, &value);
}

nar_char_t nar_to_char(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_char_t *) find(rt, NAR_OBJECT_KIND_CHAR, obj);
}

nar_object_t nar_new_int(nar_runtime_t rt, nar_int_t value) {
    return insert(rt, NAR_OBJECT_KIND_INT, &value);
}

nar_int_t nar_to_int(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_int_t *) find(rt, NAR_OBJECT_KIND_INT, obj);
}

nar_object_t nar_new_float(nar_runtime_t rt, nar_float_t value) {
    return insert(rt, NAR_OBJECT_KIND_FLOAT, &value);
}

nar_float_t nar_to_float(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_float_t *) find(rt, NAR_OBJECT_KIND_FLOAT, obj);
}

nar_object_t nar_new_string(nar_runtime_t rt, nar_cstring_t value) {
    runtime_t *r = (runtime_t *) rt;
    const string_hast_t *hash = hashmap_get(r->string_hashes, &(string_hast_t) {.string = value});

    if (hash == NULL) {
        size_t index = vector_size(r->arenas[NAR_OBJECT_KIND_STRING]);
        nar_string_t dup = frame_string_dup(rt, value);
        vector_push(r->arenas[NAR_OBJECT_KIND_STRING], 1, &dup);
        hash = &(string_hast_t) {
                .string = dup,
                .index = new_object(NAR_OBJECT_KIND_STRING, index)
        };
        hashmap_set(r->string_hashes, hash);
    }
    return hash->index;
}

nar_cstring_t nar_to_string(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_string_t *) find(rt, NAR_OBJECT_KIND_STRING, obj);
}

nar_object_t nar_new_record(
        nar_runtime_t rt, nar_size_t size, const nar_string_t *keys, const nar_object_t *values) {
    nar_object_t prev = new_object(NAR_OBJECT_KIND_RECORD, INVALID_INDEX);
    for (size_t i = 0; i < size; i++) {
        prev = nar_new_record_field_obj(rt, prev, nar_new_string(rt, keys[i]), values[i]);
    }
    return prev;
}

nar_object_t nar_new_record_field(
        nar_runtime_t rt, nar_object_t record, nar_cstring_t key, nar_object_t value) {
    return nar_new_record_field_obj(rt, record, nar_new_string(rt, key), value);
}

nar_object_t nar_new_record_field_obj(
        nar_runtime_t rt, nar_object_t record, nar_object_t key, nar_object_t value) {
    assert(nar_object_get_kind(rt, key) == NAR_OBJECT_KIND_STRING);
    return insert(rt, NAR_OBJECT_KIND_RECORD,
            &(nar_record_item_t) {.key=key, .value = value, .parent = record});
}

nar_object_t nar_new_record_raw(nar_runtime_t rt, size_t num_fields, const nar_object_t *stack) {
    num_fields *= 2;
    nar_object_t prev = new_object(NAR_OBJECT_KIND_RECORD, INVALID_INDEX);
    for (size_t i = 1; i < num_fields; i += 2) {
        prev = nar_new_record_field_obj(rt, prev, stack[i], stack[i - 1]);
    }
    return prev;
}

nar_record_t nar_to_record(nar_runtime_t rt, nar_object_t obj) {
    nar_assert(nar_object_get_kind(rt, obj) == NAR_OBJECT_KIND_RECORD);
    hashmap_t *map = hashmap_new(sizeof(key_value_t), 0, 0, 0,
            &key_value_hash, &key_value_compare, NULL, NULL);
    while (index_is_valid(obj)) {
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
    nar_assert(nar_object_get_kind(rt, obj) == NAR_OBJECT_KIND_RECORD);
    while (index_is_valid(obj)) {
        nar_record_item_t f = nar_to_record_item(rt, obj);
        nar_cstring_t it_key = nar_to_string(rt, f.key);
        if (strcmp(it_key, key) == 0) {
            return f.value;
        }
        obj = f.parent;
    }
    return INVALID_OBJECT;
}

nar_record_item_t nar_to_record_item(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_record_item_t *) find(rt, NAR_OBJECT_KIND_RECORD, obj);
}

nar_object_t nar_new_list_cons(nar_runtime_t rt, nar_object_t head, nar_object_t tail) {
    return insert(rt, NAR_OBJECT_KIND_LIST,
            &(nar_list_item_t) {.value = head, .next = tail});
}

nar_object_t nar_new_list(nar_runtime_t rt, nar_size_t size, const nar_object_t *items) {
    nar_object_t first = new_object(NAR_OBJECT_KIND_LIST, INVALID_INDEX);
    for (size_t i = size; i > 0; i--) {
        first = nar_new_list_cons(rt, items[i - 1], first);
    }
    return first;
}

nar_list_t nar_to_list(nar_runtime_t rt, nar_object_t obj) {
    nar_assert(nar_object_get_kind(rt, obj) == NAR_OBJECT_KIND_LIST);
    vector_t *vec = rvector_new(sizeof(nar_object_t), 0);

    while (index_is_valid(obj)) {
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

nar_object_t nar_new_tuple_item(nar_runtime_t rt, nar_object_t value, nar_object_t next) {
    return insert(rt, NAR_OBJECT_KIND_TUPLE,
            &(nar_tuple_item_t) {.value = value, .next = next});
}

nar_object_t nar_new_tuple(nar_runtime_t rt, nar_size_t size, const nar_object_t *items) {
    nar_object_t first = new_object(NAR_OBJECT_KIND_TUPLE, INVALID_INDEX);
    for (size_t i = size; i > 0; i--) {
        first = nar_new_tuple_item(rt, items[i - 1], first);
    }
    return first;
}

nar_tuple_t nar_to_tuple(nar_runtime_t rt, nar_object_t obj) {
    nar_assert(nar_object_get_kind(rt, obj) == NAR_OBJECT_KIND_TUPLE);
    vector_t *vec = rvector_new(sizeof(nar_object_t), 0);

    while (index_is_valid(obj)) {
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

nar_object_t nar_new_option(
        nar_runtime_t rt, nar_cstring_t name, nar_size_t size, const nar_object_t *items) {
    return insert(rt, NAR_OBJECT_KIND_OPTION,
            &(nar_option_item_t) {
                    .name = nar_new_string(rt, name),
                    .values = nar_new_list(rt, size, items)
            });
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

nar_object_t nar_new_bool(__attribute__((unused)) nar_runtime_t rt, nar_bool_t value) {
    if (value) {
        return new_object(NAR_OBJECT_KIND_OPTION, 1);
    }
    return new_object(NAR_OBJECT_KIND_OPTION, 0);
}

nar_bool_t nar_to_bool(nar_runtime_t rt, nar_object_t obj) {
    nar_assert(nar_object_get_kind(rt, obj) == NAR_OBJECT_KIND_OPTION);
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
    nar_assert(!"expected boolean type");
}

nar_object_t nar_new_func(nar_runtime_t rt, nar_ptr_t fn, nar_size_t arity) {
    if (arity > 8) {
        nar_fail(rt, "maximum function arity is 8");
        return INVALID_OBJECT;
    }
    return insert(rt, NAR_OBJECT_KIND_FUNCTION, &(nar_func_t) {.ptr = fn, .arity = arity});
}

nar_func_t nar_to_func(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_func_t *) find(rt, NAR_OBJECT_KIND_FUNCTION, obj);
}

nar_object_t nar_new_native(nar_runtime_t rt, nar_ptr_t ptr, nar_cmp_native_fn_t cmp) {
    return insert(rt, NAR_OBJECT_KIND_NATIVE, &(nar_native_t) {.ptr = ptr, .cmp = cmp});
}

nar_native_t nar_to_native(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_native_t *) find(rt, NAR_OBJECT_KIND_NATIVE, obj);
}

nar_object_t nar_new_closure(
        nar_runtime_t rt, size_t fn_index, size_t num_args, const nar_object_t *args) {
    return insert(rt, NAR_OBJECT_KIND_CLOSURE,
            &(nar_closure_t) {.fn_index = fn_index, .curried = nar_new_list(rt, num_args, args)});
}

nar_closure_t nar_to_closure(nar_runtime_t rt, nar_object_t obj) {
    return *(nar_closure_t *) find(rt, NAR_OBJECT_KIND_CLOSURE, obj);
}

nar_object_t nar_new_pattern(
        nar_runtime_t rt, pattern_kind_t kind,
        nar_cstring_t name, size_t num_items, nar_object_t *items) {
    return insert(rt, NAR_OBJECT_KIND_PATTERN,
            &(nar_pattern_t) {
                    .kind = kind,
                    .name = name,
                    .values = nar_new_list(rt, num_items, items),
            });
}

nar_pattern_t nar_to_pattern(nar_runtime_t rt, nar_object_t pattern) {
    return *(nar_pattern_t *) find(rt, NAR_OBJECT_KIND_PATTERN, pattern);
}

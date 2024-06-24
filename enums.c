#ifndef ENUMS_H
#define ENUMS_H
#include "include/nar.h"
#include "include/nar-runtime.h"
#include "include/hashmap/hashmap.h"
#include <string.h>

typedef struct {
    nar_cstring_t name;
    nar_int_t value;
} to_enum_t;

typedef struct {
    nar_int_t value;
    nar_cstring_t name;
    nar_cstring_t type;
} from_enum_t;

struct hashmap *map_to_enum = NULL;
struct hashmap *map_from_enum = NULL;

nar_bool_t nar_to_enum_option_s(nar_runtime_t rt, nar_object_t opt, nar_int_t *value) {
    const to_enum_t *e = hashmap_get(map_to_enum,
            &(to_enum_t) {.name= nar_to_option(rt, opt).name});
    if (e) {
        *value = e->value;
        return true;
    } else {
        *value = 0;
        return false;
    }
}

nar_int_t nar_to_enum_option(nar_runtime_t rt, nar_object_t opt) {
    nar_int_t value;
    if (nar_to_enum_option_s(rt, opt, &value)) {
        return value;
    } else {
        return 0;
    }
}

nar_int_t nar_to_enum_option_flags(nar_runtime_t rt, nar_object_t list) {
    nar_object_t it = list;
    nar_int_t flags = 0;
    while (nar_index_is_valid(rt, it)) {
        nar_list_item_t item = nar_to_list_item(rt, it);
        flags |= nar_to_enum_option(rt, item.value);
        it = item.next;
    }
    return flags;
}

nar_object_t nar_make_enum_option(
        nar_runtime_t rt, nar_cstring_t type, nar_int_t value, nar_size_t size,
        const nar_object_t *items) {
    const to_enum_t *e = hashmap_get(map_from_enum,
            &(from_enum_t) {.value=value, .type=type});
    if (e) {
        return nar_make_option(rt, e->name, size, items);
    } else {
        return NAR_INVALID_OBJECT;
    }
}

nar_object_t nar_make_enum_option_flags(
        nar_runtime_t rt, nar_cstring_t type, nar_int_t flags) {
    nar_object_t list = nar_make_list(rt, 0, NULL);
    for (int i = 0; flags; i++) {
        nar_int_t flag = 1 << i;
        if (flags & flag) {
            nar_object_t item = nar_make_enum_option(rt, type, flag, 0, NULL);
            list = nar_make_list_cons(rt, list, item);
            flags &= ~flag;
        }
    }
    return list;
}

static int to_enum_compare(const void *a, const void *b, __attribute__((unused)) void *data) {
    const to_enum_t *ia = a;
    const to_enum_t *ib = b;
    return strcmp(ia->name, ib->name);
}

static uint64_t to_enum_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const to_enum_t *i = item;
    return hashmap_sip(i->name, strlen(i->name), seed0, seed1);
}

static int from_enum_compare(const void *a, const void *b, __attribute__((unused)) void *data) {
    const from_enum_t *ia = a;
    const from_enum_t *ib = b;
    return ia->value - ib->value ? -1 : ia->value > ib->value ? 1 : 0;
}

static uint64_t from_enum_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const from_enum_t *i = item;
    return hashmap_sip(i->type, strlen(i->type), seed0, seed1) ^ i->value;
}

void nar_enum_def(nar_cstring_t type, nar_cstring_t option, nar_int_t value) {
    if (NULL == map_to_enum) {
        map_to_enum = hashmap_new(
                sizeof(to_enum_t), 0, 0, 0, &to_enum_hash, &to_enum_compare, NULL, NULL);
    }
    if (NULL == map_from_enum) {
        map_from_enum = hashmap_new(
                sizeof(from_enum_t), 0, 0, 0, &from_enum_hash, &from_enum_compare, NULL, NULL);
    }
    hashmap_set(map_to_enum, &(to_enum_t) {.name=option, .value=value});
    hashmap_set(map_from_enum, &(from_enum_t) {.name=option, .value=value, .type=type});
}

#endif //ENUMS_H

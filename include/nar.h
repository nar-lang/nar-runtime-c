#ifndef NAR_H
#define NAR_H

#include <stdlib.h>
#include "fchar.h"

#if __DBL_DIG__ != 15 || __DBL_MANT_DIG__ != 53 || __DBL_MIN_EXP__ != -1021 || __DBL_MAX_EXP__ != 1024
#error "IEEE 754 floating point support is required"
#endif

#ifdef NAR_UNSAFE
#define nar_assert(expr)
#else
#include <assert.h>
#define nar_assert(expr) assert(expr)
#endif

typedef void *nar_runtime_t;

typedef size_t nar_size_t;

typedef uint8_t nar_bool_t;
#define nar_false 0
#define nar_true 1
typedef uint8_t nar_byte_t;
typedef fchar_t nar_char_t;
typedef int64_t nar_int_t;
typedef uint64_t nar_uint_t;
typedef double nar_float_t;
typedef char *nar_string_t; //in utf-8
typedef const char *nar_cstring_t; //in utf-8
typedef void *nar_ptr_t;
typedef void *nar_cptr_t;
typedef nar_int_t (*nar_cmp_native_fn_t)(nar_runtime_t rt, nar_cptr_t a, nar_cptr_t b);
typedef void (*nar_stdout_fn_t)(nar_runtime_t rt, nar_cstring_t message);
typedef uint64_t nar_object_t;

typedef struct {
    nar_cptr_t ptr;
    nar_cmp_native_fn_t cmp;
} nar_native_t;

typedef struct {
    nar_size_t size;
    nar_cstring_t *keys;
    nar_object_t *values;
} nar_record_t;

typedef struct {
    nar_size_t size;
    nar_object_t *items;
} nar_list_t;

typedef struct {
    nar_size_t size;
    nar_object_t *values;
} nar_tuple_t;

typedef struct {
    nar_cstring_t name;
    nar_size_t size;
    nar_object_t *values;
} nar_option_t;

typedef struct {
    nar_cptr_t ptr;
    nar_size_t arity;
} nar_func_t;

typedef struct {
    nar_size_t fn_index;
    nar_object_t curried;
} nar_closure_t;

typedef struct {
    nar_object_t key;
    nar_object_t value;
    nar_object_t parent;
} nar_record_item_t;

typedef struct {
    nar_object_t value;
    nar_object_t next;
} nar_tuple_item_t;

typedef struct {
    nar_object_t value;
    nar_object_t next;
} nar_list_item_t;

typedef struct {
    nar_object_t name;
    nar_object_t values;
} nar_option_item_t;

typedef enum {
    NAR_OBJECT_KIND_UNKNOWN = 0,
    NAR_OBJECT_KIND_UNIT = 1,
    NAR_OBJECT_KIND_CHAR = 2,
    NAR_OBJECT_KIND_INT = 3,
    NAR_OBJECT_KIND_FLOAT = 4,
    NAR_OBJECT_KIND_STRING = 5,
    NAR_OBJECT_KIND_RECORD = 6,
    NAR_OBJECT_KIND_TUPLE = 7,
    NAR_OBJECT_KIND_LIST = 8,
    NAR_OBJECT_KIND_OPTION = 9,
    NAR_OBJECT_KIND_FUNCTION = 10,
    NAR_OBJECT_KIND_CLOSURE = 11,
    NAR_OBJECT_KIND_NATIVE = 12,
    NAR_OBJECT_KIND_PATTERN = 13,
    NAR_OBJECT_KIND__COUNT = 14,
} nar_object_kind_t;

typedef void *nar_bytecode_t;

static const nar_object_t NAR_INVALID_OBJECT = 0;

static const nar_object_t NAR_INVALID_INDEX = 0x0080000000000000;

#endif // NAR_H

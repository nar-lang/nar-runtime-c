#ifndef NAR_RUNTIME_BYTECODE_H
#define NAR_RUNTIME_BYTECODE_H

#include "include/nar.h"
#include "include/hashmap/hashmap.h"
#include <stdint.h>

typedef uint32_t version_t;
typedef uint32_t index_t;
typedef uint64_t op_t;
typedef uint32_t reg_a_t;
typedef uint8_t reg_b_t;
typedef uint8_t reg_c_t;

typedef enum {
    OP_KING_NONE = 0,
// OpKindLoadLocal adds named local object to the top of the stack
    OP_KIND_LOAD_LOCAL = 1,
// OpKindLoadGlobal adds global object to the top of the stack
    OP_KIND_LOAD_GLOBAL = 2,
// OpKindLoadConst adds const value object to the top of the stack
    OP_KIND_LOAD_CONST = 3,
// OpKindApply executes the function from the top of the stack.
// Arguments are taken from the top of the stack in reverse order
// (topmost object is the last arg). Returned value is left on the top of the stack.
// In case of NumArgs is less than number of function parameters it creates
// a closure and leaves it on the top of the stack
    OP_KIND_APPLY = 4,
// OpKindCall executes native function.
// Arguments are taken from the top of the stack in reverse order
// (topmost object is last arg). Returned value is left on the top of the stack.
    OP_KIND_CALL = 5,
// OpKindJump moves on delta ops unconditional
// conditional jump tries to match pattern with object on the top of the stack.
// If it cannot be matched it moves on delta ops
// If it matches successfully - locals are extracted from pattern
// Matched object is left on the top of the stack in both cases
    OP_KIND_JUMP = 6,
// OpKindMakeObject creates an object on stack.
// List items stored on stack in reverse order (topmost object is the last item)
// Record fields stored as repeating pairs const string and value (field name is on the top of the stack)
// Data stores option name as const string on the top of the stack and
// args after that in reverse order (topmost is the last arg)
    OP_KIND_MAKE_OBJECT = 7,
// OpKindMakePattern creates pattern object
// Arguments are taken from the top of the stack in reverse order
// (topmost object is the last arg). Created object is left on the top of the stack.
    OP_KIND_MAKE_PATTERN = 8,
// OpKindAccess takes record object from the top of the stack and leaves its field on the stack
    OP_KIND_ACCESS = 9,
// OpKindUpdate create new record with replaced field from the top of the stack and rest fields
// form the second record object from stack. Created record is left on the top of the stack
    OP_KIND_UPDATE = 10,
// OpKindSwapPop if pop mode - removes topmost object from the stack
// if both mode - removes second object from the top of the stack
    OP_KIND_SWAP_POP = 11,
} op_kind_t;

typedef enum {
    PATTERN_KIND_NONE = 0,
    PATTERN_KIND_ALIAS = 1,
    PATTERN_KIND_ANY = 2,
    PATTERN_KIND_CONS = 3,
    PATTERN_KIND_CONST = 4,
    PATTERN_KIND_OPTION = 5,
    PATTERN_KIND_LIST = 6,
    PATTERN_KIND_NAMED = 7,
    PATTERN_KIND_RECORD = 8,
    PATTERN_KIND_TUPLE = 9,
} pattern_kind_t;

typedef enum {
    CONST_KIND_NONE = 0,
    CONST_KIND_UNIT = 1,
    CONST_KIND_CHAR = 2,
    CONST_KIND_INT = 3,
    CONST_KIND_FLOAT = 4,
    CONST_KIND_STRING = 5,
} const_kind_t;

typedef enum {
    STACK_KIND_NONE = 0,
    STACK_KIND_OBJECT = 1,
    STACK_KIND_PATTERN = 2,
} stack_kind_t;

typedef enum {
    OBJECT_KIND_NONE = 0,
    OBJECT_KIND_LIST = 1,
    OBJECT_KIND_TUPLE = 2,
    OBJECT_KIND_RECORD = 3,
    OBJECT_KIND_OPTION = 4,
} object_kind_t;

typedef enum {
    SWAP_POP_KIND_NONE = 0,
    SWAP_POP_KIND_BOTH = 1,
    SWAP_POP_KIND_POP = 2,
} swap_pop_kind_t;

typedef enum {
    HASHED_CONST_KIND_NONE = 0,
    HASHED_CONST_KIND_INT = 1,
    HASHED_CONST_KIND_FLOAT = 2,
} hashed_const_kind_t;

typedef struct {
    uint32_t line;
    uint32_t column;
} location_t;

typedef struct {
    uint32_t num_args;
    uint32_t num_ops;
    op_t *ops;
    nar_string_t name;
    nar_string_t file_path;
    location_t *locations;
} func_t;

typedef struct {
    union {
        uint64_t hashed_value;
        nar_int_t int_value;
        nar_float_t float_value;
    };
    hashed_const_kind_t kind;
} hashed_const_t;

typedef struct {
    nar_cstring_t name;
    index_t index;
} exports_item_t;

typedef struct {
    nar_string_t name;
    version_t version;
} packages_item_t;

typedef struct {
    version_t compiler_version;
    uint32_t num_functions;
    uint32_t num_strings;
    uint32_t num_constants;
    func_t *functions;
    nar_string_t *strings;
    hashed_const_t *constants;
    nar_string_t entry;
    hashmap_t *exports; // exports_item_t
    hashmap_t *packages; // packages_item_t
} bytecode_t;

static op_kind_t decompose_op(op_t op, reg_a_t *a, reg_b_t *b, reg_c_t *c) {
    *a = (reg_a_t) ((op >> 32) & 0xffffffff);
    *c = (reg_c_t) ((op >> 16) & 0xff);
    *b = (reg_b_t) ((op >> 8) & 0xff);
    return (op_kind_t) (op & 0xff);
}

#endif //NAR_RUNTIME_BYTECODE_H



#include <string.h>
#include <stdio.h>
#include "runtime.h"
#include "include/vector.h"
#include "include/nar-runtime.h"

typedef nar_object_t (*fn0_t)(nar_runtime_t rt);
typedef nar_object_t (*fn1_t)(nar_runtime_t rt, nar_object_t a);
typedef nar_object_t (*fn2_t)(nar_runtime_t rt, nar_object_t a, nar_object_t b);
typedef nar_object_t (*fn3_t)(nar_runtime_t rt, nar_object_t a, nar_object_t b, nar_object_t c);
typedef nar_object_t (*fn4_t)(nar_runtime_t rt, nar_object_t a, nar_object_t b, nar_object_t c,
        nar_object_t d);
typedef nar_object_t (*fn5_t)(nar_runtime_t rt, nar_object_t a, nar_object_t b, nar_object_t c,
        nar_object_t d, nar_object_t e);
typedef nar_object_t (*fn6_t)(nar_runtime_t rt, nar_object_t a, nar_object_t b, nar_object_t c,
        nar_object_t d, nar_object_t e, nar_object_t f);
typedef nar_object_t (*fn7_t)(nar_runtime_t rt, nar_object_t a, nar_object_t b, nar_object_t c,
        nar_object_t d, nar_object_t e, nar_object_t f, nar_object_t g);
typedef nar_object_t (*fn8_t)(nar_runtime_t rt, nar_object_t a, nar_object_t b, nar_object_t c,
        nar_object_t d, nar_object_t e, nar_object_t f, nar_object_t g, nar_object_t h);

nar_bool_t const_equals_to(runtime_t *rt, nar_object_t x, nar_object_t y) {
    nar_object_kind_t x_kind = nar_object_get_kind(rt, x);
    nar_object_kind_t y_kind = nar_object_get_kind(rt, y);
    if (x_kind != y_kind) {
        nar_fail(rt, "trying to compare objects of different types");
        return false;
    }

    switch (x_kind) {
        case NAR_OBJECT_KIND_UNIT:
            return true;
        case NAR_OBJECT_KIND_CHAR:
            return nar_to_char(rt, x) == nar_to_char(rt, y);
        case NAR_OBJECT_KIND_INT:
            return nar_to_int(rt, x) == nar_to_int(rt, y);
        case NAR_OBJECT_KIND_FLOAT:
            return nar_to_float(rt, x) == nar_to_float(rt, y);
        case NAR_OBJECT_KIND_STRING:
            return strcmp(nar_to_string(rt, x), nar_to_string(rt, y)) == 0;
        default:
            nar_fail(rt, "trying to compare objects of unsupported type");
            return false;
    }
}

nar_bool_t match( // NOLINT(*-no-recursion)
        runtime_t *rt, nar_object_t pattern, nar_object_t obj, size_t *num_locals) {
    nar_pattern_t p = nar_to_pattern(rt, pattern);
    switch (p.kind) {
        case PATTERN_KIND_ALIAS: {
            vector_push(rt->locals, 1, &(local_t) {.name = p.name, .value = obj});
            (*num_locals)++;
            nar_list_t pat_values = nar_to_list(rt, p.values);
            if (pat_values.size != 1) {
                nar_fail(rt, "alias pattern should have exactly one pat_values pattern");
                return false;
            }
            return match(rt, pat_values.items[0], obj, num_locals);
        }
        case PATTERN_KIND_ANY:
            return true;
        case PATTERN_KIND_CONS: {
            nar_list_t pat_values = nar_to_list(rt, p.values);
            if (pat_values.size != 2) {
                nar_fail(rt, "cons pattern should have exactly two pat_values patterns");
                return false;
            }
            if (!index_is_valid(obj)) {
                return false;
            }
            nar_list_item_t list = nar_to_list_item(rt, obj);
            nar_bool_t matched = match(rt, pat_values.items[1], list.value, num_locals);
            if (!matched) {
                return false;
            }
            return match(rt, pat_values.items[0], list.next, num_locals);
        }
        case PATTERN_KIND_CONST: {
            nar_list_t pat_values = nar_to_list(rt, p.values);
            if (pat_values.size != 1) {
                nar_fail(rt, "const pattern should have exactly one pat_values pattern");
                return false;
            }
            return const_equals_to(rt, pat_values.items[0], obj);
        }
        case PATTERN_KIND_OPTION: {
            nar_option_t opt = nar_to_option(rt, obj);
            if (strcmp(p.name, opt.name) != 0) {
                return false;
            }
            nar_list_t pat_values = nar_to_list(rt, p.values);
            if (pat_values.size != opt.size) {
                nar_fail(rt, "invalid option pattern match, number of values differs");
                return false;
            }
            for (size_t i = 0; i < pat_values.size; i++) {
                if (!match(rt, pat_values.items[i], opt.values[i], num_locals)) {
                    return false;
                }
            }
            return true;
        }
        case PATTERN_KIND_LIST: {
            nar_list_t obj_list = nar_to_list(rt, obj);
            nar_list_t pat_list = nar_to_list(rt, p.values);
            if (obj_list.size != pat_list.size) {
                return false;
            }
            for (size_t i = 0; i < obj_list.size; i++) {
                if (!match(rt, pat_list.items[i], obj_list.items[i], num_locals)) {
                    return false;
                }
            }
            return true;
        }
        case PATTERN_KIND_NAMED: {
            vector_push(rt->locals, 1, &(local_t) {.name = p.name, .value = obj});
            (*num_locals)++;
            return true;
        }
        case PATTERN_KIND_RECORD: {
            nar_list_t field_names = nar_to_list(rt, p.values);
            for (size_t i = 0; i < field_names.size; i++) {
                nar_cstring_t name = nar_to_string(rt, field_names.items[i]);
                nar_object_t field = nar_to_record_field(rt, obj, name);
                if (!nar_object_is_valid(rt, field)) {
                    return false;
                }
                vector_push(rt->locals, 1, &(local_t) {.name = name, .value = field});
                (*num_locals)++;
            }
            return true;
        }
        case PATTERN_KIND_TUPLE: {
            nar_tuple_t tuple = nar_to_tuple(rt, obj);
            nar_list_t pat_items = nar_to_list(rt, p.values);
            if (pat_items.size != tuple.size) {
                return false;
            }
            for (size_t i = 0; i < pat_items.size; i++) {
                if (!match(rt, pat_items.items[i], tuple.values[i], num_locals)) {
                    return false;
                }
            }
            return true;
        }
        default: {
            nar_fail(rt, "loaded bytecode is corrupted (invalid pattern kind)");
            return false;
        }
    }
}

nar_cstring_t get_string(runtime_t *rt, index_t index) {
    nar_assert(index < rt->program->num_strings);
    return rt->program->strings[index];
}

const func_t *get_function(runtime_t *rt, index_t index) {
    nar_assert(index < rt->program->num_functions);
    return &rt->program->functions[index];
}

const hashed_const_t *get_hashed_const(runtime_t *rt, index_t index) {
    nar_assert(index < rt->program->num_constants);
    return &rt->program->constants[index];
}

nar_object_t execute(runtime_t *rt, const func_t *fn, vector_t *stack) { // NOLINT(*-no-recursion)
    vector_t *pattern_stack = rvector_new(sizeof(nar_object_t), 0);
    vector_push(rt->call_stack, 1, &fn->name);
    size_t num_locals = 0;
    nar_object_t result = INVALID_OBJECT;

    for (size_t index = 0; index < fn->num_ops; index++) {
        reg_a_t a;
        reg_b_t b;
        reg_c_t c;
        op_kind_t op_kind = decompose_op(fn->ops[index], &a, &b, &c);

        switch (op_kind) {
            case OP_KIND_LOAD_LOCAL: {
                nar_cstring_t name = get_string(rt, a);
                bool found = false;
                local_t *start = vector_at(rt->locals, vector_size(rt->locals) - 1);
                local_t *end = start - num_locals;
                for (local_t *it = start; it != end; it--) {
                    if (strcmp(it->name, name) == 0) {
                        vector_push(stack, 1, &it->value);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    nar_fail(rt, "loaded bytecode is corrupted (undefined local)");
                    goto cleanup;
                }
                break;
            }
            case OP_KIND_LOAD_GLOBAL: {
                const func_t *glob = get_function(rt, a);
                if (glob->num_args == 0) {
                    vector_t *inner_stack = rvector_new(sizeof(nar_object_t), 0);
                    nar_object_t const_value = execute(rt, glob, inner_stack);
                    vector_free(inner_stack);

                    vector_push(stack, 1, &const_value);
                } else {
                    nar_object_t closure = nar_new_closure(rt, a, 0, NULL);
                    vector_push(stack, 1, &closure);
                }
                break;
            }
            case OP_KIND_LOAD_CONST: {
                nar_object_t value;
                switch ((const_kind_t) c) {
                    case CONST_KIND_UNIT: {
                        value = nar_new_unit(rt);
                        break;
                    }
                    case CONST_KIND_CHAR: {
                        value = nar_new_char(rt, (nar_char_t) a);
                        break;
                    }
                    case CONST_KIND_INT: {
                        value = nar_new_int(rt, get_hashed_const(rt, a)->int_value);
                        break;
                    }
                    case CONST_KIND_FLOAT: {
                        value = nar_new_float(rt, get_hashed_const(rt, a)->float_value);
                        break;
                    }
                    case CONST_KIND_STRING: {
                        value = nar_new_string(rt, get_string(rt, a));
                        break;
                    }
                    default: {
                        nar_fail(rt, "loaded bytecode is corrupted (invalid const kind)");
                        goto cleanup;
                    }
                }
                switch ((stack_kind_t) b) {
                    case STACK_KIND_OBJECT: {
                        vector_push(stack, 1, &value);
                        break;
                    }
                    case STACK_KIND_PATTERN: {
                        vector_push(pattern_stack, 1, &value);
                        break;
                    }
                    default: {
                        nar_fail(rt, "loaded bytecode is corrupted (invalid stack kind)");
                        goto cleanup;
                    }
                }
                break;
            }
            case OP_KIND_APPLY: {
                nar_object_t x;
                vector_pop(stack, 1, &x);
                nar_closure_t afn = nar_to_closure(rt, x);
                size_t num_args = b;
                nar_list_t curried = nar_to_list(rt, afn.curried);
                size_t num_params = num_args + curried.size;
                vector_t *args = rvector_new(sizeof(nar_object_t), num_params);
                vector_push(args, curried.size, curried.items);
                vector_pop_vec(stack, num_args, args);

                func_t *f = &rt->program->functions[afn.fn_index];
                nar_object_t apply_result;
                if (f->num_args == num_params) {
                    apply_result = execute(rt, f, args);
                } else {
                    apply_result = nar_new_closure(rt, afn.fn_index, vector_size(args),
                            vector_data(args));
                }
                vector_free(args);
                vector_push(stack, 1, &apply_result);
                break;
            }
            case OP_KIND_CALL: {
                nar_cstring_t name = get_string(rt, a);
                const native_def_item_t *def = hashmap_get(rt->native_defs,
                        &(native_def_item_t) {.name = name});
                if (def == NULL) {
                    char err[1024];
                    snprintf(err, 1024, "definition `%s` is not registered", name);
                    nar_fail(rt, err);
                    goto cleanup;
                }

                if (nar_object_get_kind(rt, def->value) == NAR_OBJECT_KIND_FUNCTION) {
                    nar_object_t call_result = INVALID_OBJECT;
                    nar_func_t func = nar_to_func(rt, def->value);
                    size_t n = vector_size(stack);
                    nar_object_t *args = nar_alloc(n * sizeof(nar_object_t));
                    vector_pop(stack, n, args);
                    switch (func.arity) {
                        case 0:
                            call_result = ((fn0_t) func.ptr)(rt);
                            break;
                        case 1:
                            call_result = ((fn1_t) func.ptr)(rt, args[0]);
                            break;
                        case 2:
                            call_result = ((fn2_t) func.ptr)(rt, args[0], args[1]);
                            break;
                        case 3:
                            call_result = ((fn3_t) func.ptr)(rt, args[0], args[1], args[2]);
                            break;
                        case 4:
                            call_result = ((fn4_t) func.ptr)(rt, args[0], args[1], args[2],
                                    args[3]);
                            break;
                        case 5:
                            call_result = ((fn5_t) func.ptr)(rt, args[0], args[1], args[2], args[3],
                                    args[4]);
                            break;
                        case 6:
                            call_result = ((fn6_t) func.ptr)(rt, args[0], args[1], args[2], args[3],
                                    args[4], args[5]);
                            break;
                        case 7:
                            call_result = ((fn7_t) func.ptr)(rt, args[0], args[1], args[2], args[3],
                                    args[4], args[5], args[6]);
                            break;
                        case 8:
                            call_result = ((fn8_t) func.ptr)(rt, args[0], args[1], args[2], args[3],
                                    args[4], args[5], args[6], args[7]);
                            break;
                        default:
                            nar_fail(rt, "function has too many parameters");
                            goto cleanup;
                    }
                    nar_free(args);
                    if (!nar_object_is_valid(rt, call_result)) {
                        char err[1024];
                        snprintf(err, 1024, "definition `%s` returned invalid object", name);
                        nar_fail(rt, err);
                        goto cleanup;
                    }
                    vector_push(stack, 1, &call_result);
                } else {
                    nar_fail(rt, "definition is not a function");
                    goto cleanup;
                }
                break;
            }
            case OP_KIND_JUMP:
                if (b == 0) {
                    index += a;
                } else {
                    nar_object_t pt;
                    vector_pop(pattern_stack, 1, &pt);
                    nar_object_t obj = *(nar_object_t *) vector_at(stack, vector_size(stack) - 1);
                    if (!match(rt, pt, obj, &num_locals)) {
                        if (a == 0) {
                            nar_fail(rt, "pattern match with jump delta 0 should not fail");
                            goto cleanup;
                        }
                        index += a;
                    }
                }
                break;
            case OP_KIND_MAKE_OBJECT:
                switch ((object_kind_t) b) {
                    case OBJECT_KIND_LIST: {
                        nar_object_t *items = nar_alloc(a * sizeof(nar_object_t));
                        vector_pop(stack, a, items);
                        nar_object_t list = nar_new_list(rt, a, items);
                        nar_free(items);
                        vector_push(stack, 1, &list);
                        break;
                    }
                    case OBJECT_KIND_TUPLE: {
                        nar_object_t *items = nar_alloc(a * sizeof(nar_object_t));
                        vector_pop(stack, a, items);
                        nar_object_t tuple = nar_new_tuple(rt, a, items);
                        nar_free(items);
                        vector_push(stack, 1, &tuple);
                        break;
                    }
                    case OBJECT_KIND_RECORD: {
                        nar_object_t *items = nar_alloc(a * 2 * sizeof(nar_object_t));
                        vector_pop(stack, a * 2, items);
                        nar_object_t record = nar_new_record_raw(rt, a, items);
                        nar_free(items);
                        vector_push(stack, 1, &record);
                        break;
                    }
                    case OBJECT_KIND_OPTION: {
                        nar_object_t name_obj;
                        vector_pop(stack, 1, &name_obj);
                        nar_cstring_t name = nar_to_string(rt, name_obj);
                        nar_object_t *values = nar_alloc(a * sizeof(nar_object_t));
                        vector_pop(stack, a, values);
                        nar_object_t option = nar_new_option(rt, name, a, values);
                        nar_free(values);
                        vector_push(stack, 1, &option);
                        break;
                    }
                    default: {
                        nar_fail(rt, "loaded bytecode is corrupted (invalid object kind)");
                        goto cleanup;
                    }
                }
                break;
            case OP_KIND_MAKE_PATTERN: {
                nar_cstring_t name = "";
                nar_object_t *items = NULL;
                size_t num_items = 0;
                pattern_kind_t kind = (pattern_kind_t) b;
                switch (kind) {
                    case PATTERN_KIND_ALIAS: {
                        name = get_string(rt, a);
                        num_items = 1;
                        break;
                    }
                    case PATTERN_KIND_ANY: {
                        break;
                    }
                    case PATTERN_KIND_CONS: {
                        num_items = 2;
                        break;
                    }
                    case PATTERN_KIND_CONST: {
                        num_items = 1;
                        break;
                    }
                    case PATTERN_KIND_OPTION: {
                        name = get_string(rt, a);
                        num_items = c;
                        break;
                    }
                    case PATTERN_KIND_LIST: {
                        num_items = a;
                        break;
                    }
                    case PATTERN_KIND_NAMED: {
                        name = get_string(rt, a);
                        break;
                    }
                    case PATTERN_KIND_RECORD: {
                        num_items = a * 2;
                        break;
                    }
                    case PATTERN_KIND_TUPLE:
                        num_items = c;
                        break;
                    default: {
                        nar_fail(rt, "loaded bytecode is corrupted (invalid pattern kind)");
                        goto cleanup;
                    }
                }

                items = nar_alloc(num_items * sizeof(nar_object_t));
                vector_pop(pattern_stack, num_items, items);
                nar_object_t pattern = nar_new_pattern(rt, kind, name, num_items, items);
                nar_free(items);
                if (!nar_object_is_valid(rt, pattern)) {
                    nar_fail(rt, "loaded bytecode is corrupted (failed to create pattern)");
                    goto cleanup;
                }
                vector_push(pattern_stack, 1, &pattern);
                break;
            }
            case OP_KIND_ACCESS: {
                nar_object_t record;
                vector_pop(stack, 1, &record);
                nar_cstring_t name = get_string(rt, a);
                nar_object_t field = nar_to_record_field(rt, record, name);
                if (!nar_object_is_valid(rt, field)) {
                    nar_fail(rt, "loaded bytecode is corrupted (record missing field)");
                    goto cleanup;
                }
                vector_push(stack, 1, &field);
                break;
            }
            case OP_KIND_UPDATE: {
                nar_cstring_t key = get_string(rt, a);
                nar_object_t value, record;
                vector_pop(stack, 1, &value);
                vector_pop(stack, 1, &record);
                nar_object_t updated = nar_new_record_field(rt, record, key, value);
                vector_push(stack, 1, &updated);
                break;
            }
            case OP_KIND_SWAP_POP: {
                switch ((swap_pop_kind_t) b) {
                    case SWAP_POP_KIND_BOTH: {
                        nar_object_t v;
                        vector_pop(stack, 1, &v);
                        vector_pop(stack, 1, NULL);
                        vector_push(stack, 1, &v);
                        break;
                    }
                    case SWAP_POP_KIND_POP: {
                        vector_pop(stack, 1, NULL);
                        break;
                    }
                    default: {
                        nar_fail(rt, "loaded bytecode is corrupted (invalid swap pop kind)");
                        goto cleanup;
                    }
                }
                break;
            }
            default: {
                nar_fail(rt, "loaded binary is corrupted (invalid op kind)");
                goto cleanup;
            }
        }
    }
    if (num_locals > vector_size(rt->locals)) {
        nar_fail(rt, "bytecode is corrupted (local stack underflow)");
        goto cleanup;
    }

    vector_pop(stack, 1, &result);

    cleanup:
    vector_free(pattern_stack);
    vector_pop(rt->locals, num_locals, NULL);
    vector_pop(rt->call_stack, 1, NULL);
    return result;
}

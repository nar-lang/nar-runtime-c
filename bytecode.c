#include <string.h>
#include "bytecode.h"
#include "include/nar-runtime.h"

const uint32_t k_signature = 'N' << 8 | 'A' << 16 | 'R' << 24;
const uint32_t k_formatVersion = 100;

bool read_u8(const uint8_t *limit, uint8_t **data, uint8_t *out_value) {
    const size_t d = 1;
    if (*data + d < limit) {
        *out_value = *((nar_bool_t *) *data);
        *data += d;
        return true;
    }
    return false;
}

bool read_u32(const uint8_t *limit, uint8_t **data, uint32_t *out_value) {
    const size_t d = sizeof(uint32_t);
    if (*data + d <= limit) {
        *out_value = *((uint32_t *) *data);
        *data += d;
        return true;
    }
    return false;
}

bool read_u64(const uint8_t *limit, uint8_t **data, uint64_t *out_value) {
    const size_t d = sizeof(uint64_t);
    if (*data + d < limit) {
        *out_value = *((uint64_t *) *data);
        *data += d;
        return true;
    }
    return false;
}

bool read_string(const uint8_t *limit, uint8_t **data, nar_string_t *out_value) {
    uint32_t size;
    if (read_u32(limit, data, &size)) {
        nar_string_t value = nar_alloc(size + 1);
        memcpy(value, *data, size);
        value[size] = 0;
        *out_value = value;
        *data += size;
        return true;
    }
    *out_value = NULL;
    return false;
}

int exports_item_compare(const void *a, const void *b, __attribute__((unused)) void *data) {
    const exports_item_t *ia = a;
    const exports_item_t *ib = b;
    return strcmp(ia->name, ib->name);
}

uint64_t exports_item_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const exports_item_t *i = item;
    return hashmap_sip(i->name, strlen(i->name), seed0, seed1);
}

void exports_item_free(void *item) {
    exports_item_t *i = item;
    nar_free((nar_string_t) i->name);
}

int packages_item_compare(const void *a, const void *b, __attribute__((unused)) void *data) {
    const packages_item_t *ia = a;
    const packages_item_t *ib = b;
    return strcmp(ia->name, ib->name);
}

uint64_t packages_item_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const packages_item_t *i = item;
    return hashmap_sip(i->name, strlen(i->name), seed0, seed1);
}

void packages_item_free(void *item) {
    packages_item_t *i = item;
    nar_free((nar_string_t) i->name);
}

nar_result_t bytecode_load_binary(size_t size, uint8_t *data, bytecode_t *out_bc) {
    const uint8_t *limit = data + size;
    *out_bc = (bytecode_t) {0};

    uint32_t signature;
    read_u32(limit, &data, &signature);
    if (signature != k_signature) {
        return NAR_RESULT_SIGNATURE_MISMATCH;
    }

    uint32_t format_version;
    read_u32(limit, &data, &format_version);
    if (format_version != k_formatVersion) {
        return NAR_RESULT_UNSUPPORTED_FORMAT_VERSION;
    }

    if (!read_u32(limit, &data, &out_bc->compiler_version)) {
        return NAR_RESULT_UNEXPECTED_END;
    }
    uint8_t debug;
    if (!read_u8(limit, &data, &debug)) {
        return NAR_RESULT_UNEXPECTED_END;
    }

    if (!read_string(limit, &data, &out_bc->entry)) {
        return NAR_RESULT_UNEXPECTED_END;
    }

    if (!read_u32(limit, &data, &out_bc->num_strings)) {
        return NAR_RESULT_UNEXPECTED_END;
    }
    out_bc->strings = (nar_string_t *) nar_alloc(out_bc->num_strings * sizeof(nar_string_t));
    for (size_t i = 0; i < out_bc->num_strings; i++) {
        if (!read_string(limit, &data, &out_bc->strings[i])) {
            return NAR_RESULT_UNEXPECTED_END;
        }
    }

    if (!read_u32(limit, &data, &out_bc->num_constants)) {
        return NAR_RESULT_UNEXPECTED_END;
    }
    out_bc->constants = (hashed_const_t *) nar_alloc(
            out_bc->num_constants * sizeof(hashed_const_t));
    for (size_t i = 0; i < out_bc->num_constants; i++) {
        uint8_t kind;
        if (!read_u8(limit, &data, &kind)) {
            return NAR_RESULT_UNEXPECTED_END;
        }
        out_bc->constants[i].kind = (hashed_const_kind_t) kind;
        if (!read_u64(limit, &data, &out_bc->constants[i].hashed_value)) {
            return NAR_RESULT_UNEXPECTED_END;
        }
    }

    if (!read_u32(limit, &data, &out_bc->num_functions)) {
        return NAR_RESULT_UNEXPECTED_END;
    }
    out_bc->functions = (func_t *) nar_alloc(out_bc->num_functions * sizeof(func_t));
    for (size_t i = 0; i < out_bc->num_functions; i++) {
        func_t *f = &out_bc->functions[i];

        index_t name_index;
        if (!read_u32(limit, &data, &name_index)) {
            return NAR_RESULT_UNEXPECTED_END;
        }

        f->name = out_bc->strings[name_index];

        if (!read_u32(limit, &data, &f->num_args)) {
            return NAR_RESULT_UNEXPECTED_END;
        }
        if (!read_u32(limit, &data, &f->num_ops)) {
            return NAR_RESULT_UNEXPECTED_END;
        }
        f->ops = nar_alloc(f->num_ops * sizeof(op_t));
        for (size_t j = 0; j < f->num_ops; j++) {
            if (!read_u64(limit, &data, &f->ops[j])) {
                return NAR_RESULT_UNEXPECTED_END;
            }
        }
        if (debug) {
            if (!read_string(limit, &data, &f->file_path)) {
                return NAR_RESULT_UNEXPECTED_END;
            }
            f->locations = nar_alloc(f->num_ops * sizeof(location_t));
            for (size_t j = 0; j < f->num_ops; j++) {
                if (!read_u32(limit, &data, &f->locations[j].line)) {
                    return NAR_RESULT_UNEXPECTED_END;
                }
                if (!read_u32(limit, &data, &f->locations[j].column)) {
                    return NAR_RESULT_UNEXPECTED_END;
                }
            }
        }
    }

    uint32_t num_exports;
    if (!read_u32(limit, &data, &num_exports)) {
        return NAR_RESULT_UNEXPECTED_END;
    }
    out_bc->exports = hashmap_new(sizeof(exports_item_t), num_exports,
            0, 0, &exports_item_hash, &exports_item_compare, &exports_item_free, NULL);
    for (size_t i = 0; i < num_exports; i++) {
        exports_item_t item = {0};
        nar_string_t name;
        if (!read_string(limit, &data, &name)) {
            return NAR_RESULT_UNEXPECTED_END;
        }
        item.name = name;
        if (!read_u32(limit, &data, &item.index)) {
            return NAR_RESULT_UNEXPECTED_END;
        }
        hashmap_set(out_bc->exports, &item);
    }

    uint32_t num_packages;
    if (!read_u32(limit, &data, &num_packages)) {
        return NAR_RESULT_UNEXPECTED_END;
    }
    out_bc->packages = hashmap_new(sizeof(packages_item_t), num_packages,
            0, 0, &packages_item_hash, &packages_item_compare, &packages_item_free, NULL);
    for (size_t i = 0; i < num_packages; i++) {
        packages_item_t item = {0};
        if (!read_string(limit, &data, &item.name)) {
            return NAR_RESULT_UNEXPECTED_END;
        }
        if (!read_u32(limit, &data, &item.version)) {
            return NAR_RESULT_UNEXPECTED_END;
        }
        hashmap_set(out_bc->packages, &item);
    }

    return NAR_RESULT_OK;
}

nar_result_t nar_bytecode_new(nar_size_t size, const nar_byte_t *data, nar_bytecode_t *out_btc) {
    bytecode_t *btc = nar_alloc(sizeof(bytecode_t));
    memset(btc, 0, sizeof (bytecode_t));
    *out_btc = btc;
    return bytecode_load_binary(size, (nar_byte_t*)data, btc);
}

nar_cstring_t nar_bytecode_get_entry(nar_bytecode_t btc) {
    return ((bytecode_t *) btc)->entry;
}

void nar_bytecode_free(nar_bytecode_t bc) {
    if (bc != NULL) {
        bytecode_t *btc = (bytecode_t *) bc;

        nar_free(btc->entry);

        for (uint32_t i = 0; i < btc->num_strings; i++) {
            nar_free(btc->strings[i]);
        }
        nar_free(btc->strings);

        nar_free(btc->constants);

        for (uint32_t i = 0; i < btc->num_functions; i++) {
            func_t *f = &btc->functions[i];
            nar_free(f->ops);
            nar_free(f->file_path);
            nar_free(f->locations);
        }
        nar_free(btc->functions);

        hashmap_free(btc->exports);
        hashmap_free(btc->packages);

        nar_free(btc);
    }
}

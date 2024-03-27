#include <printf.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "include/nar-runtime.h"
#include "runtime.h"

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, ".UTF8");

    int errno = 0;
    nar_runtime_t rt = NULL;
    nar_bytecode_t btc = NULL;

    if (argc < 2) {
        printf("Usage: %s <binary-file-path>\n", argv[0]);
        errno = -1;
        goto cleanup;
    }

    for (int i = 2; i < argc; i += 2) {
        if (strcmp(argv[i - 1], "--libs-path") == 0) {
            setenv("NAR_LIBS_PATH", argv[i], 1);
        } else {
            printf("Error: unknown option %s\n", argv[i]);
            errno = -2;
            goto cleanup;
        }
    }

    FILE *file = fopen(argv[argc - 1], "rb");
    if (file == NULL) {
        printf("Error: could not open file %s\n", argv[1]);
        errno = -3;
        goto cleanup;
    }
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    nar_byte_t *binary = nar_alloc(file_size);
    fread(binary, 1, file_size, file);
    fclose(file);

    int result = nar_bytecode_new(file_size, binary, &btc);
    nar_free(binary);
    if (result != 0) {
        printf("Error: could not load bytecode from file %s (error code %d)\n", argv[1], result);
        errno = -4;
        goto cleanup;
    }

    char libs_path[1024];
    if (getenv("NAR_LIBS_PATH") != NULL) {
        strncpy(libs_path, getenv("NAR_LIBS_PATH"), 1024);
    }else {
        strncpy(libs_path, argv[argc - 1], 1024);
        char *last_slash = strrchr(libs_path, '/');
        if (last_slash != NULL) {
            *last_slash = '\0';
        } else {
            strncpy(libs_path, ".", 1024);
        }
    }

    rt = nar_runtime_new(btc, libs_path);

    nar_cstring_t err = nar_get_last_error(rt);
    if (err != NULL) {
        printf("Error: could not create runtime (error message: %s)\n", err);
        errno = -5;
        goto cleanup;
    }

    nar_cstring_t entry_point = nar_bytecode_get_entry(btc);

    nar_object_t result_obj = nar_apply(rt, entry_point, 0, NULL);
    if (!nar_object_is_valid(rt, result_obj)) {
        printf("Error: could not execute entry point %s (error message: %s)\n",
                entry_point, nar_get_last_error(rt));
        errno = -6;
        goto cleanup;
    }

    cleanup:
    nar_runtime_free(rt);
    nar_bytecode_free(btc);

    if (allocated_memory != 0) {
        errno = (int)allocated_memory;
    }
    return errno;
}
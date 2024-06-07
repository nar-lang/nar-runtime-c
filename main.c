#include <printf.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>
#include "include/nar-runtime.h"
#include <Nar.Program.h>

#define ENV_NAR_PROGRAM_PATH "NAR_PROGRAM_PATH"
#define ENV_NAR_LIBS_PATH "NAR_LIBS_PATH"

void nar_print_memory();

void clean_and_exit(nar_runtime_t rt, nar_int_t code) {
    nar_runtime_free(rt);
    nar_print_memory();
    exit((int) code);
}

void update() {}

int main(int argc, char *argv[]) {
    nar_runtime_t rt = NULL;
    setlocale(LC_ALL, ".UTF8");

    if (argc > 1) {
        char *program_path = argv[argc - 1];
        if (0 == strcmp(program_path, "--help")) {
            printf(
                    "Usage: %s [arguments] [<path-to-program>]\n\n"
                    "<path-to-program> (optional)  path to compiled program bytecode,\n"
                    "                              set to `program.binar` by default\n\n"
                    "Available options (optional):\n"
                    "--help                        show this help message\n"
                    "--version                     show version information\n"
                    "--libs-path <path>            path where libraries will be loaded from,\n"
                    "                              set to the path of program by default.\n",
                    argv[0]);
            return 0;
        }

        if (0 == strcmp(program_path, "--version")) {
            printf("NarVM version 1.0.1\n"); //TODO: implement versioning
            return 0;
        }

        setenv(ENV_NAR_PROGRAM_PATH, program_path, true);
    }

    int errno = 0;

    for (int i = 2; i < argc; i += 2) {
        if (strcmp(argv[i - 1], "--libs-path") == 0) {
            setenv(ENV_NAR_LIBS_PATH, argv[i], 1);
        } else {
            printf("Error: unknown option %s\n", argv[i]);
            errno = -2;
            goto cleanup;
        }
    }

    if (getenv(ENV_NAR_PROGRAM_PATH) == NULL) {
        setenv("NAR_PROGRAM_PATH", "program.binar", true);
    }

    if (getenv(ENV_NAR_LIBS_PATH) == NULL) {
        char libs_path[1024];
        strncpy(libs_path, getenv(ENV_NAR_PROGRAM_PATH), 1024);
        char *last_slash = strrchr(libs_path, '/');
        if (last_slash != NULL) {
            *last_slash = '\0';
        } else {
            strncpy(libs_path, ".", 1024);
        }

        setenv("NAR_LIBS_PATH", libs_path, true);
    }

    FILE *file = fopen(getenv(ENV_NAR_PROGRAM_PATH), "rb");
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

    nar_bytecode_t btc = nar_bytecode_new(file_size, binary);
    nar_free(binary);
    if (btc == 0 || nar_get_error(NULL) != NULL) {
        printf("Error: could not load bytecode from file %s (%s)\n", argv[1],
                nar_get_error(NULL));
        errno = -4;
        goto cleanup;
    }

    rt = nar_runtime_new(btc);

    if (!nar_register_libs(rt, getenv(ENV_NAR_LIBS_PATH))) {
        nar_cstring_t err = nar_get_error(rt);
        printf("Error: could not create runtime\n%s\n", err);
        errno = -5;
        goto cleanup;
    }

    nar_program_set_args_fn_t set_args = nar_get_metadata(rt, NAR_META__Nar_Program_set_args);
    if (set_args != NULL) {
        set_args(rt, argc, argv);
    }

    nar_cstring_t entry_point = nar_bytecode_get_entry(btc);

    nar_object_t result_obj = nar_apply(rt, entry_point, 0, NULL);
    if (nar_get_error(rt) != NULL) {
        printf("Error: could not execute_program entry point %s (error message: %s)\n",
                entry_point, nar_get_error(rt));
        errno = -6;
        goto cleanup;
    }

    nar_program_execute_fn_t execute = nar_get_metadata(rt, NAR_META__Nar_Program_execute);
    if (execute != NULL) {
        errno = execute(rt, result_obj, NULL, update);
        if (nar_get_error(rt) != NULL) {
            printf("Error: could not execute_program program (error message: %s)\n",
                    nar_get_error(rt));
            errno = -7;
            goto cleanup;
        }
    }

    cleanup:
    clean_and_exit(rt, errno);
}
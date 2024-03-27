cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ~/.nar/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ~/.nar/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ~/.nar/bin)

project(
        nar-runtime-c
        VERSION 1.0.0
        DESCRIPTION "Nar native runtime library"
        LANGUAGES C
)

add_library(nar-runtime SHARED
        bytecode.c
        bytecode.h
        execute.c
        hashmap/hashmap.c
        hashmap/hashmap.h
        include/fchar.h
        include/vector.h
        memory.c
        object.c
        package.c
        runtime.c
        runtime.h
)

add_library(nar-runtime-c STATIC
        bytecode.c
        bytecode.h
        execute.c
        hashmap/hashmap.c
        hashmap/hashmap.h
        include/fchar.h
        include/vector.h
        memory.c
        object.c
        package.c
        runtime.c
        runtime.h
)

add_executable(nare main.c)

add_compile_definitions(CVECTOR_LOGARITHMIC_GROWTH)
#add_compile_definitions(NAR_UNSAFE)
set_target_properties(nar-runtime-c PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(nar-runtime PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(nare PROPERTIES VERSION ${PROJECT_VERSION})

target_link_libraries(nar-runtime)
target_link_libraries(nar-runtime-c)

target_link_libraries(nare nar-runtime-c)

cmake_minimum_required(VERSION 3.9)

set(USER_HOME $ENV{HOME})
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${USER_HOME}/.nar/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${USER_HOME}/.nar/include)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${USER_HOME}/.nar/bin)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")

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
        runtime.c
        runtime.h
)

add_executable(nare main.c)
target_include_directories(nare PRIVATE ~/.nar/include)

add_compile_definitions(CVECTOR_LOGARITHMIC_GROWTH)

target_link_libraries(nar-runtime)
target_link_libraries(nar-runtime-c)
target_link_libraries(nare nar-runtime-c)

file(COPY include DESTINATION ${USER_HOME}/.nar)

cmake_minimum_required(VERSION 3.20)

project(
    BML
    VERSION 0.1.0
    LANGUAGES C
)


# Generate compile_commands.json for tooling (clangd, IDEs, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)


# Library sources
set(BML_SOURCES
    src/core/arena.c
    src/core/context.c
    src/core/ast.c
    src/core/ast_builder.c
    src/core/ast_print.c
    src/core/ast_validate.c
    src/core/visitor.c
    src/core/simplify/simplify.c
    src/core/simplify/rules/arithmetic.c
)


# Build static library
add_library(
    bml
    STATIC
    ${BML_SOURCES}
)


# Public headers
target_include_directories(
    bml
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    bml
    PUBLIC
    m
)


# Compiler warnings
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        bml
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()


# Example programs
add_executable(
    bml_example
    examples/basics/basic_ast.c
)


target_link_libraries(
    bml_example
    PRIVATE
    bml
)

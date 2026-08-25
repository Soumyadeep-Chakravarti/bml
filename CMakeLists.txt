cmake_minimum_required(VERSION 3.20)

project(
    BML
    VERSION 0.4.0
    LANGUAGES C
)

include(CTest)

option(BML_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

#Generate compile_commands.json for tooling(clangd, IDEs, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

#Library sources
set(BML_SOURCES
    src/core/arena.c
    src/core/context.c
    src/core/errors.c
    src/core/ast.c
    src/core/ast_builder.c
    src/core/ast_print.c
    src/core/ast_validate.c
    src/core/visitor.c
    src/core/semantics.c
    src/core/verify.c
    src/core/egraph.c
    src/core/simplify/simplify.c
    src/core/lower.c
    src/core/simplify/rules/arithmetic.c
    src/showcase.c
)

#Build static library
add_library(
    bml
    STATIC
    ${BML_SOURCES}
)

#Public headers
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

add_executable(bml_cli src/cli.c)
target_link_libraries(bml_cli PRIVATE bml)
add_executable(bml_bench bench/bml_bench.c)
target_link_libraries(bml_bench PRIVATE bml)

if (BML_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bml PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(bml INTERFACE -fsanitize=address,undefined)
endif()

#Compiler warnings
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        bml
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

#Example programs
add_executable(
    bml_example
    examples/basics/basic_ast.c
)


target_link_libraries(
    bml_example
    PRIVATE
    bml
)

if (BML_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bml_example PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(bml_example PRIVATE -fsanitize=address,undefined)
endif()

if (BUILD_TESTING)
    add_executable(
        bml_core_tests
        tests/core/test_core.c
    )

    add_executable(bml_showcase_tests tests/test_showcase.c)
    add_executable(bml_optimizer_tests tests/test_optimizer.c)
    add_executable(bml_v04_tests tests/test_v04.c)
    target_link_libraries(bml_showcase_tests PRIVATE bml)
    target_link_libraries(bml_optimizer_tests PRIVATE bml)
    target_link_libraries(bml_v04_tests PRIVATE bml)

    target_link_libraries(
        bml_core_tests
        PRIVATE
        bml
    )

    if (BML_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(bml_core_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(bml_core_tests PRIVATE -fsanitize=address,undefined)
        target_compile_options(bml_optimizer_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(bml_optimizer_tests PRIVATE -fsanitize=address,undefined)
        target_compile_options(bml_v04_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(bml_v04_tests PRIVATE -fsanitize=address,undefined)
    endif()

    add_test(NAME bml_core COMMAND bml_core_tests)
    add_test(NAME bml_showcase COMMAND bml_showcase_tests)
    add_test(NAME bml_optimizer COMMAND bml_optimizer_tests)
    add_test(NAME bml_v04 COMMAND bml_v04_tests)
    add_test(NAME bml_benchmark COMMAND bml_bench)
    add_test(NAME bml_cli_emit_c_optimize
             COMMAND bml_cli emit-c "(x + y)^2 + sin(x + y)" --optimize)
    set_tests_properties(bml_cli_emit_c_optimize PROPERTIES PASS_REGULAR_EXPRESSION
                                                          "const double t0 = x \\+ y;")
    add_test(NAME bml_cli_optimize_canonical COMMAND bml_cli optimize "2*x + x + 0" --target=operations)
    set_tests_properties(bml_cli_optimize_canonical PROPERTIES PASS_REGULAR_EXPRESSION
                                                           "3 \\* x")
    add_test(NAME bml_cli_optimize_explain
             COMMAND bml_cli optimize "(x + y)^2 + sin(x + y)" --target=c --explain)
    set_tests_properties(bml_cli_optimize_explain PROPERTIES PASS_REGULAR_EXPRESSION
                                                            "detected CSE: 1")
    add_test(NAME bml_cli_egraph COMMAND bml_cli optimize "(x+y)^2-2*x*y" --engine=egraph --explain)
    set_tests_properties(bml_cli_egraph PROPERTIES PASS_REGULAR_EXPRESSION "binomial-square")
    add_test(NAME bml_cli_egraph_operations
             COMMAND bml_cli optimize "(x + y)^2 - 2*x*y" --target=operations --engine=egraph --explain)
    set_tests_properties(bml_cli_egraph_operations PROPERTIES PASS_REGULAR_EXPRESSION
                                                              "selected: egraph")
endif()

find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if (CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE BML_HEADERS CONFIGURE_DEPENDS include/bml/*.h)
    set(BML_FORMAT_SOURCES ${BML_SOURCES} ${BML_HEADERS} examples/basics/basic_ast.c src/cli.c bench/bml_bench.c tests/core/test_core.c tests/test_showcase.c tests/test_optimizer.c tests/test_v04.c)

    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${BML_FORMAT_SOURCES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Formatting BML sources"
    )

    add_custom_target(
        format-check
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror ${BML_FORMAT_SOURCES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Checking BML source formatting"
    )
endif()

find_program(CLANG_TIDY_EXECUTABLE clang-tidy)
if (CLANG_TIDY_EXECUTABLE)
    add_custom_target(
        clang-tidy
        COMMAND ${CLANG_TIDY_EXECUTABLE} -p ${CMAKE_BINARY_DIR} ${BML_SOURCES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Running clang-tidy"
    )
endif()

find_program(CPPCHECK_EXECUTABLE cppcheck)
if (CPPCHECK_EXECUTABLE)
    add_custom_target(
        cppcheck
        COMMAND ${CPPCHECK_EXECUTABLE} --enable=warning,style,performance,portability --error-exitcode=1 --std=c11 -I ${PROJECT_SOURCE_DIR}/include ${BML_SOURCES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Running Cppcheck"
    )
endif()

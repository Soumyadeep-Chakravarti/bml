cmake_minimum_required(VERSION 3.20)

project(
    BML
    VERSION 0.5.0
    LANGUAGES C
)

include(CTest)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(BML_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(BML_BUILD_FUZZER "Build the optional libFuzzer parser target when supported" OFF)

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
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(BML::bml ALIAS bml)
set_target_properties(bml PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

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
    target_compile_options(bml PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(bml PUBLIC -fsanitize=address,undefined)
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

    add_test(NAME bml_core COMMAND bml_core_tests)
    add_test(NAME bml_showcase COMMAND bml_showcase_tests)
    add_test(NAME bml_optimizer COMMAND bml_optimizer_tests)
    add_test(NAME bml_v04 COMMAND bml_v04_tests)
    add_test(NAME bml_benchmark COMMAND bml_bench)
    add_test(NAME bml_benchmark_report
             COMMAND ${CMAKE_COMMAND} -DBENCH=$<TARGET_FILE:bml_bench>
                     -DWORK=${CMAKE_CURRENT_BINARY_DIR}/benchmark-report
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_benchmark_report.cmake)
    add_test(NAME bml_installed_consumer
             COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
                     -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                     -DWORK=${CMAKE_CURRENT_BINARY_DIR}/installed-consumer
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_installed_consumer.cmake)
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
    add_test(NAME bml_cli_parse_diagnostic
             COMMAND ${CMAKE_COMMAND} -DCLI=$<TARGET_FILE:bml_cli> -DMODE=parse
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_cli_failure.cmake)
    add_test(NAME bml_cli_invalid_flag
             COMMAND ${CMAKE_COMMAND} -DCLI=$<TARGET_FILE:bml_cli> -DMODE=flag
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_cli_failure.cmake)
    add_executable(bml_parser_fuzz_tests tests/test_parser_fuzz.c)
    target_link_libraries(bml_parser_fuzz_tests PRIVATE bml)
    add_test(NAME bml_parser_fuzz COMMAND bml_parser_fuzz_tests)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if (Python3_Interpreter_FOUND)
        add_test(NAME bml_sympy_differential
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/sympy_differential.py
                         --cli $<TARGET_FILE:bml_cli>)
        set_tests_properties(bml_sympy_differential PROPERTIES LABELS "optional;differential"
                             SKIP_RETURN_CODE 77)
    endif()
endif()

if (BML_BUILD_FUZZER AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    include(CheckCCompilerFlag)
    check_c_compiler_flag("-fsanitize=fuzzer" BML_HAS_LIBFUZZER)
    if (BML_HAS_LIBFUZZER)
        add_executable(bml_parser_fuzzer tests/parser_fuzzer.c)
        target_link_libraries(bml_parser_fuzzer PRIVATE bml)
        target_compile_options(bml_parser_fuzzer PRIVATE -fsanitize=fuzzer)
        target_link_options(bml_parser_fuzzer PRIVATE -fsanitize=fuzzer)
    endif()
endif()

find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if (CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE BML_HEADERS CONFIGURE_DEPENDS include/bml/*.h)
    set(BML_FORMAT_SOURCES ${BML_SOURCES} ${BML_HEADERS} examples/basics/basic_ast.c src/cli.c bench/bml_bench.c tests/core/test_core.c tests/test_showcase.c tests/test_optimizer.c tests/test_v04.c tests/test_parser_fuzz.c tests/parser_fuzzer.c)

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

install(TARGETS bml EXPORT BMLTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/bml DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        PATTERN "internal" EXCLUDE)
install(TARGETS bml_cli bml_bench RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(EXPORT BMLTargets FILE BMLTargets.cmake NAMESPACE BML::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BML)
configure_package_config_file(cmake/BMLConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/BMLConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BML)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/BMLConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BMLConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/BMLConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BML)
configure_file(cmake/bml.pc.in ${CMAKE_CURRENT_BINARY_DIR}/bml.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bml.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

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

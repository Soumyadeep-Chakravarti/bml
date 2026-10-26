cmake_minimum_required(VERSION 3.20)

project(bml VERSION 0.1.0 LANGUAGES C)

add_library(bml
    src/ast.c
    src/context.c
    src/parser.c
)

add_library(BML::bml ALIAS bml)

target_include_directories(bml
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(bml PUBLIC c_std_11)
set_target_properties(bml PROPERTIES C_EXTENSIONS OFF)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(bml PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(bml_showcase examples/showcase.c)
target_link_libraries(bml_showcase PRIVATE BML::bml)

include(CTest)
if(BUILD_TESTING)
    add_executable(bml_parser_tests tests/parser_tests.c)
    target_link_libraries(bml_parser_tests PRIVATE BML::bml)
    add_test(NAME bml_parser_tests COMMAND bml_parser_tests)
endif()

cmake_minimum_required (VERSION 3.9)
project(clike)

find_package(BISON)
find_package(FLEX)

# https://llvm.org/docs/CMake.html#embedding-llvm-in-your-project
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)

FLEX_TARGET(Lexer src/tokens.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
BISON_TARGET(Parser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)

file(GLOB HEADER_FILES include/*.h)

set (CMAKE_CXX_STANDARD 11)
add_definitions(-DYYERROR_VERBOSE)

add_executable(parser
    ${FLEX_Lexer_OUTPUTS}
    ${BISON_Parser_OUTPUTS}
    ${HEADER_FILES}
    src/codegen.cpp
    src/main.cpp)

target_include_directories(parser PRIVATE include ${CONAN_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(parser curses ${llvm_libs})

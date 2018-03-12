cmake_minimum_required (VERSION 3.9)
project(clike)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(BISON)
find_package(FLEX)

FLEX_TARGET(Lexer src/tokens.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
BISON_TARGET(Parser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)

file(GLOB HEADER_FILES include/*.h)

set (CMAKE_CXX_STANDARD 11)

add_executable(parser
    ${FLEX_Lexer_OUTPUTS}
    ${BISON_Parser_OUTPUTS}
    ${HEADER_FILES}
    src/codegen.cpp
    src/main.cpp)

target_include_directories(parser PRIVATE include ${CONAN_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
message("CONAN_LIBS: ${CONAN_LIBS}")
target_link_libraries(parser curses ${CONAN_LIBS})

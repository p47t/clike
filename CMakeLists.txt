cmake_minimum_required (VERSION 3.10)
project(clike)

find_package(BISON)
find_package(FLEX)

FLEX_TARGET(Lexer src/tokens.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
BISON_TARGET(Parser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
add_executable(parser
    ${FLEX_Lexer_OUTPUTS}
    ${BISON_Parser_OUTPUTS}
    src/codegen.cpp
    src/main.cpp)

target_include_directories(parser PUBLIC include)

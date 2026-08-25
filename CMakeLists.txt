cmake_minimum_required(VERSION 3.20)

project(Graph LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Graph
    src/main.cpp
    src/core/ExpressionEngine/Lexer/Lexer.cpp
)

target_include_directories(Graph PRIVATE
    src/core/ExpressionEngine/Lexer
)
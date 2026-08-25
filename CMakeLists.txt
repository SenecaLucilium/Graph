cmake_minimum_required(VERSION 3.20)

project(Graph LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Graph
    src/main.cpp
    src/core/ExpressionEngine/Lexer/Lexer.cpp
    src/core/ExpressionEngine/Parser/ExpressionNode.cpp
    src/core/ExpressionEngine/Parser/Parser.cpp
)

target_include_directories(Graph PRIVATE
    src/core/ExpressionEngine/Lexer
    src/core/ExpressionEngine/Parser
)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(GraphTests
    tests/LexerTests.cpp
    src/core/ExpressionEngine/Lexer/Lexer.cpp
)

target_include_directories(GraphTests PRIVATE
    src/core/ExpressionEngine/Lexer
)

target_link_libraries(GraphTests PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(GraphTests)
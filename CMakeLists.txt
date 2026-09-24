cmake_minimum_required(VERSION 3.20)

project(Graph LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 QUIET COMPONENTS Multimedia)

add_library(GraphCore STATIC
    src/core/ExpressionEngine/ExpressionEngine.cpp
    src/core/ExpressionEngine/Lexer/Lexer.cpp
    src/core/ExpressionEngine/Parser/ExpressionNode.cpp
    src/core/ExpressionEngine/Parser/Parser.cpp
    src/core/ExpressionEngine/Evaluator/Evaluator.cpp
    src/core/ExpressionEngine/Graph/GraphSampler.cpp
    src/core/GraphEngine/AsciiPlot/AsciiPlot.cpp

    src/common/Diagnostics/Logger/Logger.cpp
    src/common/Diagnostics/Logger/Sink/ConsoleSink.cpp
)

target_include_directories(GraphCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ExpressionEngine/Lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ExpressionEngine/Parser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ExpressionEngine/Evaluator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ExpressionEngine/Graph
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/GraphEngine/AsciiPlot
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/Diagnostics
)

add_executable(Graph
    src/main.cpp
    src/ui/cli/Cli.cpp
)

target_include_directories(Graph PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/cli
)

target_link_libraries(Graph PRIVATE
    GraphCore
)

add_executable(GraphUi
    src/ui/qt/main.cpp
    src/ui/qt/AudioPlayer.cpp
    src/ui/qt/MainWindow.cpp
    src/ui/qt/PlotWidget.cpp
)

target_include_directories(GraphUi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qt
)

target_link_libraries(GraphUi PRIVATE
    GraphCore
    Qt6::Widgets
)

if(TARGET Qt6::Multimedia)
    target_compile_definitions(GraphUi PRIVATE GRAPH_HAS_QT_MULTIMEDIA=1)
    target_link_libraries(GraphUi PRIVATE Qt6::Multimedia)
else()
    message(WARNING "Qt6 Multimedia was not found; animated sound will be disabled")
endif()

install(TARGETS GraphUi RUNTIME DESTINATION bin)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(GraphTests
    tests/LexerTests.cpp
    tests/ParserTests.cpp
    tests/EvaluationTests.cpp
    tests/DiagnosticsTests.cpp
)

target_include_directories(GraphTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ExpressionEngine/Lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ExpressionEngine/Parser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/Diagnostics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ExpressionEngine/Evaluator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ExpressionEngine/Graph
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/GraphEngine/AsciiPlot
)

target_link_libraries(GraphTests PRIVATE
    GraphCore
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(GraphTests)

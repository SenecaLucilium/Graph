#include <gtest/gtest.h>

#include "Diagnostics.h"

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

namespace diagnostics = src::common::Diagnostics;
namespace logging = src::common::Diagnostics::Logging;
namespace sink = src::common::Diagnostics::Logging::Sink;

TEST(MemorySinkTests, StoresRecords)
{
    sink::MemorySink memorySink;

    logging::LogRecord record;
    record.level = diagnostics::LogLevel::Debug;
    record.component = "Lexer";
    record.message = "Token created";
    record.context["tokenIndex"] = std::int64_t{3};

    memorySink.write(record);

    ASSERT_EQ(memorySink.size(), 1);

    const logging::LogRecord& storedRecord = memorySink.records().front();

    EXPECT_EQ(storedRecord.level, diagnostics::LogLevel::Debug);
    EXPECT_EQ(storedRecord.component, "Lexer");
    EXPECT_EQ(storedRecord.message, "Token created");

    const auto contextValue = storedRecord.context.find("tokenIndex");

    ASSERT_NE(contextValue, storedRecord.context.end());
    EXPECT_EQ(std::get<std::int64_t>(contextValue->second), 3);
}

TEST(MemorySinkTests, ClearRemovesAllRecords)
{
    sink::MemorySink memorySink;

    memorySink.write(logging::LogRecord{});
    memorySink.write(logging::LogRecord{});

    ASSERT_EQ(memorySink.size(), 2);

    memorySink.clear();

    EXPECT_EQ(memorySink.size(), 0);
    EXPECT_TRUE(memorySink.records().empty());
}

TEST(LoggerTests, FiltersRecordsBelowMinimumLevel)
{
    logging::Logger logger(diagnostics::LogLevel::Info);

    auto memorySink = std::make_unique<sink::MemorySink>();
    sink::MemorySink* memorySinkPointer = memorySink.get();

    logger.addSink(std::move(memorySink));

    logger.debug("Parser", "Debug message");
    EXPECT_EQ(memorySinkPointer->size(), 0);

    logger.info("Parser", "Info message");
    EXPECT_EQ(memorySinkPointer->size(), 1);
}

TEST(LoggerTests, SendsRecordToEverySink)
{
    logging::Logger logger;

    auto firstSink = std::make_unique<sink::MemorySink>();
    auto secondSink = std::make_unique<sink::MemorySink>();

    sink::MemorySink* firstSinkPointer = firstSink.get();
    sink::MemorySink* secondSinkPointer = secondSink.get();

    logger.addSink(std::move(firstSink));
    logger.addSink(std::move(secondSink));

    logger.warning("Parser", "Warning message");

    ASSERT_EQ(firstSinkPointer->size(), 1);
    ASSERT_EQ(secondSinkPointer->size(), 1);

    EXPECT_EQ(firstSinkPointer->records().front().message, "Warning message");
    EXPECT_EQ(secondSinkPointer->records().front().message, "Warning message");
}

TEST(LoggerTests, ForwardsStructuredError)
{
    logging::Logger logger;

    auto memorySink = std::make_unique<sink::MemorySink>();
    sink::MemorySink* memorySinkPointer = memorySink.get();

    logger.addSink(std::move(memorySink));

    diagnostics::Errors::Error error;
    error.domain = diagnostics::ErrorDomain::Parser;
    error.code = diagnostics::ErrorCode::MissingRightParenthesis;
    error.errorLevel = diagnostics::ErrorLevel::Error;
    error.message = "Right parenthesis expected";
    error.location = diagnostics::SourceLocation{
        .position = 12,
        .length = 1,
        .col = 13,
        .row = 1
    };

    logger.error(
        "Parser",
        "Failed to parse expression",
        {},
        std::optional<diagnostics::Errors::Error>{error}
    );

    ASSERT_EQ(memorySinkPointer->size(), 1);

    const logging::LogRecord& record = memorySinkPointer->records().front();

    ASSERT_TRUE(record.error.has_value());
    EXPECT_EQ(record.error->domain, diagnostics::ErrorDomain::Parser);
    EXPECT_EQ(record.error->code, diagnostics::ErrorCode::MissingRightParenthesis);
    EXPECT_EQ(record.error->message, "Right parenthesis expected");
    ASSERT_TRUE(record.error->location.has_value());
    EXPECT_EQ(record.error->location->position, 12);
}

TEST(ConsoleSinkTests, WritesRecordAndErrorDetails)
{
    std::ostringstream output;
    sink::ConsoleSink consoleSink(output);

    diagnostics::Errors::Error error;
    error.domain = diagnostics::ErrorDomain::Lexer;
    error.code = diagnostics::ErrorCode::UnexpectedCharacter;
    error.errorLevel = diagnostics::ErrorLevel::Error;
    error.message = "Unexpected character";
    error.context["character"] = std::string{"@"};

    logging::LogRecord record;
    record.level = diagnostics::LogLevel::Error;
    record.component = "Lexer";
    record.message = "Lexing failed";
    record.error = error;

    consoleSink.write(record);

    const std::string text = output.str();

    EXPECT_NE(text.find("[Error] Lexer: Lexing failed"), std::string::npos);
    EXPECT_NE(text.find("domain: Lexer"), std::string::npos);
    EXPECT_NE(text.find("code: UnexpectedCharacter"), std::string::npos);
    EXPECT_NE(text.find("message: Unexpected character"), std::string::npos);
    EXPECT_NE(text.find("character: @"), std::string::npos);
}

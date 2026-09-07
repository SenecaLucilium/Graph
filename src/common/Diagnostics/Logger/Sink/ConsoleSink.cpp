#include "ConsoleSink.h"

#include <cstdint>
#include <ostream>
#include <string>
#include <variant>

namespace
{

using namespace src::common::Diagnostics;

const char* logLevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Trace:   return "Trace";
    case LogLevel::Debug:   return "Debug";
    case LogLevel::Info:    return "Info";
    case LogLevel::Warning: return "Warning";
    case LogLevel::Error:   return "Error";
    case LogLevel::Fatal:   return "Fatal";
    }

    return "Unknown";
}

const char* errorLevelToString(ErrorLevel level)
{
    switch (level)
    {
    case ErrorLevel::Warning: return "Warning";
    case ErrorLevel::Error:   return "Error";
    case ErrorLevel::Fatal:   return "Fatal";
    }

    return "Unknown";
}

const char* errorDomainToString(ErrorDomain domain)
{
    switch (domain)
    {
    case ErrorDomain::Lexer:      return "Lexer";
    case ErrorDomain::Parser:     return "Parser";
    case ErrorDomain::Evaluation: return "Evaluation";
    case ErrorDomain::IO:         return "IO";
    case ErrorDomain::UI:         return "UI";
    case ErrorDomain::Internal:   return "Internal";
    }

    return "Unknown";
}

const char* errorCodeToString(ErrorCode code)
{
    switch (code)
    {
    case ErrorCode::Unknown:                    return "Unknown";
    case ErrorCode::UnexpectedCharacter:        return "UnexpectedCharacter";
    case ErrorCode::InvalidNumber:              return "InvalidNumber";
    case ErrorCode::InvalidIdentifier:          return "InvalidIdentifier";
    case ErrorCode::UnexpectedToken:            return "UnexpectedToken";
    case ErrorCode::ExpectedExpression:         return "ExpectedExpression";
    case ErrorCode::MissingRightParenthesis:    return "MissingRightParenthesis";
    case ErrorCode::MissingFunctionParenthesis: return "MissingFunctionParenthesis";
    case ErrorCode::UnexpectedEndOfExpression:  return "UnexpectedEndOfExpression";
    case ErrorCode::TrailingTokens:             return "TrailingTokens";
    case ErrorCode::DivisionByZero:             return "DivisionByZero";
    case ErrorCode::InvalidArgument:            return "InvalidArgument";
    case ErrorCode::NonFiniteResult:            return "NonFiniteResult";
    }

    return "Unknown";
}

struct ContextValueWriter
{
    std::ostream& output;

    void operator()(const std::string& value) const { output << value; }
    void operator()(std::int64_t value) const { output << value; }
    void operator()(double value) const { output << value; }
    void operator()(bool value) const { output << (value ? "true" : "false"); }
};

void writeContext(
    std::ostream& output,
    const Context& context,
    const char* indentation
)
{
    for (const auto& [key, value] : context)
    {
        output << indentation << key << ": ";
        std::visit(ContextValueWriter{output}, value);
        output << '\n';
    }
}

void writeLocation(std::ostream& output, const SourceLocation& location)
{
    output << "position: " << location.position << '\n'
           << "length: " << location.length << '\n'
           << "row: " << location.row << '\n'
           << "column: " << location.col << '\n';
}

void writeError(
    std::ostream& output,
    const Errors::Error& error,
    const char* indentation
)
{
    output << indentation << "domain: " << errorDomainToString(error.domain) << '\n'
           << indentation << "code: " << errorCodeToString(error.code) << '\n'
           << indentation << "level: " << errorLevelToString(error.errorLevel) << '\n'
           << indentation << "message: " << error.message << '\n';

    if (error.location.has_value())
    {
        output << indentation << "location:\n";
        writeLocation(output, *error.location);
    }

    if (!error.context.empty())
    {
        output << indentation << "context:\n";
        writeContext(output, error.context, "      ");
    }

    if (error.cause)
    {
        output << indentation << "cause:\n";
        writeError(output, *error.cause, "      ");
    }
}

}

namespace src::common::Diagnostics::Logging::Sink
{

ConsoleSink::ConsoleSink(std::ostream& output_)
    : output(output_)
{
}

void ConsoleSink::write(const LogRecord& record)
{
    output << '['
           << logLevelToString(record.level)
           << "] "
           << record.component
           << ": "
           << record.message
           << '\n';

    if (!record.context.empty())
    {
        output << "  context:\n";
        writeContext(output, record.context, "    ");
    }

    if (record.error.has_value())
    {
        output << "  error:\n";
        writeError(output, *record.error, "    ");
    }

    output << '\n';
}

}

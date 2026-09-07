#pragma once

#include <cstddef>
#include <cstdint>

#include <map>
#include <string>
#include <variant>

namespace src::common::Diagnostics
{

enum class LogLevel
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

enum class ErrorLevel
{
    Warning,
    Error,
    Fatal
};

struct SourceLocation
{
    std::size_t position = 0;
    std::size_t length = 0;
    std::size_t col = 0;
    std::size_t row = 0;
};

enum class ErrorDomain
{
    Lexer,
    Parser,
    Evaluation,
    IO,
    UI,
    Internal
};

using ContextValue = std::variant<std::string, std::int64_t, double, bool>;
using Context = std::map<std::string, ContextValue>;

enum class ErrorCode
{
    Unknown,

    UnexpectedCharacter,
    InvalidNumber,
    InvalidIdentifier,

    UnexpectedToken,
    ExpectedExpression,
    MissingRightParenthesis,
    MissingFunctionParenthesis,
    UnexpectedEndOfExpression,
    TrailingTokens,

    DivisionByZero,
    InvalidArgument,
    NonFiniteResult
};

}

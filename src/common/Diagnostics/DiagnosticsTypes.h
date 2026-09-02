#pragma once

#include <cstddef>
#include <cstdint>

#include <variant>
#include <string>
#include <map>

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
    std::size_t position;
    std::size_t length;
    std::size_t col;
    std::size_t row;
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
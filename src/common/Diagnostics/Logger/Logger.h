#pragma once

#include "../DiagnosticsTypes.h"
#include "../Errors/Errors.h"

namespace src::common::Diagnostics::Logger
{

struct LogRecord
{
    LogLevel level;
    std::string component;
    std::string message;

    std::optional<SourceLocation> location;
    Context context;
    std::optional<Errors::Error> error;
};

}
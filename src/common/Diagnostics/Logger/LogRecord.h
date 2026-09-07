#pragma once

#include <optional>
#include <string>

#include "../DiagnosticsTypes.h"
#include "../Error/Error.h"

namespace src::common::Diagnostics::Logging
{

struct LogRecord
{
    LogLevel level = LogLevel::Info;
    std::string component;
    std::string message;

    Context context;
    std::optional<Errors::Error> error;
};

}

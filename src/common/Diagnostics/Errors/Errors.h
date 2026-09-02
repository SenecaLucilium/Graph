#pragma once

#include "../DiagnosticsTypes.h"

namespace src::common::Diagnostics::Errors
{

struct Error
{
    ErrorDomain domain;
    ErrorCode code;
    ErrorLevel errorLevel;
    
    std::string message;
    std::optional<SourceLocation> location;
    Context context;
    std::shared_ptr<Error> cause;
};
    
}
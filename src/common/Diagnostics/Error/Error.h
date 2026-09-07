#pragma once

#include <optional>
#include <memory>
#include <string>

#include "../DiagnosticsTypes.h"

namespace src::common::Diagnostics::Errors
{

struct Error
{
    ErrorDomain domain = ErrorDomain::Internal;
    ErrorCode code = ErrorCode::Unknown;
    ErrorLevel errorLevel = ErrorLevel::Error;
    
    std::string message;
    std::optional<SourceLocation> location;
    Context context;
    std::shared_ptr<Error> cause;
};
    
}

#pragma once

#include "../LogRecord.h"

namespace src::common::Diagnostics::Logging::Sink
{

using ::src::common::Diagnostics::Logging::LogRecord;

class LogSink
{
public:
    virtual ~LogSink() = default;

    virtual void write(const LogRecord& record) = 0;
};

}

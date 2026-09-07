#pragma once

#include <iosfwd>

#include "LogSink.h"

namespace src::common::Diagnostics::Logging::Sink
{

class ConsoleSink final : public LogSink
{
private:
    std::ostream& output;

public:
    explicit ConsoleSink(std::ostream& output_);

    void write(const LogRecord& record) override;
};

}

#pragma once

#include "LogSink.h"

#include <cstddef>
#include <vector>

namespace src::common::Diagnostics::Logging::Sink
{

class MemorySink final : public LogSink
{
private:
    std::vector<LogRecord> recordsList;

public:
    void write(const LogRecord& record) override
    {
        this->recordsList.push_back(record);
    }

    const std::vector<LogRecord>& records() const noexcept
    {
        return this->recordsList;
    }

    void clear() noexcept
    {
        this->recordsList.clear();
    }

    std::size_t size() const noexcept
    {
        return this->recordsList.size();
    }
};

}

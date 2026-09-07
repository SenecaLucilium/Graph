#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../DiagnosticsTypes.h"
#include "LogRecord.h"

namespace src::common::Diagnostics::Logging
{

namespace Sink
{
class LogSink;
}

class Logger
{
private:
    LogLevel minimumLevel;
    std::vector<std::unique_ptr<Sink::LogSink>> sinks;

public:
    explicit Logger(LogLevel minimumLevel_ = LogLevel::Trace);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void addSink(std::unique_ptr<Sink::LogSink> sink);
    void clearSinks() noexcept;

    void setMinimumLevel(LogLevel level) noexcept;
    LogLevel getMinimumLevel() const noexcept;
    bool isEnabled(LogLevel level) const noexcept;

    void log(const LogRecord& record);

    void log(
        LogLevel level,
        std::string component,
        std::string message,
        Context context = {},
        std::optional<Errors::Error> error = std::nullopt
    );

    void trace(std::string component, std::string message, Context context = {});
    void debug(std::string component, std::string message, Context context = {});
    void info(std::string component, std::string message, Context context = {});
    void warning(std::string component, std::string message, Context context = {});

    void error(
        std::string component,
        std::string message,
        Context context = {},
        std::optional<Errors::Error> error = std::nullopt
    );

    void fatal(
        std::string component,
        std::string message,
        Context context = {},
        std::optional<Errors::Error> error = std::nullopt
    );
};
}

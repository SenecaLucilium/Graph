#include "Logger.h"

#include "Sink/LogSink.h"

#include <utility>

namespace src::common::Diagnostics::Logging
{

Logger::Logger(LogLevel minimumLevel_)
    : minimumLevel(minimumLevel_)
{
}

Logger::~Logger() = default;

void Logger::addSink(std::unique_ptr<Sink::LogSink> sink)
{
    if (sink) this->sinks.push_back(std::move(sink));
}

void Logger::clearSinks() noexcept
{
    this->sinks.clear();
}

void Logger::setMinimumLevel(LogLevel level) noexcept
{
    this->minimumLevel = level;
}

LogLevel Logger::getMinimumLevel() const noexcept
{
    return this->minimumLevel;
}

bool Logger::isEnabled(LogLevel level) const noexcept
{
    return static_cast<int>(level) >= static_cast<int>(this->minimumLevel);
}

void Logger::log(const LogRecord& record)
{
    if (!this->isEnabled(record.level)) return;

    for (const std::unique_ptr<Sink::LogSink>& sink : this->sinks)
    {
        sink->write(record);
    }
}

void Logger::log(
    LogLevel level,
    std::string component,
    std::string message,
    Context context,
    std::optional<Errors::Error> error
)
{
    LogRecord record;
    record.level = level;
    record.component = std::move(component);
    record.message = std::move(message);
    record.context = std::move(context);
    record.error = std::move(error);

    this->log(record);
}

void Logger::trace(std::string component, std::string message, Context context)
{
    this->log(LogLevel::Trace, std::move(component), std::move(message), std::move(context));
}

void Logger::debug(std::string component, std::string message, Context context)
{
    this->log(LogLevel::Debug, std::move(component), std::move(message), std::move(context));
}

void Logger::info(std::string component, std::string message, Context context)
{
    this->log(LogLevel::Info, std::move(component), std::move(message), std::move(context));
}

void Logger::warning(std::string component, std::string message, Context context)
{
    this->log(LogLevel::Warning, std::move(component), std::move(message), std::move(context));
}

void Logger::error(
    std::string component,
    std::string message,
    Context context,
    std::optional<Errors::Error> error
)
{
    this->log(LogLevel::Error, std::move(component), std::move(message), std::move(context), std::move(error));
}

void Logger::fatal(
    std::string component,
    std::string message,
    Context context,
    std::optional<Errors::Error> error
)
{
    this->log(LogLevel::Fatal, std::move(component), std::move(message), std::move(context), std::move(error));
}

}

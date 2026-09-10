#pragma once

#include "Error/Error.h"

namespace src::common::Diagnostics
{

template<typename T>
class Result
{
private:
    std::variant<T, Errors::Error> data;

    explicit Result(T value) : data(std::move(value)) {}
    explicit Result(Errors::Error error) : data(std::move(error)) {}

public:
    static Result success (T value)
    {
        return Result(std::move(value));
    }

    static Result failure (Errors::Error error)
    {
        return Result(std::move(error));
    }

    bool hasValue() const noexcept
    {
        return std::holds_alternative<T>(this->data);
    }

    explicit operator bool() const noexcept
    {
        return this->hasValue();
    }

    T& value() &
    {
        if (!this->hasValue()) throw std::logic_error("Attempt to access value of failed Result");

        return std::get<T>(this->data);
    }

    const T& value() const&
    {
        if (!this->hasValue()) throw std::logic_error("Attempt to access value of failed Result");

        return std::get<T>(this->data);
    }

    T&& value() &&
    {
        if (!this->hasValue()) throw std::logic_error("Attempt to access value of failed Result");

        return std::get<T>(std::move(this->data));
    }

    const Errors::Error& error() const
    {
        if (this->hasValue()) throw std::logic_error("Attempt to access error of successful Result");

        return std::get<Errors::Error>(this->data);
    }
};

template<>
class Result<void>
{
private:
    std::optional<Errors::Error> errorValue;

public:
    static Result success()
    {
        return Result();
    }

    static Result failure(Errors::Error error)
    {
        Result result;
        result.errorValue = std::move(error);
        return result;
    }

    bool hasValue() const noexcept
    {
        return !this->errorValue.has_value();
    }

    explicit operator bool() const noexcept
    {
        return this->hasValue();
    }

    void value() const
    {
        if (!this->hasValue()) throw std::logic_error("Attempt to access value of failed Result");
    }

    const Errors::Error& error() const
    {
        if (this->hasValue()) throw std::logic_error("Attempt to access error of successful Result");

        return *this->errorValue;
    }
};

}
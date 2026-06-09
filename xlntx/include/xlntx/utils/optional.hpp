#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

#include <xlntx/xlntx_config.hpp>

namespace xlntx {

template <typename T>
class optional
{
public:
    optional() : has_value_(false) {}
    optional(std::nullptr_t) : has_value_(false) {}
    optional(const T &value) : has_value_(true) { new (&storage_) T(value); }
    optional(T &&value) : has_value_(true) { new (&storage_) T(std::move(value)); }
    optional(const optional &other) : has_value_(other.has_value_)
    {
        if (has_value_) new (&storage_) T(*other);
    }
    optional(optional &&other) : has_value_(other.has_value_)
    {
        if (has_value_) new (&storage_) T(std::move(*other));
        other.reset();
    }
    ~optional() { reset(); }

    optional &operator=(const T &value)
    {
        reset();
        has_value_ = true;
        new (&storage_) T(value);
        return *this;
    }
    optional &operator=(T &&value)
    {
        reset();
        has_value_ = true;
        new (&storage_) T(std::move(value));
        return *this;
    }
    optional &operator=(std::nullptr_t)
    {
        reset();
        return *this;
    }
    optional &operator=(const optional &other)
    {
        if (this != &other)
        {
            reset();
            if (other.has_value_)
            {
                has_value_ = true;
                new (&storage_) T(*other);
            }
        }
        return *this;
    }
    optional &operator=(optional &&other)
    {
        if (this != &other)
        {
            reset();
            if (other.has_value_)
            {
                has_value_ = true;
                new (&storage_) T(std::move(*other));
                other.reset();
            }
        }
        return *this;
    }

    explicit operator bool() const { return has_value_; }
    bool is_set() const { return has_value_; }

    T &operator*() { return *reinterpret_cast<T *>(&storage_); }
    const T &operator*() const { return *reinterpret_cast<const T *>(&storage_); }
    T *operator->() { return reinterpret_cast<T *>(&storage_); }
    const T *operator->() const { return reinterpret_cast<const T *>(&storage_); }

    T &get() { return *reinterpret_cast<T *>(&storage_); }
    const T &get() const { return *reinterpret_cast<const T *>(&storage_); }

    void reset()
    {
        if (has_value_)
        {
            reinterpret_cast<T *>(&storage_)->~T();
            has_value_ = false;
        }
    }

    bool operator==(const optional &other) const
    {
        if (has_value_ != other.has_value_) return false;
        if (!has_value_) return true;
        return get() == other.get();
    }
    bool operator!=(const optional &other) const { return !(*this == other); }

private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
    bool has_value_;
};

} // namespace xlntx

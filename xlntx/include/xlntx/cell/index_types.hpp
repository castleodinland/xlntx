#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include <xlntx/xlntx_config.hpp>

namespace xlntx {

using row_t = std::uint32_t;

enum class row_or_col_t
{
    row,
    column
};

struct XLNTX_API column_t
{
    using index_t = std::uint32_t;

    column_t() : index(1) {}
    column_t(index_t idx) : index(idx) {}
    explicit column_t(const std::string &column_string);
    explicit column_t(const char *column_string);

    static index_t column_index_from_string(const std::string &column_string);
    static std::string column_string_from_index(index_t column_index);

    std::string column_string() const;
    column_t &operator=(const std::string &column_string);
    column_t &operator=(const char *column_string);

    bool operator==(const column_t &other) const { return index == other.index; }
    bool operator!=(const column_t &other) const { return index != other.index; }
    bool operator<(const column_t &other) const { return index < other.index; }
    bool operator>(const column_t &other) const { return index > other.index; }
    bool operator<=(const column_t &other) const { return index <= other.index; }
    bool operator>=(const column_t &other) const { return index >= other.index; }

    column_t &operator++() { ++index; return *this; }
    column_t operator++(int) { column_t tmp(*this); ++index; return tmp; }
    column_t &operator--() { --index; return *this; }
    column_t operator--(int) { column_t tmp(*this); --index; return tmp; }

    friend column_t operator+(column_t lhs, const column_t &rhs) { lhs.index += rhs.index; return lhs; }
    friend column_t operator-(column_t lhs, const column_t &rhs) { lhs.index -= rhs.index; return lhs; }
    column_t &operator+=(const column_t &rhs) { index += rhs.index; return *this; }
    column_t &operator-=(const column_t &rhs) { index -= rhs.index; return *this; }

    friend bool operator==(const column_t &lhs, index_t rhs) { return lhs.index == rhs; }
    friend bool operator!=(const column_t &lhs, index_t rhs) { return lhs.index != rhs; }

    index_t index;
};

struct column_hash
{
    std::size_t operator()(const column_t &k) const
    {
        return std::hash<column_t::index_t>()(k.index);
    }
};

} // namespace xlntx

namespace std {

template <>
struct hash<xlntx::column_t>
{
    std::size_t operator()(const xlntx::column_t &k) const
    {
        return hash<xlntx::column_t::index_t>()(k.index);
    }
};

} // namespace std

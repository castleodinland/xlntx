#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include <xlntx/xlntx_config.hpp>
#include <xlntx/cell/index_types.hpp>

namespace xlntx {

class cell_reference;
class range_reference;

struct cell_reference_hash
{
    std::size_t operator()(const cell_reference &k) const;
};

class XLNTX_API cell_reference
{
public:
    cell_reference();
    cell_reference(const char *reference_string);
    cell_reference(const std::string &reference_string);
    cell_reference(column_t column, row_t row);

    static std::pair<std::string, row_t> split_reference(const std::string &reference_string);
    static std::pair<std::string, row_t> split_reference(
        const std::string &reference_string, bool &absolute_column, bool &absolute_row);

    cell_reference &make_absolute(bool absolute_column = true, bool absolute_row = true);

    bool column_absolute() const { return absolute_column_; }
    void column_absolute(bool absolute_column) { absolute_column_ = absolute_column; }

    bool row_absolute() const { return absolute_row_; }
    void row_absolute(bool absolute_row) { absolute_row_ = absolute_row; }

    column_t column() const { return column_; }
    void column(const std::string &column_string);
    column_t::index_t column_index() const { return column_.index; }
    void column_index(column_t column) { column_ = column; }

    row_t row() const { return row_; }
    void row(row_t row) { row_ = row; }

    cell_reference make_offset(int column_offset, int row_offset) const;

    std::string to_string() const;

    range_reference to_range() const;

    range_reference operator,(const cell_reference &other) const;

    bool operator==(const cell_reference &comparand) const;
    bool operator!=(const cell_reference &comparand) const;
    bool operator==(const std::string &reference_string) const;
    bool operator!=(const std::string &reference_string) const;
    bool operator==(const char *reference_string) const;
    bool operator!=(const char *reference_string) const;

private:
    column_t column_;
    row_t row_ = 1;
    bool absolute_column_ = false;
    bool absolute_row_ = false;
};

} // namespace xlntx

namespace std {

template <>
struct hash<xlntx::cell_reference>
{
    std::size_t operator()(const xlntx::cell_reference &k) const
    {
        return static_cast<std::size_t>(
            (static_cast<std::uint64_t>(k.row()) << 32) | k.column_index());
    }
};

} // namespace std

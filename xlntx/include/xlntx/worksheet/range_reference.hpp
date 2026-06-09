#pragma once

#include <cstddef>
#include <string>

#include <xlntx/xlntx_config.hpp>
#include <xlntx/cell/cell_reference.hpp>

namespace xlntx {

class XLNTX_API range_reference
{
public:
    range_reference();
    range_reference(const std::string &range_string);
    range_reference(const char *range_string);
    range_reference(const cell_reference &top_left, const cell_reference &bottom_right);
    range_reference(column_t column1, row_t row1, column_t column2, row_t row2);

    static range_reference make_absolute(const range_reference &other);

    bool is_single_cell() const;
    std::size_t width() const;
    std::size_t height() const;

    cell_reference top_left() const;
    cell_reference top_right() const;
    cell_reference bottom_left() const;
    cell_reference bottom_right() const;

    range_reference make_offset(int column_offset, int row_offset) const;

    std::string to_string() const;

    bool operator==(const range_reference &other) const;
    bool operator!=(const range_reference &other) const;
    bool operator==(const std::string &other) const;
    bool operator!=(const std::string &other) const;
    bool operator==(const char *other) const;
    bool operator!=(const char *other) const;

private:
    cell_reference top_left_;
    cell_reference bottom_right_;
};

} // namespace xlntx

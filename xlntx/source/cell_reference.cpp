#include <xlntx/cell/cell_reference.hpp>
#include <xlntx/worksheet/range_reference.hpp>

#include <cctype>
#include <stdexcept>

namespace xlntx {

cell_reference::cell_reference()
    : column_(1), row_(1), absolute_column_(false), absolute_row_(false)
{
}

cell_reference::cell_reference(const char *reference_string)
    : cell_reference(std::string(reference_string))
{
}

cell_reference::cell_reference(const std::string &reference_string)
    : absolute_column_(false), absolute_row_(false)
{
    bool abs_col = false, abs_row = false;
    auto parts = split_reference(reference_string, abs_col, abs_row);
    column_ = column_t(parts.first);
    row_ = parts.second;
    absolute_column_ = abs_col;
    absolute_row_ = abs_row;
}

cell_reference::cell_reference(column_t column, row_t row)
    : column_(column), row_(row)
{
}

std::pair<std::string, row_t> cell_reference::split_reference(const std::string &reference_string)
{
    bool unused1, unused2;
    return split_reference(reference_string, unused1, unused2);
}

std::pair<std::string, row_t> cell_reference::split_reference(
    const std::string &reference_string, bool &absolute_column, bool &absolute_row)
{
    absolute_column = false;
    absolute_row = false;

    std::string col_str;
    std::string row_str;
    bool in_column = true;

    for (std::size_t i = 0; i < reference_string.size(); ++i)
    {
        char c = reference_string[i];

        if (c == '$')
        {
            if (in_column)
                absolute_column = true;
            else
                absolute_row = true;
            continue;
        }

        if (in_column && std::isdigit(static_cast<unsigned char>(c)))
        {
            in_column = false;
        }

        if (in_column)
            col_str.push_back(c);
        else
            row_str.push_back(c);
    }

    row_t row = 1;
    if (!row_str.empty())
    {
        try { row = static_cast<row_t>(std::stoul(row_str)); }
        catch (const std::exception &) { row = 1; }
    }

    return {col_str, row};
}

cell_reference &cell_reference::make_absolute(bool absolute_column, bool absolute_row)
{
    absolute_column_ = absolute_column;
    absolute_row_ = absolute_row;
    return *this;
}

void cell_reference::column(const std::string &column_string)
{
    column_ = column_t(column_string);
}

cell_reference cell_reference::make_offset(int column_offset, int row_offset) const
{
    auto new_column = column_.index;
    if (column_offset < 0)
    {
        if (static_cast<column_t::index_t>(-column_offset) > new_column)
            throw std::out_of_range("column offset out of range");
        new_column -= static_cast<column_t::index_t>(-column_offset);
    }
    else
    {
        new_column += static_cast<column_t::index_t>(column_offset);
    }

    auto new_row = static_cast<std::int64_t>(row_) + row_offset;
    if (new_row <= 0)
        throw std::out_of_range("row offset out of range");

    return cell_reference(column_t(new_column), static_cast<row_t>(new_row));
}

std::string cell_reference::to_string() const
{
    std::string result;
    if (absolute_column_) result.push_back('$');
    result += column_.column_string();
    if (absolute_row_) result.push_back('$');
    result += std::to_string(row_);
    return result;
}

range_reference cell_reference::to_range() const
{
    return range_reference(*this, *this);
}

range_reference cell_reference::operator,(const cell_reference &other) const
{
    return range_reference(*this, other);
}

bool cell_reference::operator==(const cell_reference &comparand) const
{
    return row_ == comparand.row_ && column_.index == comparand.column_.index;
}

bool cell_reference::operator!=(const cell_reference &comparand) const
{
    return !(*this == comparand);
}

bool cell_reference::operator==(const std::string &reference_string) const
{
    return *this == cell_reference(reference_string);
}

bool cell_reference::operator!=(const std::string &reference_string) const
{
    return !(*this == reference_string);
}

bool cell_reference::operator==(const char *reference_string) const
{
    return *this == std::string(reference_string);
}

bool cell_reference::operator!=(const char *reference_string) const
{
    return !(*this == reference_string);
}

std::size_t cell_reference_hash::operator()(const cell_reference &k) const
{
    return std::hash<cell_reference>()(k);
}

} // namespace xlntx

#include <xlntx/worksheet/range_reference.hpp>
#include <stdexcept>

namespace xlntx {

range_reference::range_reference()
    : top_left_("A1"), bottom_right_("A1")
{
}

range_reference::range_reference(const std::string &range_string)
{
    auto colon_pos = range_string.find(':');
    if (colon_pos == std::string::npos)
    {
        top_left_ = cell_reference(range_string);
        bottom_right_ = top_left_;
    }
    else
    {
        top_left_ = cell_reference(range_string.substr(0, colon_pos));
        bottom_right_ = cell_reference(range_string.substr(colon_pos + 1));
    }
}

range_reference::range_reference(const char *range_string)
    : range_reference(std::string(range_string))
{
}

range_reference::range_reference(const cell_reference &top_left, const cell_reference &bottom_right)
    : top_left_(top_left), bottom_right_(bottom_right)
{
}

range_reference::range_reference(column_t column1, row_t row1, column_t column2, row_t row2)
    : top_left_(column1, row1), bottom_right_(column2, row2)
{
}

range_reference range_reference::make_absolute(const range_reference &other)
{
    return range_reference(
        cell_reference(other.top_left()).make_absolute(),
        cell_reference(other.bottom_right()).make_absolute());
}

bool range_reference::is_single_cell() const
{
    return top_left_ == bottom_right_;
}

std::size_t range_reference::width() const
{
    return static_cast<std::size_t>(
        bottom_right_.column_index() >= top_left_.column_index()
            ? bottom_right_.column_index() - top_left_.column_index() + 1
            : top_left_.column_index() - bottom_right_.column_index() + 1);
}

std::size_t range_reference::height() const
{
    return static_cast<std::size_t>(
        bottom_right_.row() >= top_left_.row()
            ? bottom_right_.row() - top_left_.row() + 1
            : top_left_.row() - bottom_right_.row() + 1);
}

cell_reference range_reference::top_left() const { return top_left_; }
cell_reference range_reference::top_right() const
{
    return cell_reference(bottom_right_.column(), top_left_.row());
}
cell_reference range_reference::bottom_left() const
{
    return cell_reference(top_left_.column(), bottom_right_.row());
}
cell_reference range_reference::bottom_right() const { return bottom_right_; }

range_reference range_reference::make_offset(int column_offset, int row_offset) const
{
    return range_reference(
        top_left_.make_offset(column_offset, row_offset),
        bottom_right_.make_offset(column_offset, row_offset));
}

std::string range_reference::to_string() const
{
    if (is_single_cell())
        return top_left_.to_string();
    return top_left_.to_string() + ":" + bottom_right_.to_string();
}

bool range_reference::operator==(const range_reference &other) const
{
    return top_left_ == other.top_left_ && bottom_right_ == other.bottom_right_;
}
bool range_reference::operator!=(const range_reference &other) const
{
    return !(*this == other);
}
bool range_reference::operator==(const std::string &other) const
{
    return *this == range_reference(other);
}
bool range_reference::operator!=(const std::string &other) const
{
    return !(*this == other);
}
bool range_reference::operator==(const char *other) const
{
    return *this == std::string(other);
}
bool range_reference::operator!=(const char *other) const
{
    return !(*this == other);
}

} // namespace xlntx

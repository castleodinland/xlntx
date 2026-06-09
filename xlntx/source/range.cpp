#include <xlntx/worksheet/range.hpp>
#include <xlntx/worksheet/worksheet.hpp>
#include <xlntx/worksheet/cell_vector.hpp>
#include <xlntx/cell/cell.hpp>
#include "detail/worksheet_impl.hpp"
#include "detail/cell_impl.hpp"

#include <stdexcept>

namespace xlntx {

// ===== cell_vector =====

cell_vector::cell_vector(worksheet &ws, row_t row)
    : ws_(&ws), const_ws_(&ws), row_(row)
{
}

cell_vector::cell_vector(const worksheet &ws, row_t row)
    : ws_(const_cast<worksheet *>(&ws)), const_ws_(&ws), row_(row)
{
}

class cell cell_vector::operator[](std::size_t cell_index)
{
    if (!ws_) throw std::runtime_error("null worksheet");
    return ws_->cell(cell_reference(
        column_t(static_cast<column_t::index_t>(cell_index + 1)), row_));
}

const class cell cell_vector::operator[](std::size_t cell_index) const
{
    if (!const_ws_) throw std::runtime_error("null worksheet");
    return const_ws_->cell(cell_reference(
        column_t(static_cast<column_t::index_t>(cell_index + 1)), row_));
}

class cell cell_vector::front()
{
    return (*this)[0];
}

const class cell cell_vector::front() const
{
    return (*this)[0];
}

class cell cell_vector::back()
{
    // Return from high column - find highest in this row
    std::size_t max_col = 0;
    if (ws_)
    {
        for (int c = 16384; c >= 1; --c)
        {
            if (ws_->has_cell(cell_reference(column_t(static_cast<column_t::index_t>(c)), row_)))
            {
                max_col = static_cast<std::size_t>(c - 1);
                break;
            }
        }
    }
    return (*this)[max_col];
}

const class cell cell_vector::back() const
{
    return const_cast<cell_vector *>(this)->back();
}

std::size_t cell_vector::length() const
{
    if (!const_ws_) return 0;
    return static_cast<std::size_t>(const_ws_->highest_column().index);
}

bool cell_vector::operator==(const cell_vector &other) const
{
    return ws_ == other.ws_ && row_ == other.row_;
}

bool cell_vector::operator!=(const cell_vector &other) const
{
    return !(*this == other);
}

// ===== range_iterator =====

range_iterator::range_iterator() : ws_(nullptr), row_(0), skip_null_(true), max_row_(0) {}

range_iterator::range_iterator(worksheet &ws, row_t row, bool skip_null)
    : ws_(&ws), row_(row), skip_null_(skip_null), max_row_(ws.highest_row())
{
    if (skip_null_ && ws_) advance_to_valid();
}

void range_iterator::advance_to_valid()
{
    if (!ws_) return;
    while (row_ <= max_row_)
    {
        // Check if this row has any cells
        bool has_cells = false;
        for (column_t::index_t c = 1; c <= 16384; ++c)
        {
            if (ws_->has_cell(cell_reference(column_t(c), row_)))
            {
                has_cells = true;
                break;
            }
        }
        if (has_cells) break;
        ++row_;
    }
}

cell_vector range_iterator::operator*()
{
    if (!ws_) throw std::runtime_error("null worksheet in iterator");
    return cell_vector(*ws_, row_);
}

range_iterator &range_iterator::operator++()
{
    ++row_;
    if (skip_null_ && ws_) advance_to_valid();
    return *this;
}

range_iterator range_iterator::operator++(int)
{
    range_iterator tmp(*this);
    ++(*this);
    return tmp;
}

bool range_iterator::operator==(const range_iterator &other) const
{
    return ws_ == other.ws_ && row_ == other.row_;
}

bool range_iterator::operator!=(const range_iterator &other) const
{
    return !(*this == other);
}

// ===== const_range_iterator =====

const_range_iterator::const_range_iterator() : ws_(nullptr), row_(0), skip_null_(true), max_row_(0) {}

const_range_iterator::const_range_iterator(const worksheet &ws, row_t row, bool skip_null)
    : ws_(&ws), row_(row), skip_null_(skip_null), max_row_(ws.highest_row())
{
    if (skip_null_ && ws_) advance_to_valid();
}

void const_range_iterator::advance_to_valid()
{
    if (!ws_) return;
    while (row_ <= max_row_)
    {
        bool has_cells = false;
        for (column_t::index_t c = 1; c <= 16384; ++c)
        {
            if (ws_->has_cell(cell_reference(column_t(c), row_)))
            {
                has_cells = true;
                break;
            }
        }
        if (has_cells) break;
        ++row_;
    }
}

const cell_vector const_range_iterator::operator*()
{
    if (!ws_) throw std::runtime_error("null worksheet in iterator");
    return cell_vector(*ws_, row_);
}

const_range_iterator &const_range_iterator::operator++()
{
    ++row_;
    if (skip_null_ && ws_) advance_to_valid();
    return *this;
}

const_range_iterator const_range_iterator::operator++(int)
{
    const_range_iterator tmp(*this);
    ++(*this);
    return tmp;
}

bool const_range_iterator::operator==(const const_range_iterator &other) const
{
    return ws_ == other.ws_ && row_ == other.row_;
}

bool const_range_iterator::operator!=(const const_range_iterator &other) const
{
    return !(*this == other);
}

// ===== range =====

range::range(worksheet ws, const range_reference &reference,
             major_order order, bool skip_null)
    : ws_(ws), ref_(reference), order_(order), skip_null_(skip_null)
{
}

void range::clear_cells()
{
    if (ref_.is_single_cell())
    {
        ws_.clear_cell(ref_.top_left());
    }
    else
    {
        for (row_t r = ref_.top_left().row(); r <= ref_.bottom_right().row(); ++r)
            ws_.clear_row(r);
    }
}

cell_vector range::vector(std::size_t n)
{
    return cell_vector(ws_, ref_.top_left().row() + static_cast<row_t>(n));
}

const cell_vector range::vector(std::size_t n) const
{
    return cell_vector(ws_, ref_.top_left().row() + static_cast<row_t>(n));
}

class cell range::cell(const cell_reference &ref)
{
    return ws_.cell(ref);
}

const class cell range::cell(const cell_reference &ref) const
{
    return ws_.cell(ref);
}

const worksheet &range::target_worksheet() const { return ws_; }
range_reference range::reference() const { return ref_; }

std::size_t range::length() const
{
    return static_cast<std::size_t>(ref_.height());
}

bool range::contains(const cell_reference &ref)
{
    return ref.column_index() >= ref_.top_left().column_index() &&
           ref.column_index() <= ref_.bottom_right().column_index() &&
           ref.row() >= ref_.top_left().row() &&
           ref.row() <= ref_.bottom_right().row();
}

cell_vector range::front() { return cell_vector(ws_, ref_.top_left().row()); }
const cell_vector range::front() const { return cell_vector(ws_, ref_.top_left().row()); }
cell_vector range::back() { return cell_vector(ws_, ref_.bottom_right().row()); }
const cell_vector range::back() const { return cell_vector(ws_, ref_.bottom_right().row()); }

range::iterator range::begin()
{
    return range_iterator(ws_, ref_.top_left().row(), skip_null_);
}

range::iterator range::end()
{
    range_iterator it;
    it = range_iterator(); // represents past-the-end
    // Create a proper past-the-end iterator
    range_iterator end_it(ws_, ref_.bottom_right().row() + 1, false);
    return end_it;
}

range::const_iterator range::begin() const { return cbegin(); }
range::const_iterator range::end() const { return cend(); }

range::const_iterator range::cbegin() const
{
    return const_range_iterator(ws_, ref_.top_left().row(), skip_null_);
}

range::const_iterator range::cend() const
{
    return const_range_iterator(ws_, ref_.bottom_right().row() + 1, false);
}

cell_vector range::operator[](std::size_t n)
{
    return cell_vector(ws_, ref_.top_left().row() + static_cast<row_t>(n));
}

const cell_vector range::operator[](std::size_t n) const
{
    return cell_vector(ws_, ref_.top_left().row() + static_cast<row_t>(n));
}

bool range::operator==(const range &other) const
{
    return ref_ == other.ref_ && skip_null_ == other.skip_null_ && order_ == other.order_;
}

bool range::operator!=(const range &other) const
{
    return !(*this == other);
}

void range::apply(std::function<void(class cell)> f)
{
    for (row_t r = ref_.top_left().row(); r <= ref_.bottom_right().row(); ++r)
    {
        for (column_t::index_t c = ref_.top_left().column_index();
             c <= ref_.bottom_right().column_index(); ++c)
        {
            cell_reference ref(column_t(c), r);
            if (ws_.has_cell(ref))
                f(ws_.cell(ref));
        }
    }
}

} // namespace xlntx

#include <xlntx/worksheet/worksheet.hpp>
#include <xlntx/worksheet/range.hpp>
#include <xlntx/worksheet/range_reference.hpp>
#include <xlntx/worksheet/cell_vector.hpp>
#include <xlntx/worksheet/column_properties.hpp>
#include <xlntx/worksheet/row_properties.hpp>
#include <xlntx/worksheet/sheet_view.hpp>
#include <xlntx/worksheet/header_footer.hpp>
#include <xlntx/worksheet/sheet_format_properties.hpp>
#include <xlntx/cell/cell.hpp>
#include <xlntx/cell/cell_reference.hpp>
#include <xlntx/utils/exceptions.hpp>
#include <xlntx/utils/path.hpp>
#include <xlntx/packaging/relationship.hpp>
#include <xlntx/styles/conditional_format.hpp>
#include "detail/worksheet_impl.hpp"
#include "detail/workbook_impl.hpp"
#include "detail/cell_impl.hpp"

#include <algorithm>

namespace xlntx {

worksheet::worksheet() : d_(nullptr) {}
worksheet::worksheet(const worksheet &rhs) : d_(rhs.d_) {}
worksheet::worksheet(detail::worksheet_impl *d) : d_(d) {}

class workbook &worksheet::workbook() { throw key_not_found(); }
const class workbook &worksheet::workbook() const { throw key_not_found(); }

void worksheet::garbage_collect() {}

std::size_t worksheet::id() const { return d_ ? d_->id_ : 0; }
void worksheet::id(std::size_t id) { if (d_) d_->id_ = id; }

std::string worksheet::title() const { return d_ ? d_->title_ : ""; }
void worksheet::title(const std::string &title) { if (d_) d_->title_ = title; }

bool worksheet::has_cell(const cell_reference &reference) const
{
    return d_ && d_->cell_map_.find(reference) != d_->cell_map_.end();
}

class cell worksheet::cell(const cell_reference &reference)
{
    if (!d_) throw invalid_cell_reference(reference.to_string());
    auto it = d_->cell_map_.find(reference);
    if (it != d_->cell_map_.end())
        return class cell(&it->second);
    // Insert a new empty cell
    detail::cell_impl ci;
    ci.parent_ = d_;
    ci.column_ = reference.column_index();
    ci.row_ = reference.row();
    auto result = d_->cell_map_.emplace(reference, std::move(ci));
    return class cell(&result.first->second);
}

const class cell worksheet::cell(const cell_reference &reference) const
{
    if (!d_) throw invalid_cell_reference(reference.to_string());
    auto it = d_->cell_map_.find(reference);
    if (it != d_->cell_map_.end())
        return class cell(const_cast<detail::cell_impl *>(&it->second));
    throw invalid_cell_reference(reference.to_string());
}

class cell worksheet::cell(column_t column, row_t row)
{
    return cell(cell_reference(column, row));
}

const class cell worksheet::cell(column_t column, row_t row) const
{
    return cell(cell_reference(column, row));
}

class range worksheet::rows(bool skip_null)
{
    if (!d_) return class range(*this, range_reference(), major_order::row, skip_null);

    // Calculate the range covering all data rows
    row_t min_row = 1, max_row = 1;
    if (!d_->cell_map_.empty())
    {
        min_row = 0xFFFFFFFF;
        max_row = 0;
        for (const auto &pair : d_->cell_map_)
        {
            row_t r = pair.first.row();
            if (r < min_row) min_row = r;
            if (r > max_row) max_row = r;
        }
    }

    if (max_row < min_row)
        return class range(*this, range_reference(), major_order::row, skip_null);

    return class range(*this,
        range_reference(column_t(1), min_row, column_t(16384), max_row),
        major_order::row, skip_null);
}

const class range worksheet::rows(bool skip_null) const
{
    // Const version - just delegate to non-const for now
    return const_cast<worksheet *>(this)->rows(skip_null);
}

class range worksheet::columns(bool skip_null)
{
    return rows(skip_null);
}

const class range worksheet::columns(bool skip_null) const
{
    return rows(skip_null);
}

void worksheet::clear_cell(const cell_reference &ref)
{
    if (d_) d_->cell_map_.erase(ref);
}

void worksheet::clear_row(row_t row)
{
    if (!d_) return;
    auto it = d_->cell_map_.begin();
    while (it != d_->cell_map_.end())
    {
        if (it->first.row() == row)
            it = d_->cell_map_.erase(it);
        else
            ++it;
    }
}

row_t worksheet::lowest_row() const
{
    if (!d_ || d_->cell_map_.empty()) return 1;
    row_t min = 0xFFFFFFFF;
    for (const auto &p : d_->cell_map_)
        if (p.first.row() < min) min = p.first.row();
    return min;
}

row_t worksheet::lowest_row_or_props() const { return lowest_row(); }

row_t worksheet::highest_row() const
{
    if (!d_ || d_->cell_map_.empty()) return 1;
    row_t max = 0;
    for (const auto &p : d_->cell_map_)
        if (p.first.row() > max) max = p.first.row();
    return max;
}

row_t worksheet::highest_row_or_props() const { return highest_row(); }
row_t worksheet::next_row() const { return highest_row() + 1; }

column_t worksheet::lowest_column() const
{
    if (!d_ || d_->cell_map_.empty()) return column_t(1);
    column_t::index_t min = 0xFFFFFFFF;
    for (const auto &p : d_->cell_map_)
        if (p.first.column_index() < min) min = p.first.column_index();
    return column_t(min);
}

column_t worksheet::highest_column() const
{
    if (!d_ || d_->cell_map_.empty()) return column_t(1);
    column_t::index_t max = 0;
    for (const auto &p : d_->cell_map_)
        if (p.first.column_index() > max) max = p.first.column_index();
    return column_t(max);
}

column_t worksheet::lowest_column_or_props() const { return lowest_column(); }
column_t worksheet::highest_column_or_props() const { return highest_column(); }

range_reference worksheet::calculate_dimension() const
{
    return range_reference(lowest_column(), lowest_row(),
                           highest_column(), highest_row());
}

void worksheet::merge_cells(const std::string &ref) { merge_cells(range_reference(ref)); }
void worksheet::merge_cells(const range_reference &ref)
{
    if (d_) d_->merged_cells_.push_back(ref);
}
void worksheet::unmerge_cells(const std::string &ref) { unmerge_cells(range_reference(ref)); }
void worksheet::unmerge_cells(const range_reference &)
{
    // stub
}
std::vector<range_reference> worksheet::merged_ranges() const
{
    return d_ ? d_->merged_cells_ : std::vector<range_reference>();
}

bool worksheet::operator==(const worksheet &other) const { return d_ == other.d_; }
bool worksheet::operator!=(const worksheet &other) const { return d_ != other.d_; }
bool worksheet::operator==(std::nullptr_t) const { return d_ == nullptr; }
bool worksheet::operator!=(std::nullptr_t) const { return d_ != nullptr; }
void worksheet::operator=(const worksheet &other) { d_ = other.d_; }

class cell worksheet::operator[](const cell_reference &reference)
{
    return cell(reference);
}

const class cell worksheet::operator[](const cell_reference &reference) const
{
    return cell(reference);
}

bool worksheet::compare(const worksheet &other, bool ref) const
{
    return ref ? (d_ == other.d_) : (*this == other);
}

// Stub methods
cell_reference worksheet::frozen_panes() const { return cell_reference(); }
void worksheet::freeze_panes(class cell) {}
void worksheet::freeze_panes(const cell_reference &) {}
void worksheet::unfreeze_panes() {}
bool worksheet::has_frozen_panes() const { return false; }

class range worksheet::range(const std::string &rs)
{
    return class range(*this, range_reference(rs));
}
const class range worksheet::range(const std::string &rs) const
{
    return const_cast<worksheet *>(this)->range(rs);
}
class range worksheet::range(const range_reference &ref)
{
    return class range(*this, ref);
}
const class range worksheet::range(const range_reference &ref) const
{
    return const_cast<worksheet *>(this)->range(ref);
}

void worksheet::insert_rows(row_t, std::uint32_t) {}
void worksheet::insert_columns(column_t, std::uint32_t) {}
void worksheet::delete_rows(row_t, std::uint32_t) {}
void worksheet::delete_columns(column_t, std::uint32_t) {}

xlntx::column_properties &worksheet::column_properties(column_t) { static xlntx::column_properties cp; return cp; }
const xlntx::column_properties &worksheet::column_properties(column_t) const { static xlntx::column_properties cp; return cp; }
bool worksheet::has_column_properties(column_t) const { return false; }
void worksheet::add_column_properties(column_t, const class column_properties &) {}
double worksheet::column_width(column_t) const { return 8.0; }

xlntx::row_properties &worksheet::row_properties(row_t) { static xlntx::row_properties rp; return rp; }
const xlntx::row_properties &worksheet::row_properties(row_t) const { static xlntx::row_properties rp; return rp; }
bool worksheet::has_row_properties(row_t) const { return false; }
void worksheet::add_row_properties(row_t, const class row_properties &) {}
double worksheet::row_height(row_t) const { return 15.0; }

cell_reference worksheet::point_pos(int, int) const { return cell_reference(); }
void worksheet::create_named_range(const std::string &, const std::string &) {}
void worksheet::create_named_range(const std::string &, const range_reference &) {}
bool worksheet::has_named_range(const std::string &) const { return false; }
class range worksheet::named_range(const std::string &) { return class range(*this, range_reference()); }
const class range worksheet::named_range(const std::string &) const { return class range(*this, range_reference()); }
void worksheet::remove_named_range(const std::string &) {}

bool worksheet::has_page_setup() const { return false; }
xlntx::page_setup worksheet::page_setup() const { return xlntx::page_setup(); }
void worksheet::page_setup(const struct page_setup &) {}

bool worksheet::has_page_margins() const { return false; }
xlntx::page_margins worksheet::page_margins() const { return xlntx::page_margins(); }
void worksheet::page_margins(const class page_margins &) {}

range_reference worksheet::auto_filter() const { return range_reference(); }
void worksheet::auto_filter(const std::string &) {}
void worksheet::auto_filter(const xlntx::range &) {}
void worksheet::auto_filter(const range_reference &) {}
void worksheet::clear_auto_filter() {}
bool worksheet::has_auto_filter() const { return false; }

void worksheet::reserve(std::size_t) {}
bool worksheet::has_phonetic_properties() const { return false; }
const phonetic_pr &worksheet::phonetic_properties() const { static phonetic_pr pp; return pp; }
void worksheet::phonetic_properties(const phonetic_pr &) {}
bool worksheet::has_header_footer() const { return false; }
class header_footer worksheet::header_footer() const { return header_footer(); }
void worksheet::header_footer(const class header_footer &) {}

xlntx::sheet_state worksheet::sheet_state() const { return d_ ? d_->sheet_state_ : sheet_state::visible; }
void worksheet::sheet_state(xlntx::sheet_state s) { if (d_) d_->sheet_state_ = s; }

worksheet::iterator worksheet::begin() { return rows().begin(); }
worksheet::iterator worksheet::end() { return rows().end(); }
worksheet::const_iterator worksheet::begin() const { return cbegin(); }
worksheet::const_iterator worksheet::end() const { return cend(); }
worksheet::const_iterator worksheet::cbegin() const { return rows().cbegin(); }
worksheet::const_iterator worksheet::cend() const { return rows().cend(); }

void worksheet::print_title_rows(row_t, row_t) {}
void worksheet::print_title_rows(row_t) {}
void worksheet::print_title_cols(column_t, column_t) {}
void worksheet::print_title_cols(column_t) {}
std::string worksheet::print_titles() const { return ""; }
void worksheet::print_area(const std::string &) {}
range_reference worksheet::print_area() const { return range_reference(); }

bool worksheet::has_view() const { return false; }
sheet_view &worksheet::view(std::size_t) const { static sheet_view sv; return sv; }
void worksheet::add_view(const sheet_view &) {}
void worksheet::active_cell(const cell_reference &) {}
bool worksheet::has_active_cell() const { return false; }
cell_reference worksheet::active_cell() const { return cell_reference(); }

void worksheet::clear_page_breaks() {}
const std::vector<row_t> &worksheet::page_break_rows() const { static std::vector<row_t> v; return v; }
void worksheet::page_break_at_row(row_t) {}
const std::vector<column_t> &worksheet::page_break_columns() const { static std::vector<column_t> v; return v; }
void worksheet::page_break_at_column(column_t) {}

xlntx::conditional_format worksheet::conditional_format(const range_reference &, const condition &) { return xlntx::conditional_format(); }
xlntx::path worksheet::path() const { return xlntx::path(); }
relationship worksheet::referring_relationship() const { return relationship(); }
sheet_format_properties worksheet::format_properties() const { return sheet_format_properties(); }
void worksheet::format_properties(const sheet_format_properties &) {}
bool worksheet::has_drawing() const { return d_ && d_->has_drawing_; }

void worksheet::register_comments_in_manifest() {}
void worksheet::register_calc_chain_in_manifest() {}
void worksheet::garbage_collect_formulae() {}
void worksheet::parent(class workbook &) {}
void worksheet::move_cells(std::uint32_t, std::uint32_t, row_or_col_t, bool) {}

} // namespace xlntx

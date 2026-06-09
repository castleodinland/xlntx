#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <xlntx/xlntx_config.hpp>
#include <xlntx/cell/index_types.hpp>
#include <xlntx/worksheet/page_margins.hpp>
#include <xlntx/worksheet/page_setup.hpp>
#include <xlntx/worksheet/sheet_view.hpp>

namespace xlntx {

class cell;
class cell_reference;
class cell_vector;
class column_properties;
class comment;
class condition;
class conditional_format;
class const_range_iterator;
class footer;
class header;
class header_footer;
class path;
class range;
class range_iterator;
class range_reference;
class relationship;
class row_properties;
class sheet_format_properties;
class workbook;
class phonetic_pr;

struct date;

namespace detail {
struct worksheet_impl;
class xlsx_reader;
}

class XLNTX_API worksheet
{
public:
    using iterator = range_iterator;
    using const_iterator = const_range_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    worksheet();
    worksheet(const worksheet &rhs);

    class workbook &workbook();
    const class workbook &workbook() const;

    void garbage_collect();

    std::size_t id() const;
    void id(std::size_t id);

    std::string title() const;
    void title(const std::string &title);

    cell_reference frozen_panes() const;
    void freeze_panes(class cell top_left_cell);
    void freeze_panes(const cell_reference &top_left_coordinate);
    void unfreeze_panes();
    bool has_frozen_panes() const;

    bool has_cell(const cell_reference &reference) const;
    class cell cell(const cell_reference &reference);
    const class cell cell(const cell_reference &reference) const;
    class cell cell(column_t column, row_t row);
    const class cell cell(column_t column, row_t row) const;

    class range range(const std::string &reference_string);
    const class range range(const std::string &reference_string) const;
    class range range(const range_reference &reference);
    const class range range(const range_reference &reference) const;
    class range rows(bool skip_null = true);
    const class range rows(bool skip_null = true) const;
    class range columns(bool skip_null = true);
    const class range columns(bool skip_null = true) const;

    void clear_cell(const cell_reference &ref);
    void clear_row(row_t row);
    void insert_rows(row_t row, std::uint32_t amount);
    void insert_columns(column_t column, std::uint32_t amount);
    void delete_rows(row_t row, std::uint32_t amount);
    void delete_columns(column_t column, std::uint32_t amount);

    xlntx::column_properties &column_properties(column_t column);
    const xlntx::column_properties &column_properties(column_t column) const;
    bool has_column_properties(column_t column) const;
    void add_column_properties(column_t column, const class column_properties &props);
    double column_width(column_t column) const;

    xlntx::row_properties &row_properties(row_t row);
    const xlntx::row_properties &row_properties(row_t row) const;
    bool has_row_properties(row_t row) const;
    void add_row_properties(row_t row, const class row_properties &props);
    double row_height(row_t row) const;

    cell_reference point_pos(int left, int top) const;

    void create_named_range(const std::string &name, const std::string &reference_string);
    void create_named_range(const std::string &name, const range_reference &reference);
    bool has_named_range(const std::string &name) const;
    class range named_range(const std::string &name);
    const class range named_range(const std::string &name) const;
    void remove_named_range(const std::string &name);

    row_t lowest_row() const;
    row_t lowest_row_or_props() const;
    row_t highest_row() const;
    row_t highest_row_or_props() const;
    row_t next_row() const;
    column_t lowest_column() const;
    column_t lowest_column_or_props() const;
    column_t highest_column() const;
    column_t highest_column_or_props() const;
    range_reference calculate_dimension() const;

    void merge_cells(const std::string &reference_string);
    void merge_cells(const range_reference &reference);
    void unmerge_cells(const std::string &reference_string);
    void unmerge_cells(const range_reference &reference);
    std::vector<range_reference> merged_ranges() const;

    bool has_page_setup() const;
    xlntx::page_setup page_setup() const;
    void page_setup(const struct page_setup &setup);

    bool has_page_margins() const;
    xlntx::page_margins page_margins() const;
    void page_margins(const class page_margins &margins);

    range_reference auto_filter() const;
    void auto_filter(const std::string &range_string);
    void auto_filter(const xlntx::range &range);
    void auto_filter(const range_reference &reference);
    void clear_auto_filter();
    bool has_auto_filter() const;

    void reserve(std::size_t n);

    bool has_phonetic_properties() const;
    const phonetic_pr &phonetic_properties() const;
    void phonetic_properties(const phonetic_pr &phonetic_props);

    bool has_header_footer() const;
    class header_footer header_footer() const;
    void header_footer(const class header_footer &new_header_footer);

    xlntx::sheet_state sheet_state() const;
    void sheet_state(xlntx::sheet_state state);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    void print_title_rows(row_t first_row, row_t last_row);
    void print_title_rows(row_t last_row);
    void print_title_cols(column_t first_column, column_t last_column);
    void print_title_cols(column_t last_column);
    std::string print_titles() const;
    void print_area(const std::string &print_area);
    range_reference print_area() const;

    bool has_view() const;
    sheet_view &view(std::size_t index = 0) const;
    void add_view(const sheet_view &new_view);
    void active_cell(const cell_reference &ref);
    bool has_active_cell() const;
    cell_reference active_cell() const;

    void clear_page_breaks();
    const std::vector<row_t> &page_break_rows() const;
    void page_break_at_row(row_t row);
    const std::vector<column_t> &page_break_columns() const;
    void page_break_at_column(column_t column);

    xlntx::conditional_format conditional_format(const range_reference &ref, const condition &when);
    xlntx::path path() const;
    relationship referring_relationship() const;
    sheet_format_properties format_properties() const;
    void format_properties(const sheet_format_properties &properties);

    bool has_drawing() const;

    bool operator==(const worksheet &other) const;
    bool operator!=(const worksheet &other) const;
    bool operator==(std::nullptr_t) const;
    bool operator!=(std::nullptr_t) const;
    void operator=(const worksheet &other);
    class cell operator[](const cell_reference &reference);
    const class cell operator[](const cell_reference &reference) const;
    bool compare(const worksheet &other, bool reference) const;

private:
    friend class cell;
    friend class workbook;
    friend class detail::xlsx_reader;

    worksheet(detail::worksheet_impl *d);
    void register_comments_in_manifest();
    void register_calc_chain_in_manifest();
    void garbage_collect_formulae();
    void parent(class workbook &wb);
    void move_cells(std::uint32_t index, std::uint32_t amount, row_or_col_t row_or_col, bool reverse = false);

    detail::worksheet_impl *d_ = nullptr;
};

} // namespace xlntx

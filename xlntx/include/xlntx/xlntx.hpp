// xlntx — drop-in read-only replacement for xlnt (single-header distribution)
// Usage: #include "xlntx.hpp" + link xlntx.lib
// No other headers are needed. No external dependencies beyond the C++17 STL.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// ============================================================================
// Config
// ============================================================================
#ifndef XLNTX_STATIC
#define XLNTX_STATIC
#endif
#ifdef XLNTX_STATIC
#define XLNTX_API
#else
#ifdef _MSC_VER
#define XLNTX_API __declspec(dllimport)
#else
#define XLNTX_API
#endif
#endif

namespace xlntx {

// ============================================================================
// Forward declarations (all types, for dependency resolution)
// ============================================================================
class alignment;
class border;
class calculation_properties;
class cell;
class cell_reference;
class cell_vector;
class color;
class column_properties;
class comment;
class condition;
class conditional_format;
class const_range_iterator;
class document_security;
class ext_list;
class external_book;
class fill;
class font;
class footer;
class format;
class header;
class header_footer;
class hyperlink;
class manifest;
class metadata_property;
class named_range;
class number_format;
class page_margins;
class pane;
class path;
class phonetic_pr;
class print_options;
class protection;
class range;
class range_iterator;
class range_reference;
class relationship;
class row_properties;
class selection;
class sheet_format_properties;
class sheet_pr;
class sheet_protection;
class sheet_view;
class spreadsheet_drawing;
class streaming_workbook_reader;
class streaming_workbook_writer;
class style;
class theme;
class uri;
class variant;
class workbook;
class workbook_view;
class worksheet;
class worksheet_iterator;
class const_worksheet_iterator;
struct date;
struct datetime;
struct time;
struct timedelta;
enum class calendar;
enum class core_property;
enum class extended_property;

} // namespace xlntx

// ============================================================================
// index_types (has std::hash specializations that must go outside namespace)
// ============================================================================
namespace xlntx {

using row_t = std::uint32_t;

enum class row_or_col_t { row, column };

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

struct column_hash { std::size_t operator()(const column_t &k) const { return std::hash<column_t::index_t>()(k.index); } };

} // namespace xlntx

namespace std {
template <> struct hash<xlntx::column_t> { std::size_t operator()(const xlntx::column_t &k) const { return hash<xlntx::column_t::index_t>()(k.index); } };
} // namespace std

namespace xlntx {

// ============================================================================
// cell_type
// ============================================================================
enum class cell_type { empty, boolean, date, error, inline_string, number, shared_string, formula_string };

// ============================================================================
// calendar
// ============================================================================
enum class calendar { windows_1900, mac_1904 };

// ============================================================================
// Empty struct types (datetime, date, time, timedelta)
// ============================================================================
struct datetime {};
struct date {};
struct time {};
struct timedelta {};

// ============================================================================
// numeric, scoped_enum_hash (empty placeholders)
// ============================================================================
} // namespace xlntx

// ============================================================================
// exceptions
// ============================================================================
namespace xlntx {
class XLNTX_API exception : public std::runtime_error {
public:
    exception(const std::string &message);
    virtual ~exception();
    void message(const std::string &message);
private:
    std::string message_;
};
class XLNTX_API invalid_parameter : public exception { public: invalid_parameter(); };
class XLNTX_API invalid_sheet_title : public exception { public: invalid_sheet_title(const std::string &title); };
class XLNTX_API missing_number_format : public exception { public: missing_number_format(); };
class XLNTX_API invalid_file : public exception { public: invalid_file(const std::string &filename); };
class XLNTX_API illegal_character : public exception { public: illegal_character(char c); };
class XLNTX_API invalid_data_type : public exception { public: invalid_data_type(); };
class XLNTX_API invalid_column_index : public exception { public: invalid_column_index(); };
class XLNTX_API invalid_cell_reference : public exception {
public:
    invalid_cell_reference(column_t column, row_t row);
    invalid_cell_reference(const std::string &reference_string);
};
class XLNTX_API invalid_attribute : public exception { public: invalid_attribute(); };
class XLNTX_API key_not_found : public exception { public: key_not_found(); };
class XLNTX_API no_visible_worksheets : public exception { public: no_visible_worksheets(); };
class XLNTX_API unhandled_switch_case : public exception { public: unhandled_switch_case(); };
class XLNTX_API unsupported : public exception { public: unsupported(const std::string &message); };
} // namespace xlntx

// ============================================================================
// optional<T>
// ============================================================================
namespace xlntx {
template <typename T>
class optional {
public:
    optional() : has_value_(false) {}
    optional(std::nullptr_t) : has_value_(false) {}
    optional(const T &value) : has_value_(true) { new (&storage_) T(value); }
    optional(T &&value) : has_value_(true) { new (&storage_) T(std::move(value)); }
    optional(const optional &other) : has_value_(other.has_value_) { if (has_value_) new (&storage_) T(*other); }
    optional(optional &&other) : has_value_(other.has_value_) { if (has_value_) new (&storage_) T(std::move(*other)); other.reset(); }
    ~optional() { reset(); }
    optional &operator=(const T &value) { reset(); has_value_ = true; new (&storage_) T(value); return *this; }
    optional &operator=(T &&value) { reset(); has_value_ = true; new (&storage_) T(std::move(value)); return *this; }
    optional &operator=(std::nullptr_t) { reset(); return *this; }
    optional &operator=(const optional &other) { if (this != &other) { reset(); if (other.has_value_) { has_value_ = true; new (&storage_) T(*other); } } return *this; }
    optional &operator=(optional &&other) { if (this != &other) { reset(); if (other.has_value_) { has_value_ = true; new (&storage_) T(std::move(*other)); other.reset(); } } return *this; }
    explicit operator bool() const { return has_value_; }
    bool is_set() const { return has_value_; }
    T &operator*() { return *reinterpret_cast<T *>(&storage_); }
    const T &operator*() const { return *reinterpret_cast<const T *>(&storage_); }
    T *operator->() { return reinterpret_cast<T *>(&storage_); }
    const T *operator->() const { return reinterpret_cast<const T *>(&storage_); }
    T &get() { return *reinterpret_cast<T *>(&storage_); }
    const T &get() const { return *reinterpret_cast<const T *>(&storage_); }
    void reset() { if (has_value_) { reinterpret_cast<T *>(&storage_)->~T(); has_value_ = false; } }
    bool operator==(const optional &other) const { if (has_value_ != other.has_value_) return false; if (!has_value_) return true; return get() == other.get(); }
    bool operator!=(const optional &other) const { return !(*this == other); }
private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
    bool has_value_;
};
} // namespace xlntx

// ============================================================================
// Font (needed by rich_text_run)
// ============================================================================
namespace xlntx {
class XLNTX_API font {
public:
    font() = default;
    bool operator==(const font &) const { return true; }
    bool operator!=(const font &) const { return false; }
};
} // namespace xlntx

// ============================================================================
// rich_text_run, phonetic_run, phonetic_pr, rich_text
// ============================================================================
namespace xlntx {
struct XLNTX_API rich_text_run {
    std::string first;
    optional<font> second;
    bool preserve_space = false;
    bool operator==(const rich_text_run &) const;
    bool operator!=(const rich_text_run &) const;
};
struct XLNTX_API phonetic_run {
    std::string text;
    bool operator==(const phonetic_run &) const { return true; }
    bool operator!=(const phonetic_run &) const { return false; }
};
struct XLNTX_API phonetic_pr {
    std::uint32_t font_id = 0;
    bool operator==(const phonetic_pr &) const { return true; }
};
class XLNTX_API rich_text {
public:
    rich_text() = default;
    explicit rich_text(const std::string &plain_text);
    rich_text(const std::string &text, const class font &);
    explicit rich_text(const rich_text_run &);
    void clear();
    void plain_text(const std::string &s, bool preserve_space);
    std::string plain_text() const;
    const std::vector<rich_text_run> &runs() const { return runs_; }
    void runs(const std::vector<rich_text_run> &r) { runs_ = r; }
    void add_run(const rich_text_run &run) { runs_.push_back(run); }
    const std::vector<phonetic_run> &phonetic_runs() const { return phonetic_runs_; }
    void phonetic_runs(const std::vector<phonetic_run> &r) { phonetic_runs_ = r; }
    void add_phonetic_run(const phonetic_run &run) { phonetic_runs_.push_back(run); }
    bool has_phonetic_properties() const { return phonetic_properties_.is_set(); }
    const phonetic_pr &phonetic_properties() const { return phonetic_properties_.get(); }
    void phonetic_properties(const phonetic_pr &pp) { phonetic_properties_ = pp; }
    rich_text &operator=(const rich_text &) = default;
    bool operator==(const rich_text &other) const;
    bool operator!=(const rich_text &other) const;
    bool operator==(const std::string &other) const;
    bool operator!=(const std::string &other) const;
private:
    std::vector<rich_text_run> runs_;
    std::vector<phonetic_run> phonetic_runs_;
    optional<phonetic_pr> phonetic_properties_;
};
struct rich_text_hash { std::size_t operator()(const rich_text &k) const; };
} // namespace xlntx

// ============================================================================
// variant
// ============================================================================
namespace xlntx {
class XLNTX_API variant {
public:
    enum class type { vector, null, i4, lpstr, date, boolean };
    variant() = default;
    variant(const std::string &) {}
    variant(const char *) {}
    variant(std::int32_t) {}
    variant(bool) {}
    variant(const datetime &) {}
    variant(const std::initializer_list<std::int32_t> &) {}
    variant(const std::vector<std::int32_t> &) {}
    variant(const std::initializer_list<std::string> &) {}
    variant(const std::vector<std::string> &) {}
    variant(const std::vector<variant> &) {}
    bool is(type t) const { return type_ == t; }
    template<typename T> T get() const;
    type value_type() const { return type_; }
    bool operator==(const variant &) const { return true; }
private:
    type type_ = type::null;
    std::vector<variant> vector_value_;
    std::int32_t i4_value_ = 0;
    std::string lpstr_value_;
};
} // namespace xlntx

// ============================================================================
// Style stubs (color, fill, border, alignment, number_format, protection, format, style, conditional_format)
// ============================================================================
namespace xlntx {
class XLNTX_API color { public: color() = default; bool operator==(const color &) const { return true; } };
class XLNTX_API fill { public: fill() = default; };
class pattern_fill {};
class XLNTX_API border { public: border() = default; bool operator==(const border &) const { return true; } };
class XLNTX_API alignment { public: alignment() = default; bool operator==(const alignment &) const { return true; } };
class XLNTX_API number_format { public: number_format() = default; bool operator==(const number_format &) const { return true; } };
class XLNTX_API protection { public: protection() = default; bool operator==(const protection &) const { return true; } };
class XLNTX_API format { public: format() = default; bool operator==(const format &) const { return true; } };
class XLNTX_API style { public: style() = default; bool operator==(const style &) const { return true; } };
class condition {};
class XLNTX_API conditional_format { public: conditional_format() = default; };
} // namespace xlntx

// ============================================================================
// path
// ============================================================================
namespace xlntx {
class XLNTX_API path {
public:
    static char system_separator();
    path() = default;
    path(const std::string &path_string);
    path(const std::string &path_string, char sep);
    bool is_relative() const;
    bool is_absolute() const;
    bool is_root() const;
    path parent() const;
    std::string filename() const;
    std::string extension() const;
    std::pair<std::string, std::string> split_extension() const;
    std::vector<std::string> split() const;
    const std::string &string() const;
    bool exists() const;
    bool is_directory() const;
    bool is_file() const;
    std::string read_contents() const;
    path append(const std::string &part) const;
    path append(const path &p) const;
    path resolve(const path &base) const;
    path relative_to(const path &base) const;
    bool operator==(const path &other) const;
    bool operator!=(const path &other) const;
private:
    std::string internal_;
};
} // namespace xlntx

// ============================================================================
// comment, hyperlink (empty stubs)
// ============================================================================
namespace xlntx {
class comment {};
class hyperlink {};
} // namespace xlntx

// ============================================================================
// packaging (uri, relationship, manifest, ext_list)
// ============================================================================
namespace xlntx {
class XLNTX_API uri {
public:
    uri() = default;
    explicit uri(const std::string &uri_string);
    std::string to_string() const;
private:
    std::string uri_string_;
};
enum class relationship_type {
    office_document, workbook, worksheet, shared_strings, styles,
    theme, image, chart, comments, vml_drawing,
    core_properties, extended_properties, custom_properties,
    custom_xml, thumbnail, calculation_chain, connections,
    external_link, printer_settings, unknown
};
class XLNTX_API relationship {
public:
    relationship() = default;
    relationship(const std::string &id, relationship_type type, const uri &source, const uri &target);
private:
    std::string id_;
    relationship_type type_ = relationship_type::unknown;
    uri source_;
    uri target_;
};
class manifest {};
class ext_list {};
} // namespace xlntx

// ============================================================================
// drawing
// ============================================================================
namespace xlntx {
class spreadsheet_drawing {};
} // namespace xlntx

// ============================================================================
// Worksheet component stubs
// ============================================================================
namespace xlntx {
class column_properties {};
class row_properties {};
enum class sheet_state { visible, hidden, very_hidden };
class sheet_view {};
struct page_setup {};
class page_margins {};
class header {};
class footer {};
class header_footer {};
class pane {};
class selection {};
class sheet_pr {};
class sheet_protection {};
class print_options {};
class sheet_format_properties {};
enum class major_order { row, column };
} // namespace xlntx

// ============================================================================
// metadata_property enums
// ============================================================================
namespace xlntx {
enum class core_property {};
enum class extended_property {};
class metadata_property {};
} // namespace xlntx

// ============================================================================
// cell_reference (needs range_reference forward-declared)
// ============================================================================
namespace xlntx {

struct cell_reference_hash { std::size_t operator()(const cell_reference &k) const; };

class XLNTX_API cell_reference {
public:
    cell_reference();
    cell_reference(const char *reference_string);
    cell_reference(const std::string &reference_string);
    cell_reference(column_t column, row_t row);
    static std::pair<std::string, row_t> split_reference(const std::string &reference_string);
    static std::pair<std::string, row_t> split_reference(const std::string &reference_string, bool &absolute_column, bool &absolute_row);
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
template <> struct hash<xlntx::cell_reference> {
    std::size_t operator()(const xlntx::cell_reference &k) const {
        return static_cast<std::size_t>((static_cast<std::uint64_t>(k.row()) << 32) | k.column_index());
    }
};
} // namespace std

namespace xlntx {

// ============================================================================
// range_reference
// ============================================================================
class XLNTX_API range_reference {
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

// ============================================================================
// cell_vector
// ============================================================================
class XLNTX_API cell_vector {
public:
    cell_vector(worksheet &ws, row_t row);
    cell_vector(const worksheet &ws, row_t row);
    class cell operator[](std::size_t cell_index);
    const class cell operator[](std::size_t cell_index) const;
    class cell front();
    const class cell front() const;
    class cell back();
    const class cell back() const;
    std::size_t length() const;
    row_t row() const { return row_; }
    bool operator==(const cell_vector &other) const;
    bool operator!=(const cell_vector &other) const;
private:
    worksheet *ws_;
    const worksheet *const_ws_;
    row_t row_;
};

// ============================================================================
// cell_iterator (empty stub)
// ============================================================================
class cell_iterator {};

// ============================================================================
// worksheet (forward declarations resolved above)
// ============================================================================
namespace detail { struct worksheet_impl; class xlsx_reader; }

class XLNTX_API worksheet {
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

// ============================================================================
// range + range_iterator + const_range_iterator
// ============================================================================
class XLNTX_API range {
public:
    using iterator = range_iterator;
    using const_iterator = const_range_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    range(worksheet ws, const range_reference &reference, major_order order = major_order::row, bool skip_null = false);

    void clear_cells();
    cell_vector vector(std::size_t n);
    const cell_vector vector(std::size_t n) const;
    class cell cell(const cell_reference &ref);
    const class cell cell(const cell_reference &ref) const;
    const worksheet &target_worksheet() const;
    range_reference reference() const;
    std::size_t length() const;
    bool contains(const cell_reference &ref);

    range alignment(const xlntx::alignment &a) { return *this; }
    range border(const xlntx::border &b) { return *this; }
    range fill(const xlntx::fill &f) { return *this; }
    range font(const xlntx::font &f) { return *this; }
    range number_format(const xlntx::number_format &nf) { return *this; }
    range protection(const xlntx::protection &p) { return *this; }
    range style(const class style &s) { return *this; }
    range style(const std::string &) { return *this; }
    xlntx::conditional_format conditional_format(const condition &) { return xlntx::conditional_format(); }

    cell_vector front();
    const cell_vector front() const;
    cell_vector back();
    const cell_vector back() const;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    cell_vector operator[](std::size_t n);
    const cell_vector operator[](std::size_t n) const;
    bool operator==(const range &other) const;
    bool operator!=(const range &other) const;
    void apply(std::function<void(class cell)> f);

private:
    worksheet ws_;
    range_reference ref_;
    major_order order_;
    bool skip_null_;
};

class XLNTX_API range_iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = cell_vector;
    using difference_type = std::ptrdiff_t;
    using pointer = cell_vector *;
    using reference = cell_vector;

    range_iterator();
    range_iterator(worksheet &ws, row_t row, bool skip_null);
    cell_vector operator*();
    range_iterator &operator++();
    range_iterator operator++(int);
    bool operator==(const range_iterator &other) const;
    bool operator!=(const range_iterator &other) const;
private:
    worksheet *ws_ = nullptr;
    row_t row_ = 0;
    bool skip_null_ = true;
    row_t max_row_ = 0;
    void advance_to_valid();
};

class XLNTX_API const_range_iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const cell_vector;
    using difference_type = std::ptrdiff_t;
    const_range_iterator();
    const_range_iterator(const worksheet &ws, row_t row, bool skip_null);
    const cell_vector operator*();
    const_range_iterator &operator++();
    const_range_iterator operator++(int);
    bool operator==(const const_range_iterator &other) const;
    bool operator!=(const const_range_iterator &other) const;
private:
    const worksheet *ws_ = nullptr;
    row_t row_ = 0;
    bool skip_null_ = true;
    row_t max_row_ = 0;
    void advance_to_valid();
};

// ============================================================================
// cell
// ============================================================================
namespace detail { struct cell_impl; }

class XLNTX_API cell {
public:
    using type = cell_type;
    static const std::unordered_map<std::string, int> &error_codes();

    cell(const cell &) = default;
    bool has_value() const;
    template <typename T> T value() const;
    void clear_value();
    void value(std::nullptr_t);
    void value(bool boolean_value);
    void value(int int_value);
    void value(unsigned int int_value);
    void value(long long int int_value);
    void value(unsigned long long int int_value);
    void value(float float_value);
    void value(double float_value);
    void value(const date &date_value);
    void value(const time &time_value);
    void value(const datetime &datetime_value);
    void value(const timedelta &timedelta_value);
    void value(const std::string &string_value);
    void value(const char *string_value);
    void value(const rich_text &text_value);
    void value(const cell other_cell);
    void value(const std::string &string_value, bool infer_type);

    type data_type() const;
    void data_type(type t);
    bool garbage_collectible() const;
    bool is_date() const;

    cell_reference reference() const;
    column_t column() const;
    column_t::index_t column_index() const;
    row_t row() const;
    std::pair<int, int> anchor() const;

    class hyperlink hyperlink() const;
    void hyperlink(const std::string &url, const std::string &display = "");
    void hyperlink(xlntx::cell target, const std::string &display = "");
    void hyperlink(class range target, const std::string &display = "");
    bool has_hyperlink() const;

    class alignment computed_alignment() const;
    class border computed_border() const;
    class fill computed_fill() const;
    class font computed_font() const;
    class number_format computed_number_format() const;
    class protection computed_protection() const;

    bool has_format() const;
    const class format format() const;
    void format(const class format new_format);
    void clear_format();
    class number_format number_format() const;
    void number_format(const class number_format &format);
    class font font() const;
    void font(const class font &font_);
    class fill fill() const;
    void fill(const class fill &fill_);
    class border border() const;
    void border(const class border &border_);
    class alignment alignment() const;
    void alignment(const class alignment &alignment_);
    class protection protection() const;
    void protection(const class protection &protection_);

    bool has_style() const;
    class style style();
    const class style style() const;
    void style(const class style &new_style);
    void style(const std::string &style_name);
    void clear_style();

    std::string formula() const;
    void formula(const std::string &formula);
    void clear_formula();
    bool has_formula() const;

    std::string to_string() const;

    bool is_merged() const;
    void merged(bool merged);

    bool phonetics_visible() const;
    void show_phonetics(bool phonetics);

    std::string error() const;
    void error(const std::string &error);

    cell offset(int column, int row);

    class worksheet worksheet();
    const class worksheet worksheet() const;
    class workbook &workbook();
    const class workbook &workbook() const;

    calendar base_date() const;
    std::string check_string(const std::string &to_check);

    bool has_comment();
    void clear_comment();
    class comment comment();
    void comment(const std::string &text, const std::string &author = "Microsoft Office User");
    void comment(const std::string &comment_text, const class font &comment_font, const std::string &author = "Microsoft Office User");
    void comment(const class comment &new_comment);

    double width() const;
    double height() const;

    cell &operator=(const cell &rhs);
    bool operator==(const cell &comparand) const;
    bool operator!=(const cell &comparand) const;

private:
    friend class style;
    friend class worksheet;
    friend class detail::xlsx_reader;
    friend XLNTX_API bool operator==(std::nullptr_t, const cell &);
    friend XLNTX_API bool operator==(const cell &, std::nullptr_t);
    friend XLNTX_API std::ostream &operator<<(std::ostream &, const cell &);

    class format modifiable_format();
    cell() = delete;
    cell(detail::cell_impl *d);
    detail::cell_impl *d_ = nullptr;
};

XLNTX_API bool operator==(std::nullptr_t, const cell &cell);
XLNTX_API bool operator==(const cell &cell, std::nullptr_t);
XLNTX_API std::ostream &operator<<(std::ostream &stream, const cell &cell);

// cell::value<T> template specializations
template <> bool cell::value<bool>() const;
template <> int cell::value<int>() const;
template <> unsigned int cell::value<unsigned int>() const;
template <> long long int cell::value<long long int>() const;
template <> unsigned long long int cell::value<unsigned long long int>() const;
template <> float cell::value<float>() const;
template <> double cell::value<double>() const;
template <> date cell::value<date>() const;
template <> time cell::value<time>() const;
template <> datetime cell::value<datetime>() const;
template <> timedelta cell::value<timedelta>() const;
template <> std::string cell::value<std::string>() const;
template <> rich_text cell::value<rich_text>() const;

} // namespace xlntx

// ============================================================================
// workbook sub-types
// ============================================================================
#include <iosfwd>

namespace xlntx {
class workbook_view {};
class worksheet_iterator {};
class const_worksheet_iterator {};
class calculation_properties {};
class document_security {};
class external_book {};
class named_range {};
class streaming_workbook_reader {};
class streaming_workbook_writer {};
class theme {};
} // namespace xlntx

// ============================================================================
// workbook
// ============================================================================
namespace xlntx {

namespace detail { struct workbook_impl; }

class XLNTX_API workbook {
public:
    using iterator = worksheet_iterator;
    using const_iterator = const_worksheet_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static workbook empty();

    workbook();
    workbook(const xlntx::path &file);
    workbook(const xlntx::path &file, const std::string &password);
    workbook(std::istream &data);
    workbook(std::istream &data, const std::string &password);
    workbook(workbook &&other);
    workbook(const workbook &other);
    ~workbook();

    worksheet create_sheet();
    worksheet create_sheet(std::size_t index);
    worksheet create_sheet_with_rel(const std::string &title, const relationship &rel);
    worksheet copy_sheet(worksheet worksheet);
    worksheet copy_sheet(worksheet worksheet, std::size_t index);

    worksheet active_sheet();
    worksheet sheet_by_title(const std::string &title);
    const worksheet sheet_by_title(const std::string &title) const;
    worksheet sheet_by_index(std::size_t index);
    const worksheet sheet_by_index(std::size_t index) const;
    worksheet sheet_by_id(std::size_t id);
    const worksheet sheet_by_id(std::size_t id) const;

    bool contains(const std::string &title) const;
    std::size_t index(worksheet worksheet);
    void remove_sheet(worksheet worksheet);
    void clear();

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    void apply_to_cells(std::function<void(cell)> f);

    std::vector<std::string> sheet_titles() const;
    std::size_t sheet_count() const;

    bool has_core_property(xlntx::core_property type) const;
    std::vector<xlntx::core_property> core_properties() const;
    variant core_property(xlntx::core_property type) const;
    void core_property(xlntx::core_property type, const variant &value);

    bool has_extended_property(xlntx::extended_property type) const;
    std::vector<xlntx::extended_property> extended_properties() const;
    variant extended_property(xlntx::extended_property type) const;
    void extended_property(xlntx::extended_property type, const variant &value);

    bool has_custom_property(const std::string &property_name) const;
    std::vector<std::string> custom_properties() const;
    variant custom_property(const std::string &property_name) const;
    void custom_property(const std::string &property_name, const variant &value);

    calendar base_date() const;
    void base_date(calendar base_date);

    bool has_title() const;
    std::string title() const;
    void title(const std::string &title);

    void abs_path(const std::string &path);
    void arch_id_flags(const std::size_t flags);

    std::vector<xlntx::named_range> named_ranges() const;
    void create_named_range(const std::string &name, worksheet worksheet, const range_reference &reference);
    void create_named_range(const std::string &name, worksheet worksheet, const std::string &reference_string);
    bool has_named_range(const std::string &name) const;
    class range named_range(const std::string &name);
    void remove_named_range(const std::string &name);

    void save(std::vector<std::uint8_t> &data) const;
    void save(std::vector<std::uint8_t> &data, const std::string &password) const;
    void save(const std::string &filename) const;
    void save(const std::string &filename, const std::string &password) const;
#ifdef _MSC_VER
    void save(const std::wstring &filename) const;
    void save(const std::wstring &filename, const std::string &password) const;
#endif
    void save(const xlntx::path &filename) const;
    void save(const xlntx::path &filename, const std::string &password) const;
    void save(std::ostream &stream) const;
    void save(std::ostream &stream, const std::string &password) const;

    void load(const std::vector<std::uint8_t> &data);
    void load(const std::vector<std::uint8_t> &data, const std::string &password);
    void load(const std::string &filename);
    void load(const std::string &filename, const std::string &password);
#ifdef _MSC_VER
    void load(const std::wstring &filename);
    void load(const std::wstring &filename, const std::string &password);
#endif
    void load(const xlntx::path &filename);
    void load(const xlntx::path &filename, const std::string &password);
    void load(std::istream &stream);
    void load(std::istream &stream, const std::string &password);

    bool has_view() const;
    workbook_view view() const;
    void view(const workbook_view &view);

    bool has_code_name() const;
    std::string code_name() const;
    void code_name(const std::string &code_name);

    bool has_file_version() const;
    std::string app_name() const;
    std::size_t last_edited() const;
    std::size_t lowest_edited() const;
    std::size_t rup_build() const;

    bool has_theme() const;
    const xlntx::theme &theme() const;
    void theme(const class theme &value);

    xlntx::format format(std::size_t format_index);
    const xlntx::format format(std::size_t format_index) const;
    xlntx::format create_format(bool default_format = false);
    void clear_formats();

    bool has_style(const std::string &name) const;
    class style style(const std::string &name);
    const class style style(const std::string &name) const;
    class style create_style(const std::string &name);
    class style create_builtin_style(std::size_t builtin_id);
    void clear_styles();
    void default_slicer_style(const std::string &value);
    std::string default_slicer_style() const;
    void enable_known_fonts();
    void disable_known_fonts();
    bool known_fonts_enabled() const;

    class manifest &manifest();
    const class manifest &manifest() const;

    std::size_t add_shared_string(const rich_text &shared, bool allow_duplicates = false);
    const std::map<std::size_t, rich_text> &shared_strings_by_id() const;
    const rich_text &shared_strings(std::size_t index) const;
    std::unordered_map<rich_text, std::size_t, rich_text_hash> &shared_strings();
    const std::unordered_map<rich_text, std::size_t, rich_text_hash> &shared_strings() const;

    void thumbnail(const std::vector<std::uint8_t> &thumbnail, const std::string &extension, const std::string &content_type);
    const std::vector<std::uint8_t> &thumbnail() const;

    bool has_calculation_properties() const;
    class calculation_properties calculation_properties() const;
    void calculation_properties(const class calculation_properties &props);

    workbook &operator=(workbook other);
    worksheet operator[](const std::string &name);
    worksheet operator[](std::size_t index);
    bool operator==(const workbook &rhs) const;
    bool operator!=(const workbook &rhs) const;

private:
    friend class streaming_workbook_reader;
    friend class worksheet;
    friend class detail::xlsx_reader;

    workbook(detail::workbook_impl *impl);
    detail::workbook_impl &impl();
    const detail::workbook_impl &impl() const;
    void register_package_part(relationship_type type);
    void register_workbook_part(relationship_type type);
    void register_worksheet_part(worksheet ws, relationship_type type);
    void garbage_collect_formulae();
    void update_sheet_properties();
    void swap(workbook &other);
    void reorder_relationships();

    std::unique_ptr<detail::workbook_impl> d_;
};

} // namespace xlntx

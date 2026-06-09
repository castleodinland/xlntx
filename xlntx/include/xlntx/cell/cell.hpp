#pragma once

#include <memory>
#include <string>

#include <xlntx/xlntx_config.hpp>
#include <xlntx/cell/cell_type.hpp>
#include <xlntx/cell/index_types.hpp>
#include <xlntx/cell/rich_text.hpp>

namespace xlntx {

class alignment;
class border;
class fill;
class font;
class format;
class number_format;
class protection;
class style;
class workbook;
class worksheet;
class cell_reference;
class hyperlink;
class comment;
struct date;
struct datetime;
struct time;
struct timedelta;
enum class calendar;

namespace detail {
struct cell_impl;
class xlsx_reader;
}

class XLNTX_API cell
{
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
    void comment(const std::string &comment_text, const class font &comment_font,
                 const std::string &author = "Microsoft Office User");
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

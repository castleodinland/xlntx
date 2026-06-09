#include <xlntx/cell/cell.hpp>
#include <xlntx/cell/cell_reference.hpp>
#include <xlntx/cell/comment.hpp>
#include <xlntx/cell/hyperlink.hpp>
#include <xlntx/worksheet/range.hpp>
#include <xlntx/styles/alignment.hpp>
#include <xlntx/styles/border.hpp>
#include <xlntx/styles/fill.hpp>
#include <xlntx/styles/font.hpp>
#include <xlntx/styles/format.hpp>
#include <xlntx/styles/number_format.hpp>
#include <xlntx/styles/protection.hpp>
#include <xlntx/styles/style.hpp>
#include <xlntx/utils/date.hpp>
#include <xlntx/utils/time.hpp>
#include <xlntx/utils/datetime.hpp>
#include <xlntx/utils/timedelta.hpp>
#include <xlntx/utils/exceptions.hpp>
#include <xlntx/utils/calendar.hpp>
#include "detail/cell_impl.hpp"
#include "detail/workbook_impl.hpp"
#include "detail/worksheet_impl.hpp"

#include <sstream>
#include <cmath>

namespace xlntx {

const std::unordered_map<std::string, int> &cell::error_codes()
{
    static const std::unordered_map<std::string, int> codes = {
        {"#NULL!", 0}, {"#DIV/0!", 1}, {"#VALUE!", 2}, {"#REF!", 3},
        {"#NAME?", 4}, {"#NUM!", 5}, {"#N/A", 6}
    };
    return codes;
}

cell::cell(detail::cell_impl *d) : d_(d) {}

bool cell::has_value() const
{
    return d_ && d_->type_ != cell_type::empty;
}

void cell::clear_value()
{
    if (d_)
    {
        d_->type_ = cell_type::empty;
        d_->value_numeric_ = 0;
        d_->value_text_ = rich_text();
    }
}

void cell::value(std::nullptr_t) { clear_value(); }
void cell::value(bool bv) { if (d_) { d_->type_ = cell_type::boolean; d_->value_numeric_ = bv ? 1.0 : 0.0; } }
void cell::value(int v) { if (d_) { d_->type_ = cell_type::number; d_->value_numeric_ = static_cast<double>(v); } }
void cell::value(unsigned int v) { if (d_) { d_->type_ = cell_type::number; d_->value_numeric_ = static_cast<double>(v); } }
void cell::value(long long int v) { if (d_) { d_->type_ = cell_type::number; d_->value_numeric_ = static_cast<double>(v); } }
void cell::value(unsigned long long int v) { if (d_) { d_->type_ = cell_type::number; d_->value_numeric_ = static_cast<double>(v); } }
void cell::value(float v) { if (d_) { d_->type_ = cell_type::number; d_->value_numeric_ = static_cast<double>(v); } }
void cell::value(double v) { if (d_) { d_->type_ = cell_type::number; d_->value_numeric_ = v; } }
void cell::value(const date &) { if (d_) { d_->type_ = cell_type::date; } }
void cell::value(const time &) { if (d_) { d_->type_ = cell_type::number; } }
void cell::value(const datetime &) { if (d_) { d_->type_ = cell_type::number; } }
void cell::value(const timedelta &) { if (d_) { d_->type_ = cell_type::number; } }
void cell::value(const std::string &s) { if (d_) { d_->type_ = cell_type::inline_string; d_->value_text_ = rich_text(s); } }
void cell::value(const char *s) { value(std::string(s)); }
void cell::value(const rich_text &rt) { if (d_) { d_->type_ = cell_type::inline_string; d_->value_text_ = rt; } }
void cell::value(const cell other) { if (d_) { *d_ = *other.d_; } }
void cell::value(const std::string &s, bool) { value(s); }

cell::type cell::data_type() const { return d_ ? d_->type_ : cell_type::empty; }
void cell::data_type(type t) { if (d_) d_->type_ = t; }

bool cell::garbage_collectible() const { return d_ && d_->type_ == cell_type::empty && !is_merged(); }
bool cell::is_date() const { return false; }

cell_reference cell::reference() const
{
    if (d_) return cell_reference(column_t(d_->column_), d_->row_);
    return cell_reference();
}
column_t cell::column() const { return d_ ? column_t(d_->column_) : column_t(1); }
column_t::index_t cell::column_index() const { return d_ ? d_->column_ : 1; }
row_t cell::row() const { return d_ ? d_->row_ : 1; }
std::pair<int, int> cell::anchor() const { return {static_cast<int>(column_index()), static_cast<int>(row())}; }

std::string cell::to_string() const
{
    if (!d_) return "";

    switch (d_->type_)
    {
    case cell_type::empty:
        return "";
    case cell_type::boolean:
        return d_->value_numeric_ != 0.0 ? "TRUE" : "FALSE";
    case cell_type::number:
    {
        // Avoid trailing zeros for integers
        double v = d_->value_numeric_;
        if (v == std::floor(v) && std::abs(v) < 1e15)
        {
            std::ostringstream oss;
            oss << static_cast<long long>(v);
            return oss.str();
        }
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
    case cell_type::shared_string:
    case cell_type::inline_string:
    case cell_type::formula_string:
        return d_->value_text_.plain_text();
    case cell_type::error:
        return d_->value_text_.plain_text();
    case cell_type::date:
        return d_->value_text_.plain_text();
    }
    return "";
}

bool cell::is_merged() const { return d_ && d_->is_merged_; }
void cell::merged(bool m) { if (d_) d_->is_merged_ = m; }

bool cell::phonetics_visible() const { return d_ && d_->phonetics_visible_; }
void cell::show_phonetics(bool p) { if (d_) d_->phonetics_visible_ = p; }

std::string cell::error() const { return d_ ? d_->value_text_.plain_text() : ""; }
void cell::error(const std::string &e) { if (d_) { d_->type_ = cell_type::error; d_->value_text_ = rich_text(e); } }

bool cell::has_format() const { return d_ && d_->format_index_.is_set(); }
const class format cell::format() const
{
    static class format f;
    return f;
}
void cell::format(const class format) {}
void cell::clear_format() { if (d_) d_->format_index_.reset(); }

bool cell::has_style() const { return false; }
class style cell::style() { return class style(); }
const class style cell::style() const { return class style(); }
void cell::style(const class style &) {}
void cell::style(const std::string &) {}
void cell::clear_style() {}

std::string cell::formula() const { return d_ && d_->formula_.is_set() ? d_->formula_.get() : ""; }
void cell::formula(const std::string &f) { if (d_) d_->formula_ = f; }
void cell::clear_formula() { if (d_) d_->formula_.reset(); }
bool cell::has_formula() const { return d_ && d_->formula_.is_set(); }

cell &cell::operator=(const cell &rhs) { if (this != &rhs) d_ = rhs.d_; return *this; }
bool cell::operator==(const cell &comparand) const { return d_ == comparand.d_; }
bool cell::operator!=(const cell &comparand) const { return d_ != comparand.d_; }

bool operator==(std::nullptr_t, const cell &c) { return c == cell(nullptr); }
bool operator==(const cell &c, std::nullptr_t) { return cell(nullptr) == c; }
std::ostream &operator<<(std::ostream &stream, const cell &c) { return stream << c.to_string(); }

// Template specializations
template <> bool cell::value<bool>() const { return d_ && d_->value_numeric_ != 0.0; }
template <> int cell::value<int>() const { return d_ ? static_cast<int>(d_->value_numeric_) : 0; }
template <> unsigned int cell::value<unsigned int>() const { return d_ ? static_cast<unsigned int>(d_->value_numeric_) : 0; }
template <> long long int cell::value<long long int>() const { return d_ ? static_cast<long long int>(d_->value_numeric_) : 0; }
template <> unsigned long long int cell::value<unsigned long long int>() const { return d_ ? static_cast<unsigned long long int>(d_->value_numeric_) : 0; }
template <> float cell::value<float>() const { return d_ ? static_cast<float>(d_->value_numeric_) : 0.0f; }
template <> double cell::value<double>() const { return d_ ? d_->value_numeric_ : 0.0; }
template <> date cell::value<date>() const { return date(); }
template <> time cell::value<time>() const { return time(); }
template <> datetime cell::value<datetime>() const { return datetime(); }
template <> timedelta cell::value<timedelta>() const { return timedelta(); }
template <> std::string cell::value<std::string>() const { return to_string(); }
template <> rich_text cell::value<rich_text>() const { return d_ ? d_->value_text_ : rich_text(); }

// Format/style accessors (stubs)
class alignment cell::alignment() const { static class alignment a; return a; }
void cell::alignment(const class alignment &) {}
class font cell::font() const { static class font f; return f; }
void cell::font(const class font &) {}
class fill cell::fill() const { static class fill f; return f; }
void cell::fill(const class fill &) {}
class border cell::border() const { static class border b; return b; }
void cell::border(const class border &) {}
class number_format cell::number_format() const { static class number_format nf; return nf; }
void cell::number_format(const class number_format &) {}
class protection cell::protection() const { static class protection p; return p; }
void cell::protection(const class protection &) {}

class alignment cell::computed_alignment() const { static class alignment a; return a; }
class border cell::computed_border() const { static class border b; return b; }
class fill cell::computed_fill() const { static class fill f; return f; }
class font cell::computed_font() const { static class font f; return f; }
class number_format cell::computed_number_format() const { static class number_format nf; return nf; }
class protection cell::computed_protection() const { static class protection p; return p; }

class hyperlink cell::hyperlink() const { return class hyperlink(); }
void cell::hyperlink(const std::string &, const std::string &) {}
void cell::hyperlink(xlntx::cell, const std::string &) {}
void cell::hyperlink(class range, const std::string &) {}
bool cell::has_hyperlink() const { return false; }

calendar cell::base_date() const { return calendar::windows_1900; }
std::string cell::check_string(const std::string &s) { return s; }

bool cell::has_comment() { return false; }
void cell::clear_comment() {}
class comment cell::comment() { return class comment(); }
void cell::comment(const std::string &, const std::string &) {}
void cell::comment(const std::string &, const class font &, const std::string &) {}
void cell::comment(const class comment &) {}

double cell::width() const { return 8.0; }
double cell::height() const { return 15.0; }

cell cell::offset(int col, int row)
{
    // stub — read-only library, no cell creation
    return cell(nullptr);
}

} // namespace xlntx

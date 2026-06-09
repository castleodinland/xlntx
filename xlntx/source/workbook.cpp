#include <xlntx/workbook/workbook.hpp>
#include <xlntx/worksheet/worksheet.hpp>
#include <xlntx/worksheet/range.hpp>
#include <xlntx/workbook/calculation_properties.hpp>
#include <xlntx/workbook/workbook_view.hpp>
#include <xlntx/workbook/theme.hpp>
#include <xlntx/workbook/named_range.hpp>
#include <xlntx/packaging/manifest.hpp>
#include <xlntx/packaging/relationship.hpp>
#include <xlntx/utils/exceptions.hpp>
#include <xlntx/utils/path.hpp>
#include <xlntx/utils/variant.hpp>
#include <xlntx/styles/format.hpp>
#include <xlntx/styles/style.hpp>
#include <xlntx/drawing/spreadsheet_drawing.hpp>

#include "detail/workbook_impl.hpp"
#include "detail/worksheet_impl.hpp"
#include "detail/cell_impl.hpp"
#include "detail/xlsx_reader.hpp"
#include "detail/zip_reader.hpp"

#include <fstream>
#include <filesystem>
#include <sstream>

namespace xlntx {

// ===== workbook static =====

workbook workbook::empty()
{
    workbook wb;
    return wb;
}

// ===== Constructors =====

workbook::workbook() : d_(new detail::workbook_impl()) {}

workbook::workbook(const xlntx::path &file)
    : d_(new detail::workbook_impl())
{
    load(file);
}

workbook::workbook(const xlntx::path &file, const std::string &password)
    : d_(new detail::workbook_impl())
{
    load(file, password);
}

workbook::workbook(std::istream &data)
    : d_(new detail::workbook_impl())
{
    load(data);
}

workbook::workbook(std::istream &data, const std::string &password)
    : d_(new detail::workbook_impl())
{
    load(data, password);
}

workbook::workbook(workbook &&other) : d_(std::move(other.d_)) {}
workbook::workbook(const workbook &other) : d_(new detail::workbook_impl(*other.d_)) {}
workbook::~workbook() = default;

workbook::workbook(detail::workbook_impl *impl) : d_(impl) {}

// ===== Sheet access =====

worksheet workbook::sheet_by_index(std::size_t index)
{
    auto it = d_->worksheets_.begin();
    std::advance(it, index);
    return worksheet(&(*it));
}

const worksheet workbook::sheet_by_index(std::size_t index) const
{
    auto it = d_->worksheets_.begin();
    std::advance(it, index);
    return worksheet(const_cast<detail::worksheet_impl *>(&(*it)));
}

worksheet workbook::sheet_by_title(const std::string &title)
{
    for (auto &ws : d_->worksheets_)
    {
        if (ws.title_ == title)
            return worksheet(&ws);
    }
    throw invalid_sheet_title(title);
}

const worksheet workbook::sheet_by_title(const std::string &title) const
{
    for (auto &ws : d_->worksheets_)
    {
        if (ws.title_ == title)
            return worksheet(const_cast<detail::worksheet_impl *>(&ws));
    }
    throw invalid_sheet_title(title);
}

worksheet workbook::sheet_by_id(std::size_t id)
{
    for (auto &ws : d_->worksheets_)
    {
        if (ws.id_ == id)
            return worksheet(&ws);
    }
    throw key_not_found();
}

const worksheet workbook::sheet_by_id(std::size_t id) const
{
    for (auto &ws : d_->worksheets_)
    {
        if (ws.id_ == id)
            return worksheet(const_cast<detail::worksheet_impl *>(&ws));
    }
    throw key_not_found();
}

bool workbook::contains(const std::string &title) const
{
    for (auto &ws : d_->worksheets_)
        if (ws.title_ == title) return true;
    return false;
}

std::size_t workbook::index(worksheet ws) { return 0; } // stub
worksheet workbook::active_sheet() { return sheet_by_index(0); }

// ===== Load =====

void workbook::load(const xlntx::path &filename)
{
    load(filename.string());
}

void workbook::load(const xlntx::path &filename, const std::string &password)
{
    if (!password.empty())
        throw unsupported("encrypted xlsx not supported in xlntx");
    load(filename);
}

void workbook::load(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
        throw invalid_file(filename);
    d_->abs_path_ = filename;
    load(file);
}

void workbook::load(const std::string &filename, const std::string &password)
{
    if (!password.empty())
        throw unsupported("encrypted xlsx not supported in xlntx");
    load(filename);
}

void workbook::load(std::istream &stream)
{
    // Clear any existing data
    d_.reset(new detail::workbook_impl());
    detail::xlsx_reader reader(*d_);
    reader.read(stream);
}

void workbook::load(std::istream &stream, const std::string &password)
{
    if (!password.empty())
        throw unsupported("encrypted xlsx not supported in xlntx");
    load(stream);
}

void workbook::load(const std::vector<std::uint8_t> &data)
{
    // Create a streambuf from the vector, then load
    // We need to copy data to a string for istringstream
    std::string str(reinterpret_cast<const char *>(data.data()), data.size());
    std::istringstream iss(str);
    load(iss);
}

void workbook::load(const std::vector<std::uint8_t> &data, const std::string &password)
{
    if (!password.empty())
        throw unsupported("encrypted xlsx not supported in xlntx");
    load(data);
}

#ifdef _MSC_VER
void workbook::load(const std::wstring &filename)
{
    load(xlntx::path());
}
void workbook::load(const std::wstring &filename, const std::string &password)
{
    load(filename);
}
#endif

// ===== Save (stubs — read-only) =====

void workbook::save(std::vector<std::uint8_t> &) const { throw unsupported("save not supported in xlntx"); }
void workbook::save(std::vector<std::uint8_t> &, const std::string &) const { throw unsupported("save not supported in xlntx"); }
void workbook::save(const std::string &) const { throw unsupported("save not supported in xlntx"); }
void workbook::save(const std::string &, const std::string &) const { throw unsupported("save not supported in xlntx"); }
#ifdef _MSC_VER
void workbook::save(const std::wstring &) const { throw unsupported("save not supported in xlntx"); }
void workbook::save(const std::wstring &, const std::string &) const { throw unsupported("save not supported in xlntx"); }
#endif
void workbook::save(const xlntx::path &) const { throw unsupported("save not supported in xlntx"); }
void workbook::save(const xlntx::path &, const std::string &) const { throw unsupported("save not supported in xlntx"); }
void workbook::save(std::ostream &) const { throw unsupported("save not supported in xlntx"); }
void workbook::save(std::ostream &, const std::string &) const { throw unsupported("save not supported in xlntx"); }

// ===== Sheet management =====

worksheet workbook::create_sheet() { return create_sheet(d_->worksheets_.size()); }
worksheet workbook::create_sheet(std::size_t) { return worksheet(); }
worksheet workbook::create_sheet_with_rel(const std::string &, const relationship &) { return worksheet(); }
worksheet workbook::copy_sheet(worksheet) { return worksheet(); }
worksheet workbook::copy_sheet(worksheet, std::size_t) { return worksheet(); }

void workbook::remove_sheet(worksheet) {}
void workbook::clear() { d_.reset(new detail::workbook_impl()); }

// ===== Shared strings =====

std::size_t workbook::add_shared_string(const rich_text &shared, bool allow_duplicates)
{
    if (!allow_duplicates)
    {
        auto it = d_->shared_strings_ids_.find(shared);
        if (it != d_->shared_strings_ids_.end())
            return it->second;
    }
    auto idx = d_->next_shared_string_index();
    d_->shared_strings_values_[idx] = shared;
    d_->shared_strings_ids_[shared] = idx;
    return idx;
}

const std::map<std::size_t, rich_text> &workbook::shared_strings_by_id() const
{
    return d_->shared_strings_values_;
}

const rich_text &workbook::shared_strings(std::size_t index) const
{
    static rich_text empty;
    auto it = d_->shared_strings_values_.find(index);
    if (it != d_->shared_strings_values_.end())
        return it->second;
    return empty;
}

std::unordered_map<rich_text, std::size_t, rich_text_hash> &workbook::shared_strings()
{
    return d_->shared_strings_ids_;
}

const std::unordered_map<rich_text, std::size_t, rich_text_hash> &workbook::shared_strings() const
{
    return d_->shared_strings_ids_;
}

// ===== Sheet titles/count =====

std::vector<std::string> workbook::sheet_titles() const
{
    std::vector<std::string> titles;
    for (auto &ws : d_->worksheets_)
        titles.push_back(ws.title_);
    return titles;
}

std::size_t workbook::sheet_count() const
{
    return d_->worksheets_.size();
}

// ===== Metadata/Properties (stubs) =====

bool workbook::has_core_property(xlntx::core_property) const { return false; }
std::vector<xlntx::core_property> workbook::core_properties() const { return {}; }
variant workbook::core_property(xlntx::core_property) const { return variant(); }
void workbook::core_property(xlntx::core_property, const variant &) {}

bool workbook::has_extended_property(xlntx::extended_property) const { return false; }
std::vector<xlntx::extended_property> workbook::extended_properties() const { return {}; }
variant workbook::extended_property(xlntx::extended_property) const { return variant(); }
void workbook::extended_property(xlntx::extended_property, const variant &) {}

bool workbook::has_custom_property(const std::string &) const { return false; }
std::vector<std::string> workbook::custom_properties() const { return {}; }
variant workbook::custom_property(const std::string &) const { return variant(); }
void workbook::custom_property(const std::string &, const variant &) {}

calendar workbook::base_date() const { return d_->base_date_; }
void workbook::base_date(calendar bd) { d_->base_date_ = bd; }

bool workbook::has_title() const { return false; }
std::string workbook::title() const { return ""; }
void workbook::title(const std::string &) {}

void workbook::abs_path(const std::string &p) { d_->abs_path_ = p; }
void workbook::arch_id_flags(const std::size_t f) { d_->arch_id_flags_ = f; }

// ===== Named ranges (stub) =====

std::vector<xlntx::named_range> workbook::named_ranges() const { return {}; }
void workbook::create_named_range(const std::string &, worksheet, const range_reference &) {}
void workbook::create_named_range(const std::string &, worksheet, const std::string &) {}
bool workbook::has_named_range(const std::string &) const { return false; }
class range workbook::named_range(const std::string &) { return range(worksheet(), range_reference()); }
void workbook::remove_named_range(const std::string &) {}

// ===== View (stub) =====

bool workbook::has_view() const { return false; }
workbook_view workbook::view() const { return workbook_view(); }
void workbook::view(const workbook_view &) {}

// ===== Properties (stub) =====

bool workbook::has_code_name() const { return false; }
std::string workbook::code_name() const { return ""; }
void workbook::code_name(const std::string &) {}

bool workbook::has_file_version() const { return false; }
std::string workbook::app_name() const { return ""; }
std::size_t workbook::last_edited() const { return 0; }
std::size_t workbook::lowest_edited() const { return 0; }
std::size_t workbook::rup_build() const { return 0; }

// ===== Theme (stub) =====

bool workbook::has_theme() const { return false; }
const xlntx::theme &workbook::theme() const { static xlntx::theme t; return t; }
void workbook::theme(const class theme &) {}

// ===== Formats (stub) =====

xlntx::format workbook::format(std::size_t) { return xlntx::format(); }
const xlntx::format workbook::format(std::size_t) const { return xlntx::format(); }
xlntx::format workbook::create_format(bool) { return xlntx::format(); }
void workbook::clear_formats() {}

// ===== Styles (stub) =====

bool workbook::has_style(const std::string &) const { return false; }
class style workbook::style(const std::string &) { return class style(); }
const class style workbook::style(const std::string &) const { return class style(); }
class style workbook::create_style(const std::string &) { return class style(); }
class style workbook::create_builtin_style(std::size_t) { return class style(); }
void workbook::clear_styles() {}
void workbook::default_slicer_style(const std::string &) {}
std::string workbook::default_slicer_style() const { return ""; }
void workbook::enable_known_fonts() {}
void workbook::disable_known_fonts() {}
bool workbook::known_fonts_enabled() const { return false; }

// ===== Manifest (stub) =====

class manifest &workbook::manifest() { static class manifest m; return m; }
const class manifest &workbook::manifest() const { static class manifest m; return m; }

// ===== Thumbnail (stub) =====

void workbook::thumbnail(const std::vector<std::uint8_t> &, const std::string &, const std::string &) {}
const std::vector<std::uint8_t> &workbook::thumbnail() const { static std::vector<std::uint8_t> v; return v; }

// ===== Calculation properties (stub) =====

bool workbook::has_calculation_properties() const { return false; }
class calculation_properties workbook::calculation_properties() const { return calculation_properties(); }
void workbook::calculation_properties(const class calculation_properties &) {}

// ===== Operators =====

workbook &workbook::operator=(workbook other) { swap(other); return *this; }
worksheet workbook::operator[](const std::string &name) { return sheet_by_title(name); }
worksheet workbook::operator[](std::size_t index) { return sheet_by_index(index); }
bool workbook::operator==(const workbook &rhs) const { return d_ == rhs.d_; }
bool workbook::operator!=(const workbook &rhs) const { return d_ != rhs.d_; }

// ===== Iterators (stub) =====

workbook::iterator workbook::begin() { return iterator(); }
workbook::iterator workbook::end() { return iterator(); }
workbook::const_iterator workbook::begin() const { return const_iterator(); }
workbook::const_iterator workbook::end() const { return const_iterator(); }
workbook::const_iterator workbook::cbegin() const { return const_iterator(); }
workbook::const_iterator workbook::cend() const { return const_iterator(); }

void workbook::apply_to_cells(std::function<void(cell)>) {}

// ===== Internal =====

detail::workbook_impl &workbook::impl() { return *d_; }
const detail::workbook_impl &workbook::impl() const { return *d_; }
void workbook::register_package_part(relationship_type) {}
void workbook::register_workbook_part(relationship_type) {}
void workbook::register_worksheet_part(worksheet, relationship_type) {}
void workbook::garbage_collect_formulae() {}
void workbook::update_sheet_properties() {}
void workbook::swap(workbook &other) { std::swap(d_, other.d_); }
void workbook::reorder_relationships() {}

} // namespace xlntx

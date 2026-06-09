#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <xlntx/xlntx_config.hpp>
#include <xlntx/cell/rich_text.hpp>
#include <xlntx/packaging/relationship.hpp>
#include <xlntx/workbook/worksheet_iterator.hpp>

namespace xlntx {

enum class calendar;
enum class core_property;
enum class extended_property;

class alignment;
class border;
class calculation_properties;
class cell;
class color;
class fill;
class font;
class format;
class manifest;
class metadata_property;
class named_range;
class number_format;
class path;
class protection;
class range;
class range_reference;
class relationship;
class streaming_workbook_reader;
class style;
class theme;
class variant;
class workbook_view;
class worksheet;
class worksheet_iterator;

struct datetime;

namespace detail {
struct workbook_impl;
class xlsx_reader;
}

class XLNTX_API workbook
{
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

    // Metadata
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

    // Named ranges
    std::vector<xlntx::named_range> named_ranges() const;
    void create_named_range(const std::string &name, worksheet worksheet, const range_reference &reference);
    void create_named_range(const std::string &name, worksheet worksheet, const std::string &reference_string);
    bool has_named_range(const std::string &name) const;
    class range named_range(const std::string &name);
    void remove_named_range(const std::string &name);

    // Serialization
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

    // View
    bool has_view() const;
    workbook_view view() const;
    void view(const workbook_view &view);

    // Properties
    bool has_code_name() const;
    std::string code_name() const;
    void code_name(const std::string &code_name);

    bool has_file_version() const;
    std::string app_name() const;
    std::size_t last_edited() const;
    std::size_t lowest_edited() const;
    std::size_t rup_build() const;

    // Theme
    bool has_theme() const;
    const xlntx::theme &theme() const;
    void theme(const class theme &value);

    // Formats
    xlntx::format format(std::size_t format_index);
    const xlntx::format format(std::size_t format_index) const;
    xlntx::format create_format(bool default_format = false);
    void clear_formats();

    // Styles
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

    // Manifest
    class manifest &manifest();
    const class manifest &manifest() const;

    // Shared strings
    std::size_t add_shared_string(const rich_text &shared, bool allow_duplicates = false);
    const std::map<std::size_t, rich_text> &shared_strings_by_id() const;
    const rich_text &shared_strings(std::size_t index) const;
    std::unordered_map<rich_text, std::size_t, rich_text_hash> &shared_strings();
    const std::unordered_map<rich_text, std::size_t, rich_text_hash> &shared_strings() const;

    // Thumbnail
    void thumbnail(const std::vector<std::uint8_t> &thumbnail,
                   const std::string &extension, const std::string &content_type);
    const std::vector<std::uint8_t> &thumbnail() const;

    // Calculation properties
    bool has_calculation_properties() const;
    class calculation_properties calculation_properties() const;
    void calculation_properties(const class calculation_properties &props);

    // Operators
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

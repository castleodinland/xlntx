#pragma once

#include <cstddef>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <xlntx/cell/rich_text.hpp>
#include <xlntx/utils/calendar.hpp>
#include "worksheet_impl.hpp"

namespace xlntx {

class workbook_view;
class theme;
class calculation_properties;
class manifest;

namespace detail {

struct workbook_impl
{
    std::list<worksheet_impl> worksheets_;
    std::unordered_map<rich_text, std::size_t, rich_text_hash> shared_strings_ids_;
    std::map<std::size_t, rich_text> shared_strings_values_;
    std::unordered_map<std::string, std::string> sheet_title_rel_id_map_;
    std::vector<std::string> sheet_titles_;
    calendar base_date_ = calendar::windows_1900;
    manifest *manifest_ = nullptr;
    theme *theme_ = nullptr;
    calculation_properties *calculation_properties_ = nullptr;
    workbook_view *view_ = nullptr;
    std::string code_name_;
    std::string abs_path_;
    std::size_t arch_id_flags_ = 0;
    bool has_theme_ = false;
    bool has_calculation_properties_ = false;
    bool has_view_ = false;
    bool known_fonts_enabled_ = false;

    std::size_t next_shared_string_index() const
    {
        return shared_strings_values_.empty() ? 0 : shared_strings_values_.rbegin()->first + 1;
    }
};

} // namespace detail
} // namespace xlntx

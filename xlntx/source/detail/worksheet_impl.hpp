#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <xlntx/cell/cell_reference.hpp>
#include <xlntx/cell/index_types.hpp>
#include <xlntx/worksheet/row_properties.hpp>
#include <xlntx/worksheet/column_properties.hpp>
#include <xlntx/worksheet/sheet_view.hpp>
#include <xlntx/worksheet/range_reference.hpp>
#include <xlntx/worksheet/page_setup.hpp>
#include <xlntx/worksheet/page_margins.hpp>
#include <xlntx/worksheet/header_footer.hpp>
#include <xlntx/worksheet/phonetic_pr.hpp>
#include <xlntx/worksheet/sheet_format_properties.hpp>
#include <xlntx/utils/optional.hpp>
#include <xlntx/drawing/spreadsheet_drawing.hpp>
#include "cell_impl.hpp"
#include <vector>
#include <list>

namespace xlntx {

class relationship;

namespace detail {

struct workbook_impl;

struct worksheet_impl
{
    workbook_impl *parent_ = nullptr;
    std::size_t id_ = 0;
    std::string title_;
    std::string sheet_id_;  // rId from workbook.xml
    std::unordered_map<cell_reference, cell_impl, cell_reference_hash> cell_map_;
    std::unordered_map<row_t, row_properties> row_properties_;
    std::unordered_map<column_t, column_properties> column_properties_;
    std::vector<range_reference> merged_cells_;
    std::vector<relationship> hyperlinks_;
    optional<range_reference> auto_filter_;
    optional<sheet_view> view_;
    optional<sheet_format_properties> format_properties_;
    optional<page_setup> page_setup_;
    optional<page_margins> page_margins_;
    optional<header_footer> header_footer_;
    optional<phonetic_pr> phonetic_properties_;
    optional<spreadsheet_drawing> drawing_;
    sheet_state sheet_state_ = sheet_state::visible;
    bool has_drawing_ = false;
};

} // namespace detail
} // namespace xlntx

#pragma once

#include <cstddef>
#include <string>
#include <xlntx/cell/cell_type.hpp>
#include <xlntx/cell/index_types.hpp>
#include <xlntx/cell/rich_text.hpp>
#include <xlntx/utils/optional.hpp>

namespace xlntx {
namespace detail {

struct worksheet_impl;

struct cell_impl
{
    cell_type type_ = cell_type::empty;
    worksheet_impl *parent_ = nullptr;
    column_t::index_t column_ = 1;
    row_t row_ = 1;
    bool is_merged_ = false;
    bool phonetics_visible_ = false;
    rich_text value_text_;
    double value_numeric_ = 0.0;
    optional<std::string> formula_;
    optional<std::size_t> format_index_;
};

} // namespace detail
} // namespace xlntx

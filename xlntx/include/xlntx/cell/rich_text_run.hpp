#pragma once
#include <xlntx/xlntx_config.hpp>
#include <xlntx/styles/font.hpp>
#include <xlntx/utils/optional.hpp>
#include <string>
namespace xlntx {
struct XLNTX_API rich_text_run {
    std::string first;
    optional<font> second;
    bool preserve_space = false;
    bool operator==(const rich_text_run &) const;
    bool operator!=(const rich_text_run &) const;
};
} // namespace xlntx

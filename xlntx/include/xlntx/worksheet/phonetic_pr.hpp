#pragma once
#include <xlntx/xlntx_config.hpp>
namespace xlntx {
struct XLNTX_API phonetic_pr {
    std::uint32_t font_id = 0;
    bool operator==(const phonetic_pr &) const { return true; }
};
} // namespace xlntx

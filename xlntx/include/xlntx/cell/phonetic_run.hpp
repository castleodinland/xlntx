#pragma once
#include <xlntx/xlntx_config.hpp>
#include <string>
namespace xlntx {
struct XLNTX_API phonetic_run {
    std::string text;
    bool operator==(const phonetic_run &) const { return true; }
    bool operator!=(const phonetic_run &) const { return false; }
};
} // namespace xlntx

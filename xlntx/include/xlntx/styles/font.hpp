#pragma once
#include <xlntx/xlntx_config.hpp>
#include <string>
namespace xlntx {
class XLNTX_API font {
public:
    font() = default;
    bool operator==(const font &) const { return true; }
    bool operator!=(const font &) const { return false; }
};
} // namespace xlntx

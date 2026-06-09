#pragma once
#include <xlntx/xlntx_config.hpp>
namespace xlntx {
class XLNTX_API color {
public:
    color() = default;
    bool operator==(const color &) const { return true; }
};
} // namespace xlntx

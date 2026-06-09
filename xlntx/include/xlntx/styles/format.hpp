#pragma once
#include <xlntx/xlntx_config.hpp>
namespace xlntx {
class XLNTX_API format {
public:
    format() = default;
    bool operator==(const format &) const { return true; }
};
} // namespace xlntx

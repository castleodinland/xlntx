#pragma once
#include <xlntx/xlntx_config.hpp>
namespace xlntx {
class XLNTX_API border {
public:
    border() = default;
    bool operator==(const border &) const { return true; }
};
} // namespace xlntx

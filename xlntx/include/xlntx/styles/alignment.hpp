#pragma once
#include <xlntx/xlntx_config.hpp>
namespace xlntx {
class XLNTX_API alignment {
public:
    alignment() = default;
    bool operator==(const alignment &) const { return true; }
};
} // namespace xlntx

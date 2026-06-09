#pragma once
#include <xlntx/xlntx_config.hpp>
namespace xlntx {
class XLNTX_API protection {
public:
    protection() = default;
    bool operator==(const protection &) const { return true; }
};
} // namespace xlntx

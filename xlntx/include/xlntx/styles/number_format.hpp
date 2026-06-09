#pragma once
#include <xlntx/xlntx_config.hpp>
#include <string>
namespace xlntx {
class XLNTX_API number_format {
public:
    number_format() = default;
    bool operator==(const number_format &) const { return true; }
};
} // namespace xlntx

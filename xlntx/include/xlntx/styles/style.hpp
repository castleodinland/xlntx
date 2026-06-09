#pragma once
#include <xlntx/xlntx_config.hpp>
#include <string>
namespace xlntx {
class XLNTX_API style {
public:
    style() = default;
    bool operator==(const style &) const { return true; }
};
} // namespace xlntx

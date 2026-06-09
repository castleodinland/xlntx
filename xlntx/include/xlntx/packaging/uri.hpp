#pragma once
#include <xlntx/xlntx_config.hpp>
#include <string>
namespace xlntx {
class XLNTX_API uri {
public:
    uri() = default;
    explicit uri(const std::string &uri_string);
    std::string to_string() const;
private:
    std::string uri_string_;
};
} // namespace xlntx

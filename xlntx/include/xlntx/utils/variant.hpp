#pragma once
#include <xlntx/xlntx_config.hpp>
#include <xlntx/utils/datetime.hpp>
#include <string>
#include <vector>
#include <cstdint>
namespace xlntx {
class XLNTX_API variant {
public:
    enum class type { vector, null, i4, lpstr, date, boolean };
    variant() = default;
    variant(const std::string &) {}
    variant(const char *) {}
    variant(std::int32_t) {}
    variant(bool) {}
    variant(const datetime &) {}
    variant(const std::initializer_list<std::int32_t> &) {}
    variant(const std::vector<std::int32_t> &) {}
    variant(const std::initializer_list<std::string> &) {}
    variant(const std::vector<std::string> &) {}
    variant(const std::vector<variant> &) {}
    bool is(type t) const { return type_ == t; }
    template<typename T> T get() const;
    type value_type() const { return type_; }
    bool operator==(const variant &) const { return true; }
private:
    type type_ = type::null;
    std::vector<variant> vector_value_;
    std::int32_t i4_value_ = 0;
    std::string lpstr_value_;
};
} // namespace xlntx

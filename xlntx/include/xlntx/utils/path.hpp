#pragma once
#include <xlntx/xlntx_config.hpp>
#include <string>
#include <vector>
#include <cstddef>
namespace xlntx {
class XLNTX_API path {
public:
    static char system_separator();
    path() = default;
    path(const std::string &path_string);
    path(const std::string &path_string, char sep);
    bool is_relative() const;
    bool is_absolute() const;
    bool is_root() const;
    path parent() const;
    std::string filename() const;
    std::string extension() const;
    std::pair<std::string, std::string> split_extension() const;
    std::vector<std::string> split() const;
    const std::string &string() const;
    bool exists() const;
    bool is_directory() const;
    bool is_file() const;
    std::string read_contents() const;
    path append(const std::string &part) const;
    path append(const path &p) const;
    path resolve(const path &base) const;
    path relative_to(const path &base) const;
    bool operator==(const path &other) const;
    bool operator!=(const path &other) const;
private:
    std::string internal_;
};
} // namespace xlntx

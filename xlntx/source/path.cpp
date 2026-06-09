#include <xlntx/utils/path.hpp>
#include <algorithm>
#include <sstream>

namespace xlntx {

char path::system_separator() { return '\\'; }

path::path(const std::string &path_string) : internal_(path_string) {}

path::path(const std::string &path_string, char /*sep*/) : internal_(path_string) {}

bool path::is_relative() const
{
    return !internal_.empty() && internal_[0] != '/' && internal_[0] != '\\'
        && (internal_.size() < 2 || internal_[1] != ':');
}

bool path::is_absolute() const { return !is_relative(); }

bool path::is_root() const
{
    return internal_ == "/" || internal_ == "\\" ||
           (internal_.size() == 3 && internal_[1] == ':' && (internal_[2] == '\\' || internal_[2] == '/'));
}

path path::parent() const
{
    auto pos = internal_.find_last_of("/\\");
    if (pos == std::string::npos) return path();
    if (pos == internal_.size() - 1)
    {
        pos = internal_.find_last_of("/\\", pos - 1);
        if (pos == std::string::npos) return path();
    }
    if (pos == 2 && internal_[1] == ':') return path(internal_.substr(0, 3));
    return path(internal_.substr(0, pos));
}

std::string path::filename() const
{
    auto pos = internal_.find_last_of("/\\");
    if (pos == std::string::npos) return internal_;
    return internal_.substr(pos + 1);
}

std::string path::extension() const
{
    auto fname = filename();
    auto pos = fname.rfind('.');
    if (pos == std::string::npos) return "";
    return fname.substr(pos);
}

std::pair<std::string, std::string> path::split_extension() const
{
    auto fname = filename();
    auto pos = fname.rfind('.');
    if (pos == std::string::npos) return {fname, ""};
    return {fname.substr(0, pos), fname.substr(pos)};
}

std::vector<std::string> path::split() const
{
    std::vector<std::string> parts;
    if (internal_.empty()) return parts;
    std::istringstream iss(internal_);
    std::string part;
    while (std::getline(iss, part, '/'))
    {
        std::istringstream iss2(part);
        std::string subpart;
        while (std::getline(iss2, subpart, '\\'))
        {
            if (!subpart.empty())
                parts.push_back(subpart);
        }
    }
    return parts;
}

const std::string &path::string() const { return internal_; }

bool path::exists() const
{
    // Stub: always returns false (read-only library, no FS access needed)
    return false;
}

bool path::is_directory() const { return false; }
bool path::is_file() const { return false; }

std::string path::read_contents() const
{
    // Stub
    return "";
}

path path::append(const std::string &part) const
{
    if (internal_.empty()) return path(part);
    if (internal_.back() == '/' || internal_.back() == '\\')
        return path(internal_ + part);
    return path(internal_ + "\\" + part);
}

path path::append(const path &p) const { return append(p.string()); }

path path::resolve(const path &base) const
{
    if (is_absolute()) return *this;
    return base.append(internal_);
}

path path::relative_to(const path & /*base*/) const
{
    // Stub
    return *this;
}

bool path::operator==(const path &other) const { return internal_ == other.internal_; }
bool path::operator!=(const path &other) const { return internal_ != other.internal_; }

} // namespace xlntx

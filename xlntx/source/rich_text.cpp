#include <xlntx/cell/rich_text.hpp>
#include <xlntx/cell/rich_text_run.hpp>

namespace xlntx {

// ===== rich_text =====

rich_text::rich_text(const std::string &plain_text)
{
    rich_text_run run;
    run.first = plain_text;
    runs_.push_back(run);
}

rich_text::rich_text(const std::string &text, const class font &f)
{
    rich_text_run run;
    run.first = text;
    run.second = f;
    runs_.push_back(run);
}

rich_text::rich_text(const rich_text_run &run)
{
    runs_.push_back(run);
}

void rich_text::clear()
{
    runs_.clear();
    phonetic_runs_.clear();
    phonetic_properties_.reset();
}

void rich_text::plain_text(const std::string &s, bool preserve_space)
{
    clear();
    rich_text_run run;
    run.first = s;
    run.preserve_space = preserve_space;
    runs_.push_back(run);
}

std::string rich_text::plain_text() const
{
    std::string result;
    for (const auto &run : runs_)
        result += run.first;
    return result;
}

bool rich_text::operator==(const rich_text &other) const
{
    return runs_ == other.runs_ &&
           phonetic_runs_ == other.phonetic_runs_;
}

bool rich_text::operator!=(const rich_text &other) const
{
    return !(*this == other);
}

bool rich_text::operator==(const std::string &other) const
{
    return plain_text() == other;
}

bool rich_text::operator!=(const std::string &other) const
{
    return plain_text() != other;
}

// ===== rich_text_run =====

bool rich_text_run::operator==(const rich_text_run &other) const
{
    return first == other.first && preserve_space == other.preserve_space;
}

bool rich_text_run::operator!=(const rich_text_run &other) const
{
    return !(*this == other);
}

// ===== rich_text_hash =====

std::size_t rich_text_hash::operator()(const rich_text &k) const
{
    std::size_t h = 0;
    for (const auto &run : k.runs())
    {
        h ^= std::hash<std::string>{}(run.first) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}

} // namespace xlntx

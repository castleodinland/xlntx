#pragma once
#include <xlntx/xlntx_config.hpp>
#include <xlntx/cell/rich_text_run.hpp>
#include <xlntx/cell/phonetic_run.hpp>
#include <xlntx/worksheet/phonetic_pr.hpp>
#include <xlntx/utils/optional.hpp>
#include <string>
#include <vector>
#include <cstddef>
namespace xlntx {
class XLNTX_API rich_text {
public:
    rich_text() = default;
    explicit rich_text(const std::string &plain_text);
    rich_text(const std::string &text, const class font &);
    explicit rich_text(const rich_text_run &);
    void clear();
    void plain_text(const std::string &s, bool preserve_space);
    std::string plain_text() const;
    const std::vector<rich_text_run> &runs() const { return runs_; }
    void runs(const std::vector<rich_text_run> &r) { runs_ = r; }
    void add_run(const rich_text_run &run) { runs_.push_back(run); }
    const std::vector<phonetic_run> &phonetic_runs() const { return phonetic_runs_; }
    void phonetic_runs(const std::vector<phonetic_run> &r) { phonetic_runs_ = r; }
    void add_phonetic_run(const phonetic_run &run) { phonetic_runs_.push_back(run); }
    bool has_phonetic_properties() const { return phonetic_properties_.is_set(); }
    const phonetic_pr &phonetic_properties() const { return phonetic_properties_.get(); }
    void phonetic_properties(const phonetic_pr &pp) { phonetic_properties_ = pp; }
    rich_text &operator=(const rich_text &) = default;
    bool operator==(const rich_text &other) const;
    bool operator!=(const rich_text &other) const;
    bool operator==(const std::string &other) const;
    bool operator!=(const std::string &other) const;
private:
    std::vector<rich_text_run> runs_;
    std::vector<phonetic_run> phonetic_runs_;
    optional<phonetic_pr> phonetic_properties_;
};
struct rich_text_hash {
    std::size_t operator()(const rich_text &k) const;
};
} // namespace xlntx

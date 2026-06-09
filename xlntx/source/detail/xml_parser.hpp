#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <string>
#include <vector>

namespace xlntx {
namespace detail {

enum class xml_parser_event
{
    start_element,
    end_element,
    characters,
    start_namespace_decl,
    end_namespace_decl,
    eof
};

class xml_parser
{
public:
    explicit xml_parser(std::istream &stream, const std::string &name = "");

    xml_parser_event next();
    xml_parser_event peek();
    const std::string &name() const;
    const std::string &value() const;
    const std::string &attribute(const std::string &attr_name) const;
    bool attribute_present(const std::string &attr_name) const;
    const std::map<std::string, std::string> &attribute_map() const;
    std::string content();

private:
    void parse_attributes(const std::string &tag_content);
    void trim_value();

    std::istream &stream_;
    std::string document_name_;
    std::string current_name_;
    std::string current_value_;
    std::map<std::string, std::string> attributes_;
    xml_parser_event current_event_ = xml_parser_event::eof;
    xml_parser_event peeked_event_ = xml_parser_event::eof;
    bool peeked_ = false;
    bool pending_self_close_ = false;
    std::string pending_close_name_;
    std::map<std::string, std::string> pending_close_attrs_;
    std::vector<std::string> element_stack_;
    std::vector<char> buffer_;
    std::size_t buf_pos_ = 0;

    bool read_next_event();
    int get_char();
    void skip_whitespace();
    void skip_bom();
    std::string read_name();
    void skip_until_char(char c);
    void skip_until(const std::string &s);
    std::string read_until_char(char c);
    void collapse_whitespace(std::string &s);
};

inline bool string_equal(const char *s1, const char *s2, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i)
    {
        if (s1[i] != s2[i]) return false;
        if (s1[i] == '\0') return true;
    }
    return true;
}

inline bool string_equal(const char *s1, const std::string &s2)
{
    return s2 == s1;
}

} // namespace detail
} // namespace xlntx

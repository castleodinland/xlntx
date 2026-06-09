#include "xml_parser.hpp"

#include <cctype>
#include <cstring>
#include <istream>
#include <stdexcept>

namespace xlntx {
namespace detail {

xml_parser::xml_parser(std::istream &stream, const std::string &name)
    : stream_(stream), document_name_(name)
{
    // Read entire stream into buffer
    buffer_.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    buf_pos_ = 0;
    skip_bom();
}

int xml_parser::get_char()
{
    if (buf_pos_ >= buffer_.size()) return -1;
    return static_cast<unsigned char>(buffer_[buf_pos_++]);
}

void xml_parser::skip_bom()
{
    if (buffer_.size() >= 3 &&
        static_cast<unsigned char>(buffer_[0]) == 0xEF &&
        static_cast<unsigned char>(buffer_[1]) == 0xBB &&
        static_cast<unsigned char>(buffer_[2]) == 0xBF)
    {
        buf_pos_ = 3;
    }
}

void xml_parser::skip_whitespace()
{
    while (buf_pos_ < buffer_.size())
    {
        char c = buffer_[buf_pos_];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            ++buf_pos_;
        else
            break;
    }
}

std::string xml_parser::read_name()
{
    std::string result;
    while (buf_pos_ < buffer_.size())
    {
        char c = buffer_[buf_pos_];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == ':' || c == '.')
        {
            result.push_back(c);
            ++buf_pos_;
        }
        else
        {
            break;
        }
    }
    return result;
}

void xml_parser::skip_until_char(char target)
{
    while (buf_pos_ < buffer_.size())
    {
        if (buffer_[buf_pos_] == target)
            break;
        ++buf_pos_;
    }
}

void xml_parser::skip_until(const std::string &s)
{
    while (buf_pos_ + s.size() <= buffer_.size())
    {
        if (std::memcmp(&buffer_[buf_pos_], s.data(), s.size()) == 0)
            break;
        ++buf_pos_;
    }
}

std::string xml_parser::read_until_char(char target)
{
    std::string result;
    while (buf_pos_ < buffer_.size())
    {
        char c = buffer_[buf_pos_];
        if (c == target) break;
        result.push_back(c);
        ++buf_pos_;
    }
    return result;
}

void xml_parser::collapse_whitespace(std::string &s)
{
    std::string result;
    bool in_whitespace = false;
    for (char c : s)
    {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            if (!in_whitespace && !result.empty())
            {
                in_whitespace = true;
                result.push_back(' ');
            }
        }
        else
        {
            in_whitespace = false;
            result.push_back(c);
        }
    }
    // Trim trailing whitespace
    while (!result.empty() && result.back() == ' ')
        result.pop_back();
    // Trim leading whitespace
    if (!result.empty() && result.front() == ' ')
        result.erase(0, 1);
    s = std::move(result);
}

void xml_parser::parse_attributes(const std::string &tag_content)
{
    attributes_.clear();
    std::size_t pos = 0;
    std::size_t len = tag_content.size();

    while (pos < len)
    {
        // Skip whitespace
        while (pos < len && (tag_content[pos] == ' ' || tag_content[pos] == '\t' ||
               tag_content[pos] == '\n' || tag_content[pos] == '\r'))
            ++pos;
        if (pos >= len) break;

        // Read attribute name
        std::string attr_name;
        while (pos < len && tag_content[pos] != '=' && tag_content[pos] != ' ' &&
               tag_content[pos] != '\t' && tag_content[pos] != '\n' && tag_content[pos] != '\r' &&
               tag_content[pos] != '/' && tag_content[pos] != '>')
        {
            attr_name.push_back(tag_content[pos++]);
        }

        if (attr_name.empty()) break;

        // Strip namespace prefix from attribute name
        auto colon_pos = attr_name.find(':');
        if (colon_pos != std::string::npos)
            attr_name = attr_name.substr(colon_pos + 1);

        // Skip whitespace before =
        while (pos < len && (tag_content[pos] == ' ' || tag_content[pos] == '\t' ||
               tag_content[pos] == '\n' || tag_content[pos] == '\r'))
            ++pos;

        if (pos >= len || tag_content[pos] != '=') break;
        ++pos; // skip =

        // Skip whitespace after =
        while (pos < len && (tag_content[pos] == ' ' || tag_content[pos] == '\t' ||
               tag_content[pos] == '\n' || tag_content[pos] == '\r'))
            ++pos;

        if (pos >= len) break;

        // Read attribute value
        char quote = tag_content[pos];
        if (quote == '"' || quote == '\'')
        {
            ++pos;
            std::string attr_value;
            while (pos < len && tag_content[pos] != quote)
            {
                if (tag_content[pos] == '&')
                {
                    // Handle XML entities
                    if (pos + 3 < len && tag_content.substr(pos, 4) == "&lt;")
                        { attr_value.push_back('<'); pos += 4; }
                    else if (pos + 3 < len && tag_content.substr(pos, 4) == "&gt;")
                        { attr_value.push_back('>'); pos += 4; }
                    else if (pos + 4 < len && tag_content.substr(pos, 5) == "&amp;")
                        { attr_value.push_back('&'); pos += 5; }
                    else if (pos + 5 < len && tag_content.substr(pos, 6) == "&quot;")
                        { attr_value.push_back('"'); pos += 6; }
                    else if (pos + 5 < len && tag_content.substr(pos, 6) == "&apos;")
                        { attr_value.push_back('\''); pos += 6; }
                    else if (pos + 2 < len && tag_content[pos+1] == '#')
                    {
                        // Numeric entity - simplified
                        std::size_t end = tag_content.find(';', pos);
                        if (end != std::string::npos)
                        {
                            pos = end + 1;
                            attr_value.push_back('?');
                        }
                        else
                        {
                            attr_value.push_back(tag_content[pos++]);
                        }
                    }
                    else
                    {
                        attr_value.push_back(tag_content[pos++]);
                    }
                }
                else
                {
                    attr_value.push_back(tag_content[pos++]);
                }
            }
            if (pos < len) ++pos; // skip closing quote
            attributes_[attr_name] = attr_value;
        }
    }
}

bool xml_parser::read_next_event()
{
    if (buf_pos_ >= buffer_.size())
    {
        current_event_ = xml_parser_event::eof;
        return false;
    }

    // Skip whitespace, comments, and processing instructions
    while (buf_pos_ < buffer_.size())
    {
        char c = buffer_[buf_pos_];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            ++buf_pos_;
            continue;
        }
        if (c == '<')
        {
            if (buf_pos_ + 1 < buffer_.size())
                break;
        }
        // Text content between tags
        if (c != '<')
        {
            std::string text;
            while (buf_pos_ < buffer_.size() && buffer_[buf_pos_] != '<')
            {
                char ch = buffer_[buf_pos_];
                if (ch == '&')
                {
                    if (buf_pos_ + 3 < buffer_.size())
                    {
                        std::string_view sv(&buffer_[buf_pos_], buffer_.size() - buf_pos_);
                        if (sv.substr(0, 4) == "&lt;")
                            { text.push_back('<'); buf_pos_ += 4; continue; }
                        if (sv.substr(0, 4) == "&gt;")
                            { text.push_back('>'); buf_pos_ += 4; continue; }
                    }
                    if (buf_pos_ + 4 < buffer_.size())
                    {
                        std::string_view sv(&buffer_[buf_pos_], buffer_.size() - buf_pos_);
                        if (sv.substr(0, 5) == "&amp;")
                            { text.push_back('&'); buf_pos_ += 5; continue; }
                    }
                    if (buf_pos_ + 5 < buffer_.size())
                    {
                        std::string_view sv(&buffer_[buf_pos_], buffer_.size() - buf_pos_);
                        if (sv.substr(0, 6) == "&quot;")
                            { text.push_back('"'); buf_pos_ += 6; continue; }
                        if (sv.substr(0, 6) == "&apos;")
                            { text.push_back('\''); buf_pos_ += 6; continue; }
                    }
                }
                text.push_back(ch);
                ++buf_pos_;
            }
            current_value_ = text;
            current_event_ = xml_parser_event::characters;
            return true;
        }
        break;
    }

    if (buf_pos_ >= buffer_.size())
    {
        current_event_ = xml_parser_event::eof;
        return false;
    }

    // We're at '<'
    ++buf_pos_; // skip '<'

    if (buf_pos_ >= buffer_.size())
    {
        current_event_ = xml_parser_event::eof;
        return false;
    }

    char next = buffer_[buf_pos_];

    // Comments: <!-- ... -->
    if (next == '!' && buf_pos_ + 2 < buffer_.size() &&
        buffer_[buf_pos_ + 1] == '-' && buffer_[buf_pos_ + 2] == '-')
    {
        buf_pos_ += 3; // skip !--
        skip_until("-->");
        if (buf_pos_ < buffer_.size()) buf_pos_ += 3;
        return read_next_event(); // recurse to get next event
    }

    // Processing instruction: <? ... ?>
    if (next == '?')
    {
        ++buf_pos_; // skip ?
        skip_until("?>");
        if (buf_pos_ < buffer_.size()) buf_pos_ += 2;
        return read_next_event();
    }

    // CDATA: <![CDATA[ ... ]]>
    if (next == '!' && buf_pos_ + 7 < buffer_.size() &&
        std::memcmp(&buffer_[buf_pos_], "![CDATA[", 8) == 0)
    {
        buf_pos_ += 8; // skip ![CDATA[
        std::string cdata = read_until_char(']');
        // skip ]]>
        while (buf_pos_ < buffer_.size() && buffer_[buf_pos_] == ']') ++buf_pos_;
        if (buf_pos_ < buffer_.size() && buffer_[buf_pos_] == '>') ++buf_pos_;
        current_value_ = cdata;
        current_event_ = xml_parser_event::characters;
        return true;
    }

    // End element: </name>
    if (next == '/')
    {
        ++buf_pos_; // skip /
        std::string name = read_name();
        // Strip namespace prefix
        auto colon = name.find(':');
        if (colon != std::string::npos) name = name.substr(colon + 1);
        current_name_ = name;
        // Skip to end of tag
        skip_until_char('>');
        if (buf_pos_ < buffer_.size()) ++buf_pos_;
        current_value_.clear();
        attributes_.clear();

        if (!element_stack_.empty() && element_stack_.back() == name)
            element_stack_.pop_back();

        current_event_ = xml_parser_event::end_element;
        return true;
    }

    // Start element: <name attr="val" ...>
    // Find end of tag (self-closing: /> or just >)
    std::size_t tag_start = buf_pos_;
    bool self_closing = false;

    // Read until > (handling quoted strings)
    char quote_char = 0;
    while (buf_pos_ < buffer_.size())
    {
        char c = buffer_[buf_pos_];
        if (quote_char)
        {
            if (c == quote_char) quote_char = 0;
        }
        else
        {
            if (c == '"' || c == '\'') quote_char = c;
            else if (c == '>') break;
        }
        ++buf_pos_;
    }

    if (buf_pos_ >= buffer_.size())
    {
        current_event_ = xml_parser_event::eof;
        return false;
    }

    std::string tag_content(&buffer_[tag_start], buf_pos_ - tag_start);

    // Check for self-closing
    if (!tag_content.empty() && tag_content.back() == '/')
    {
        self_closing = true;
        tag_content.pop_back();
    }

    ++buf_pos_; // skip >

    // Extract element name
    std::size_t name_start = 0;
    while (name_start < tag_content.size() &&
           (tag_content[name_start] == ' ' || tag_content[name_start] == '\t'))
        ++name_start;

    std::size_t name_end = name_start;
    while (name_end < tag_content.size() && tag_content[name_end] != ' ' &&
           tag_content[name_end] != '\t' && tag_content[name_end] != '\n')
        ++name_end;

    std::string name = tag_content.substr(name_start, name_end - name_start);

    // Strip namespace prefix
    auto colon = name.find(':');
    if (colon != std::string::npos) name = name.substr(colon + 1);

    current_name_ = name;
    current_value_.clear();

    // Parse attributes from the remaining tag content (after name)
    std::string attr_part;
    if (name_end < tag_content.size())
        attr_part = tag_content.substr(name_end);

    parse_attributes(attr_part);

    if (self_closing)
    {
        pending_self_close_ = true;
        pending_close_name_ = name;
        pending_close_attrs_ = attributes_;
    }
    else
    {
        element_stack_.push_back(name);
    }

    current_event_ = xml_parser_event::start_element;
    return true;
}

xml_parser_event xml_parser::next()
{
    if (peeked_)
    {
        peeked_ = false;
        xml_parser_event ev = current_event_;
        // If we peeked a start_element that was self-closing, synthesize end_element
        if (pending_self_close_)
        {
            pending_self_close_ = false;
            current_name_ = pending_close_name_;
            attributes_ = pending_close_attrs_;
            current_value_.clear();
            current_event_ = xml_parser_event::end_element;
            // return the start_element we peeked, save end_element for next call
            peeked_ = true;
            peeked_event_ = current_event_;
        }
        return ev;
    }

    // Synthesize end_element for previously seen self-closing tag
    if (pending_self_close_)
    {
        pending_self_close_ = false;
        current_name_ = pending_close_name_;
        attributes_ = pending_close_attrs_;
        current_value_.clear();
        current_event_ = xml_parser_event::end_element;
        return current_event_;
    }

    read_next_event();
    return current_event_;
}

xml_parser_event xml_parser::peek()
{
    if (!peeked_)
    {
        peeked_event_ = current_event_;
        read_next_event();
        peeked_ = true;
    }
    return current_event_;
}

const std::string &xml_parser::name() const
{
    return current_name_;
}

const std::string &xml_parser::value() const
{
    return current_value_;
}

const std::string &xml_parser::attribute(const std::string &attr_name) const
{
    static const std::string empty;
    auto it = attributes_.find(attr_name);
    if (it != attributes_.end()) return it->second;
    return empty;
}

bool xml_parser::attribute_present(const std::string &attr_name) const
{
    return attributes_.find(attr_name) != attributes_.end();
}

const std::map<std::string, std::string> &xml_parser::attribute_map() const
{
    return attributes_;
}

void xml_parser::trim_value()
{
    // Trim leading whitespace
    auto start = current_value_.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
    {
        current_value_.clear();
        return;
    }
    auto end = current_value_.find_last_not_of(" \t\n\r");
    current_value_ = current_value_.substr(start, end - start + 1);
}

std::string xml_parser::content()
{
    // Read all text content until next start/end element
    if (current_event_ != xml_parser_event::start_element)
        return std::string();

    std::string text;
    while (true)
    {
        auto ev = next();
        if (ev == xml_parser_event::characters)
            text += current_value_;
        else if (ev == xml_parser_event::start_element)
        {
            // Read and skip this nested element and its content
            int depth = 1;
            while (depth > 0)
            {
                auto inner = next();
                if (inner == xml_parser_event::start_element) ++depth;
                else if (inner == xml_parser_event::end_element) --depth;
                else if (inner == xml_parser_event::eof) break;
            }
        }
        else
            break;
    }

    collapse_whitespace(text);
    current_value_ = text;
    current_event_ = xml_parser_event::characters;
    return text;
}

} // namespace detail
} // namespace xlntx

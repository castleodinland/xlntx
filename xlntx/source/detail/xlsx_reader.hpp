#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

namespace xlntx {

class cell_reference;
class rich_text;
enum class relationship_type;

namespace detail {

struct workbook_impl;
struct worksheet_impl;
struct cell_impl;
class zip_reader;
class xml_parser;

class xlsx_reader
{
public:
    xlsx_reader(workbook_impl &wb);

    void read(std::istream &stream);

private:
    void populate_workbook(zip_reader &zip);

    // Content types and relationships
    void read_content_types(zip_reader &zip);
    std::vector<std::pair<std::string, std::string>> read_relationships(
        zip_reader &zip, const std::string &part_path);

    // Core document parts
    void read_office_document(zip_reader &zip);
    void read_shared_string_table(zip_reader &zip);
    void read_stylesheet(zip_reader &zip);
    void read_worksheet(zip_reader &zip, worksheet_impl &ws);

    // XML parsing helpers
    rich_text read_rich_text(xml_parser &parser);
    std::string read_text(xml_parser &parser);

    // Fast sheet data parsing
    void parse_sheet_data(xml_parser &parser, worksheet_impl &ws);

    // XML navigation helpers
    void expect_start_element(xml_parser &parser, const std::string &name);
    void expect_end_element(xml_parser &parser, const std::string &name);
    bool in_element(xml_parser &parser, const std::string &name);
    void skip_element(xml_parser &parser);

    workbook_impl &wb_;

    // Paths discovered from relationships
    std::string shared_strings_path_;
    std::string styles_path_;
    std::vector<std::string> worksheet_paths_;
    std::string theme_path_;

    // Content type -> part path mapping
    std::unordered_map<std::string, std::string> default_types_;
    std::unordered_map<std::string, std::string> override_types_;
};

} // namespace detail
} // namespace xlntx

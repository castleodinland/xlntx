#include "xlsx_reader.hpp"

#include "xml_parser.hpp"
#include "zip_reader.hpp"
#include "workbook_impl.hpp"
#include "worksheet_impl.hpp"
#include "cell_impl.hpp"

#include <xlntx/cell/cell_reference.hpp>
#include <xlntx/cell/cell_type.hpp>
#include <xlntx/cell/rich_text.hpp>
#include <xlntx/cell/rich_text_run.hpp>
#include <xlntx/utils/exceptions.hpp>
#include <xlntx/packaging/relationship.hpp>

#include <cstring>
#include <sstream>
#include <vector>

namespace xlntx {
namespace detail {

namespace {

// OOXML namespace URIs
const char *ns_spreadsheetml = "http://schemas.openxmlformats.org/spreadsheetml/2006/main";
const char *ns_relationships = "http://schemas.openxmlformats.org/officeDocument/2006/relationships";
const char *ns_content_types = "http://schemas.openxmlformats.org/package/2006/content-types";
const char *ns_mc = "http://schemas.openxmlformats.org/markup-compatibility/2006";

bool name_is(const std::string &name, const char *expected)
{
    return name == expected;
}

cell_type type_from_string(const std::string &str)
{
    if (str == "s") return cell_type::shared_string;
    if (str == "n") return cell_type::number;
    if (str == "b") return cell_type::boolean;
    if (str == "e") return cell_type::error;
    if (str == "inlineStr") return cell_type::inline_string;
    if (str == "str") return cell_type::formula_string;
    return cell_type::empty;
}

// Safe numeric parsing — returns false on failure instead of throwing
bool safe_stoull(const std::string &s, std::size_t &out)
{
    if (s.empty()) return false;
    try { out = static_cast<std::size_t>(std::stoull(s)); return true; }
    catch (const std::exception &) { return false; }
}

bool safe_stod(const std::string &s, double &out)
{
    if (s.empty()) return false;
    try { out = std::stod(s); return true; }
    catch (const std::exception &) { return false; }

}

std::string resolve_relationship_target(const std::string &base_path, const std::string &target)
{
    // base_path is like "xl/workbook.xml", target is like "worksheets/sheet1.xml"
    // Result: "xl/worksheets/sheet1.xml"
    auto slash = base_path.rfind('/');
    if (slash != std::string::npos)
        return base_path.substr(0, slash + 1) + target;
    return target;
}

} // anonymous namespace

xlsx_reader::xlsx_reader(workbook_impl &wb) : wb_(wb) {}

void xlsx_reader::read(std::istream &stream)
{
    zip_reader zip(stream);
    populate_workbook(zip);
}

// ===== Navigation helpers =====

void xlsx_reader::expect_start_element(xml_parser &parser, const std::string &name)
{
    auto ev = parser.next();
    if (ev != xml_parser_event::start_element || parser.name() != name)
        throw invalid_attribute();
}

void xlsx_reader::expect_end_element(xml_parser &parser, const std::string &name)
{
    auto ev = parser.next();
    if (ev != xml_parser_event::end_element || parser.name() != name)
        throw invalid_attribute();
}

bool xlsx_reader::in_element(xml_parser &parser, const std::string &name)
{
    auto ev = parser.peek();
    if (ev == xml_parser_event::start_element && parser.name() == name)
    {
        parser.next(); // consume start
        return true;
    }
    if (ev == xml_parser_event::end_element)
    {
        if (parser.name() == name)
            parser.next(); // consume end
        return false;
    }
    if (ev == xml_parser_event::eof)
        return false;
    return false;
}

void xlsx_reader::skip_element(xml_parser &parser)
{
    int depth = 1;
    while (depth > 0)
    {
        auto ev = parser.next();
        if (ev == xml_parser_event::start_element) ++depth;
        else if (ev == xml_parser_event::end_element) --depth;
        else if (ev == xml_parser_event::eof) break;
    }
}

// ===== Content types =====

void xlsx_reader::read_content_types(zip_reader &zip)
{
    if (!zip.has_file("[Content_Types].xml")) return;

    auto xml_str = zip.read("[Content_Types].xml");
    std::istringstream iss(xml_str);
    xml_parser parser(iss);

    while (parser.next() != xml_parser_event::eof)
    {
        if (parser.name() == "Default")
        {
            auto ext = parser.attribute("Extension");
            auto ct = parser.attribute("ContentType");
            if (!ext.empty() && !ct.empty())
                default_types_[ct] = ext;
        }
        else if (parser.name() == "Override")
        {
            auto part = parser.attribute("PartName");
            auto ct = parser.attribute("ContentType");
            if (!part.empty() && !ct.empty())
                override_types_[ct] = part;
        }
    }
}

// ===== Relationships =====

std::vector<std::pair<std::string, std::string>> xlsx_reader::read_relationships(
    zip_reader &zip, const std::string &part_path)
{
    std::vector<std::pair<std::string, std::string>> rels;
    if (!zip.has_file(part_path)) return rels;

    auto xml_str = zip.read(part_path);
    std::istringstream iss(xml_str);
    xml_parser parser(iss);

    while (parser.next() != xml_parser_event::eof)
    {
        if (parser.name() == "Relationship")
        {
            std::string id = parser.attribute("Id");
            std::string type = parser.attribute("Type");
            std::string target = parser.attribute("Target");
            if (!id.empty())
                rels.emplace_back(id, target);
        }
    }
    return rels;
}

// ===== Office document (xl/workbook.xml) =====

void xlsx_reader::read_office_document(zip_reader &zip)
{
    if (!zip.has_file("xl/workbook.xml")) return;

    auto xml_str = zip.read("xl/workbook.xml");
    std::istringstream iss(xml_str);
    xml_parser parser(iss);

    // Parse workbook XML
    bool in_workbook = false;
    bool in_sheets = false;

    while (true)
    {
        auto ev = parser.next();
        if (ev == xml_parser_event::eof) break;

        if (ev == xml_parser_event::start_element)
        {
            const auto &name = parser.name();

            if (name == "workbook")
            {
                in_workbook = true;
            }
            else if (name == "sheets")
            {
                in_sheets = true;
            }
            else if (name == "sheet" && in_sheets)
            {
                std::string sheet_name = parser.attribute("name");
                std::string rId = parser.attribute("id");
                std::string sheet_id_str = parser.attribute("sheetId");

                if (!sheet_name.empty() && !rId.empty())
                {
                    wb_.sheet_title_rel_id_map_[sheet_name] = rId;
                    wb_.sheet_titles_.push_back(sheet_name);

                    // Create worksheet impl
                    worksheet_impl ws;
                    ws.parent_ = &wb_;
                    ws.title_ = sheet_name;
                    ws.id_ = wb_.worksheets_.size() + 1;
                    ws.sheet_id_ = rId;
                    wb_.worksheets_.push_back(std::move(ws));
                }
            }
            else if (name == "workbookPr")
            {
                auto date1904 = parser.attribute("date1904");
                if (date1904 == "1" || date1904 == "true")
                    wb_.base_date_ = calendar::mac_1904;
            }
            else if (name == "fileVersion")
            {
                // Skip - read and ignore
            }
            else if (name == "bookViews")
            {
                // Skip
            }
            else if (name == "calcPr")
            {
                // Skip
            }
        }
    }
}

// ===== Shared string table =====

void xlsx_reader::read_shared_string_table(zip_reader &zip)
{
    if (!zip.has_file("xl/sharedStrings.xml")) return;

    auto xml_str = zip.read("xl/sharedStrings.xml");
    std::istringstream iss(xml_str);
    xml_parser parser(iss);

    // Find sst start
    while (parser.next() != xml_parser_event::eof)
    {
        if (parser.name() == "sst")
            break;
    }

    // Read each <si> element
    while (parser.peek() != xml_parser_event::eof)
    {
        auto ev = parser.peek();
        if (ev == xml_parser_event::start_element && parser.name() == "si")
        {
            parser.next(); // consume start
            rich_text rt = read_rich_text(parser);
            // expect_end_element for "si" should have been consumed by read_rich_text
            wb_.shared_strings_values_[wb_.shared_strings_values_.size()] = rt;
            wb_.shared_strings_ids_[rt] = wb_.shared_strings_values_.size() - 1;
        }
        else if (ev == xml_parser_event::end_element)
        {
            parser.next(); // consume end (sst)
            break;
        }
        else
        {
            parser.next(); // skip
        }
    }
}

rich_text xlsx_reader::read_rich_text(xml_parser &parser)
{
    // Called when we're inside an <si> or <r> start element
    rich_text result;
    std::string text_content;
    bool preserve_space = false;

    // Check xml:space attribute
    if (parser.attribute_present("space"))
    {
        preserve_space = (parser.attribute("space") == "preserve");
    }

    int depth = 1;
    while (depth > 0)
    {
        auto ev = parser.next();
        if (ev == xml_parser_event::eof) break;

        if (ev == xml_parser_event::start_element)
        {
            const auto &name = parser.name();

            if (name == "t")
            {
                // Text element - read its content
                std::string text;
                while (true)
                {
                    auto tev = parser.next();
                    if (tev == xml_parser_event::characters)
                        text += parser.value();
                    else if (tev == xml_parser_event::end_element)
                        break;
                    else if (tev == xml_parser_event::eof)
                        break;
                }

                if (!text.empty())
                {
                    rich_text_run run;
                    run.first = text;
                    run.preserve_space = preserve_space;
                    result.add_run(run);
                    text_content += text;
                }
            }
            else if (name == "r")
            {
                // Rich text run - recursive
                rich_text rt = read_rich_text(parser);
                for (const auto &run : rt.runs())
                    result.add_run(run);
            }
            else if (name == "rPr")
            {
                // Run properties - skip
                int rpr_depth = 1;
                while (rpr_depth > 0)
                {
                    auto rev = parser.next();
                    if (rev == xml_parser_event::start_element) ++rpr_depth;
                    else if (rev == xml_parser_event::end_element) --rpr_depth;
                    else if (rev == xml_parser_event::eof) break;
                }
            }
            else if (name == "phoneticPr" || name == "rPh")
            {
                // Skip phonetic elements
                int pdepth = 1;
                while (pdepth > 0)
                {
                    auto pev = parser.next();
                    if (pev == xml_parser_event::start_element) ++pdepth;
                    else if (pev == xml_parser_event::end_element) --pdepth;
                    else if (pev == xml_parser_event::eof) break;
                }
            }
        }
        else if (ev == xml_parser_event::end_element)
        {
            --depth;
        }
    }

    // If no runs were added but we have plain text from a direct <t> child of <si>
    if (result.runs().empty() && !text_content.empty())
    {
        result.add_run(rich_text_run{text_content, {}, preserve_space});
    }

    return result;
}

std::string xlsx_reader::read_text(xml_parser &parser)
{
    std::string text;
    int depth = 1;
    while (depth > 0)
    {
        auto ev = parser.next();
        if (ev == xml_parser_event::characters)
            text += parser.value();
        else if (ev == xml_parser_event::start_element)
            ++depth;
        else if (ev == xml_parser_event::end_element)
            --depth;
        else if (ev == xml_parser_event::eof)
            break;
    }
    return text;
}

// ===== Stylesheet (minimal) =====

void xlsx_reader::read_stylesheet(zip_reader &zip)
{
    if (!zip.has_file("xl/styles.xml")) return;

    // Minimal: just parse enough to not break. We don't need style data for reading values.
    auto xml_str = zip.read("xl/styles.xml");
    std::istringstream iss(xml_str);
    xml_parser parser(iss);

    while (parser.next() != xml_parser_event::eof)
    {
        // Just consume all events
    }
}

// ===== Worksheet parsing =====

void xlsx_reader::read_worksheet(zip_reader &zip, worksheet_impl &ws)
{
    // Build the sheet path from its rId
    // First, read workbook relationships
    auto wb_rels = read_relationships(zip, "xl/_rels/workbook.xml.rels");
    std::string sheet_target;
    for (const auto &rel : wb_rels)
    {
        if (rel.first == ws.sheet_id_)
        {
            sheet_target = resolve_relationship_target("xl/workbook.xml", rel.second);
            break;
        }
    }

    if (sheet_target.empty() || !zip.has_file(sheet_target)) return;

    auto xml_str = zip.read(sheet_target);
    std::istringstream iss(xml_str);
    xml_parser parser(iss);

    // Parse worksheet sections
    bool done = false;
    while (!done)
    {
        auto ev = parser.next();
        if (ev == xml_parser_event::eof) break;

        if (ev == xml_parser_event::start_element)
        {
            const auto &name = parser.name();

            if (name == "sheetPr")
            {
                skip_element(parser); // Skip for now
            }
            else if (name == "dimension")
            {
                // Skip - we calculate from data
            }
            else if (name == "sheetViews")
            {
                skip_element(parser);
            }
            else if (name == "sheetFormatPr")
            {
                skip_element(parser);
            }
            else if (name == "cols")
            {
                skip_element(parser);
            }
            else if (name == "sheetData")
            {
                parse_sheet_data(parser, ws);
            }
            else if (name == "mergeCells")
            {
                // Parse merged cell ranges
                while (parser.peek() != xml_parser_event::eof)
                {
                    auto pe = parser.peek();
                    if (pe == xml_parser_event::start_element && parser.name() == "mergeCell")
                    {
                        parser.next();
                        auto ref = parser.attribute("ref");
                        if (!ref.empty())
                            ws.merged_cells_.push_back(range_reference(ref));
                    }
                    else if (pe == xml_parser_event::end_element)
                    {
                        parser.next();
                        break;
                    }
                    else
                    {
                        parser.next();
                    }
                }
            }
            else if (name == "autoFilter")
            {
                auto ref = parser.attribute("ref");
                if (!ref.empty())
                    ws.auto_filter_ = range_reference(ref);
            }
            else if (name == "pageMargins" || name == "pageSetup" ||
                     name == "headerFooter" || name == "drawing" ||
                     name == "conditionalFormatting" || name == "dataValidations" ||
                     name == "hyperlinks" || name == "phoneticPr" ||
                     name == "printOptions" || name == "rowBreaks" ||
                     name == "colBreaks" || name == "extLst")
            {
                skip_element(parser);
            }
        }
        else if (ev == xml_parser_event::end_element && parser.name() == "worksheet")
        {
            done = true;
        }
    }
}

// ===== Fast sheet data parser =====

namespace {

struct ParsedCell
{
    std::string reference;  // A1-style, e.g. "A1"
    std::string type;       // "s", "n", "b", "e", "inlineStr", "str"
    std::string style;      // style index (number string)
    std::string value;      // cell value as string
};

void parse_cell(xml_parser &parser, std::vector<ParsedCell> &cells)
{
    ParsedCell cell;
    cell.reference = parser.attribute("r");
    cell.type = parser.attribute("t");
    cell.style = parser.attribute("s");

    // Read cell content
    while (true)
    {
        auto ev = parser.next();
        if (ev == xml_parser_event::start_element)
        {
            const auto &name = parser.name();
            if (name == "v" || name == "is" || name == "f")
            {
                // Read text content
                std::string text;
                while (true)
                {
                    auto tev = parser.next();
                    if (tev == xml_parser_event::characters)
                        text += parser.value();
                    else if (tev == xml_parser_event::end_element)
                        break;
                    else if (tev == xml_parser_event::eof)
                        break;
                }

                if (name == "v")
                    cell.value = text;
                else if (name == "is")
                    cell.value = text;
                else if (name == "f")
                    {} // formula - skip, value will be read from <v>
            }
            else
            {
                // Skip unknown element inside cell
                int depth = 1;
                while (depth > 0)
                {
                    auto sev = parser.next();
                    if (sev == xml_parser_event::start_element) ++depth;
                    else if (sev == xml_parser_event::end_element) --depth;
                    else if (sev == xml_parser_event::eof) break;
                }
            }
        }
        else if (ev == xml_parser_event::end_element)
        {
            break; // end of <c>
        }
        else if (ev == xml_parser_event::eof)
        {
            break;
        }
    }

    cells.push_back(std::move(cell));
}

void parse_row(xml_parser &parser, std::vector<ParsedCell> &cells)
{
    while (true)
    {
        auto ev = parser.peek();
        if (ev == xml_parser_event::start_element && parser.name() == "c")
        {
            parser.next(); // consume <c>
            parse_cell(parser, cells);
        }
        else if (ev == xml_parser_event::end_element)
        {
            parser.next(); // consume </row>
            break;
        }
        else if (ev == xml_parser_event::eof)
        {
            break;
        }
        else
        {
            parser.next(); // skip unknown
        }
    }
}

} // anonymous namespace

void xlsx_reader::parse_sheet_data(xml_parser &parser, worksheet_impl &ws)
{
    // We're inside <sheetData>
    std::vector<ParsedCell> all_cells;

    while (true)
    {
        auto ev = parser.peek();
        if (ev == xml_parser_event::start_element && parser.name() == "row")
        {
            parser.next(); // consume <row>
            parse_row(parser, all_cells);
        }
        else if (ev == xml_parser_event::end_element)
        {
            parser.next(); // consume </sheetData>
            break;
        }
        else if (ev == xml_parser_event::eof)
        {
            break;
        }
        else
        {
            parser.next();
        }
    }

    // Now populate the worksheet cell_map from parsed cells
    auto &wb = *ws.parent_;

    for (auto &pc : all_cells)
    {
        if (pc.reference.empty()) continue;

        cell_reference ref(pc.reference);
        cell_impl ci;
        ci.parent_ = &ws;
        ci.column_ = ref.column_index();
        ci.row_ = ref.row();

        cell_type ct = type_from_string(pc.type);

        if (pc.value.empty() && ct != cell_type::shared_string)
        {
            ci.type_ = cell_type::empty;
        }
        else
        {
            ci.type_ = ct;

            switch (ct)
            {
            case cell_type::shared_string:
            {
                std::size_t idx = 0;
                if (safe_stoull(pc.value, idx))
                {
                    auto it = wb.shared_strings_values_.find(idx);
                    if (it != wb.shared_strings_values_.end())
                    {
                        ci.value_text_ = it->second;
                        ci.value_numeric_ = static_cast<double>(idx);
                    }
                }
                break;
            }
            case cell_type::number:
            {
                double num = 0.0;
                if (safe_stod(pc.value, num))
                    ci.value_numeric_ = num;
                break;
            }
            case cell_type::boolean:
                ci.value_numeric_ = (pc.value == "1" || pc.value == "true") ? 1.0 : 0.0;
                break;
            case cell_type::error:
                ci.value_text_ = rich_text(pc.value);
                break;
            case cell_type::inline_string:
            case cell_type::formula_string:
                ci.value_text_ = rich_text(pc.value);
                if (ci.value_text_.runs().empty())
                    ci.value_text_.add_run(rich_text_run{pc.value});
                break;
            default:
                break;
            }
        }

        if (!pc.style.empty())
        {
            std::size_t style_idx = 0;
            if (safe_stoull(pc.style, style_idx))
                ci.format_index_ = style_idx;
        }

        ws.cell_map_[ref] = std::move(ci);
    }
}

// ===== Main orchestration =====

void xlsx_reader::populate_workbook(zip_reader &zip)
{
    // Read content types
    read_content_types(zip);

    // Read root relationships to find workbook path
    auto root_rels = read_relationships(zip, "_rels/.rels");
    std::string workbook_target = "xl/workbook.xml"; // default

    // Read workbook relationships
    auto wb_rels = read_relationships(zip, "xl/_rels/workbook.xml.rels");

    // Map rIds to targets
    std::unordered_map<std::string, std::string> rel_targets;
    for (const auto &rel : wb_rels)
        rel_targets[rel.first] = rel.second;

    // Read shared strings
    read_shared_string_table(zip);

    // Read styles (minimal)
    read_stylesheet(zip);

    // Read workbook XML to get sheet info
    read_office_document(zip);

    // Read each worksheet
    std::unordered_map<std::string, std::string> rid_to_target;
    for (const auto &rel : wb_rels)
        rid_to_target[rel.first] = resolve_relationship_target("xl/workbook.xml", rel.second);

    for (auto &ws : wb_.worksheets_)
    {
        read_worksheet(zip, ws);
    }
}

} // namespace detail
} // namespace xlntx

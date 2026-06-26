// Comprehensive xlntx test — reads all cells from a spreadsheet
#include "xlntx/xlntx.hpp"

#include <iostream>
#include <iomanip>
#include <string>

#include "fmt/format.h"
#include "fmt/core.h"

using namespace xlntx;

void print_separator(char ch = '=') { std::cout << std::string(60, ch) << '\n'; }

void dump_all_cells(const std::string &filename)
{
    std::cout << "=== Reading: " << filename << " ===\n";

    xlntx::workbook wb;
    try
    {
        wb.load(filename);
    }
    catch (const std::exception &ex)
    {
        fmt::print(stderr, "  LOAD ERROR: {}\n", ex.what());
        return;
    }

    fmt::print("  Sheet count: {}\n", wb.sheet_count());
    for (const auto &title : wb.sheet_titles())
        fmt::print("    Sheet: \"{}\"\n", title);

    auto ws = wb.sheet_by_index(0);
    fmt::print("  Title: \"{}\"\n", ws.title());
    auto dim = ws.calculate_dimension();
    fmt::print("  Dimension: {}\n", dim.to_string());

    row_t lo = ws.lowest_row();
    row_t hi = ws.highest_row();
    column_t lc = ws.lowest_column();
    column_t hc = ws.highest_column();
    fmt::print("  Range: {}:{} to {}:{}\n", lc.column_string(), lo, hc.column_string(), hi);

    // Dump ALL cells with their types
    for (row_t r = lo; r <= hi; ++r)
    {
        fmt::print("  Row {}: ", r);
        for (column_t::index_t c = lc.index; c <= hc.index; ++c)
        {
            try
            {
                auto cell = ws.cell(column_t(c), r);
                if (cell.has_value())
                {
                    auto dt = cell.data_type();
                    std::string type_name = "?";
                    switch (dt)
                    {
                    case xlntx::cell_type::empty: type_name = "E"; break;
                    case xlntx::cell_type::boolean: type_name = "B"; break;
                    case xlntx::cell_type::number: type_name = "N"; break;
                    case xlntx::cell_type::shared_string: type_name = "S"; break;
                    case xlntx::cell_type::error: type_name = "Err"; break;
                    case xlntx::cell_type::inline_string: type_name = "I"; break;
                    case xlntx::cell_type::formula_string: type_name = "F"; break;
                    case xlntx::cell_type::date: type_name = "D"; break;
                    }
                    std::string val = cell.to_string();
                    if (val.length() > 40) val = val.substr(0, 40) + "...";
                    fmt::print("[{}{}:{}] ", type_name, cell.column().column_string(), val);
                }
            }
            catch (const std::exception &ex)
            {
                fmt::print(stderr, "\n  CELL ERROR at {}:{}: {}\n",
                           column_t(c).column_string(), r, ex.what());
            }
        }
        std::cout << "\n";
    }
}

void test_rows_iteration(const std::string &filename)
{
    std::cout << "\n=== Testing rows(false) iteration: " << filename << " ===\n";
    xlntx::workbook wb;
    wb.load(filename);
    auto ws = wb.sheet_by_index(0);

    int row_count = 0;
    for (auto &row : ws.rows(false))
    {
        ++row_count;
        auto front_cell = row.front();
        row_t r = front_cell.row();
        auto len = row.length();
        fmt::print("  Row {} (r={}, len={}): ", row_count, r, len);

        try
        {
            for (std::size_t i = 0; i < len && i < 10; ++i)
            {
                auto cell = row[i];
                if (cell.has_value())
                {
                    std::string val = cell.to_string();
                    if (val.length() > 30) val = val.substr(0, 30) + "...";
                    fmt::print("[c{}:{}] ", i, val);
                }
            }
        }
        catch (const std::exception &ex)
        {
            fmt::print(stderr, "  ROW ITER ERROR at row {}: {}", r, ex.what());
        }
        std::cout << "\n";

        if (row_count > 20) { fmt::print("  ... (truncated)\n"); break; }
    }
    fmt::print("  Total rows iterated: {}\n", row_count);
}

void test_range_iteration(const std::string &filename)
{
    std::cout << "\n=== Testing range iteration: " << filename << " ===\n";
    xlntx::workbook wb;
    wb.load(filename);
    auto ws = wb.sheet_by_index(0);

    auto dim = ws.calculate_dimension();
    std::string ref = dim.to_string();
    fmt::print("  Range string: {}\n", ref);

    try
    {
        auto rng = ws.range(ref);
        fmt::print("  Range length: {}\n", rng.length());
        for (std::size_t i = 0; i < rng.length(); ++i)
        {
            auto cv = rng[i];
            fmt::print("  Vector[{}] row={} len={}: ", i, cv.row(), cv.length());
            for (std::size_t j = 0; j < cv.length() && j < 5; ++j)
            {
                try
                {
                    auto c = cv[j];
                    if (c.has_value())
                        fmt::print("[{}] ", c.to_string());
                }
                catch (const std::exception &ex)
                {
                    fmt::print(stderr, "[ERR:{}] ", ex.what());
                }
            }
            std::cout << "\n";
        }
    }
    catch (const std::exception &ex)
    {
        fmt::print(stderr, "  RANGE ERROR: {}\n", ex.what());
    }
}

int main(int argc, char **argv)
{
    std::string filename = "UserLibsData.xlsx";
    if (argc > 1) filename = argv[1];

    print_separator();
    dump_all_cells(filename);
    print_separator('-');
    test_rows_iteration(filename);
    print_separator('-');
    test_range_iteration(filename);
    print_separator();
    std::cout << "Tests complete.\n";
    return 0;
}

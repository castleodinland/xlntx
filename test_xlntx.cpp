// xlnt vs xlntx A/B 对比测试 — 同一 xlsx 文件分别用两个库加载，逐行对比数据
#include "xlntx/xlntx.hpp"
#include "xlnt/xlnt.hpp"

#include <iostream>
#include <iomanip>
#include <string>

#include "fmt/format.h"
#include "fmt/core.h"

const int param_start_row = 5;
const int param_name_column = 1;

void print_separator(char ch = '=') { std::cout << std::string(60, ch) << '\n'; }

int main(int argc, char **argv)
{
    // std::string filename = "UserLibsData.xlsx";
    std::string filename = "params_msg_mapping_error_office.xlsx";
    if (argc > 1) filename = argv[1];

    std::cout << "A/B comparison test — reading: " << filename << '\n';
    print_separator();

    // ===== xlnt (reference) =====
    std::cout << "\n[xlnt] Loading...\n";
    try
    {
        xlnt::workbook wb_xlnt;
        wb_xlnt.load(filename);
        std::cout << "  Sheet count: " << wb_xlnt.sheet_count() << '\n';
        std::cout << "  Sheet titles:\n";
        for (const auto &title : wb_xlnt.sheet_titles())
            std::cout << "    - " << title << '\n';

        auto ws_xlnt = wb_xlnt.sheet_by_index(0);
        std::cout << "  Sheet[0] title: \"" << ws_xlnt.title() << "\"\n";
        std::cout << "  Dimension: " << ws_xlnt.calculate_dimension().to_string() << '\n';
        std::cout << "  Lowest row: " << ws_xlnt.lowest_row() << ", Highest row: " << ws_xlnt.highest_row() << '\n';

        std::cout << "\n  [xlnt] Rows (first 10):\n";
        int row_count = 0;

        for (auto &row : ws_xlnt.rows(false))
        {
            if (row.front().row() >= param_start_row)
            {
                xlnt::cell cell = row[param_name_column];

                if (cell.to_string() == "")
                {
                    break;
                }
                fmt::print("param: {}\n", cell.to_string());
            }
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "  xlnt ERROR: " << ex.what() << '\n';
    }

    print_separator('-');

    // ===== xlntx (our library) =====
    std::cout << "\n[xlntx] Loading...\n";
    try
    {
        xlntx::workbook wb;
        wb.load(filename);
        std::cout << "  Sheet count: " << wb.sheet_count() << '\n';
        std::cout << "  Sheet titles:\n";
        for (const auto &title : wb.sheet_titles())
            std::cout << "    - " << title << '\n';

        auto ws = wb.sheet_by_index(0);
        std::cout << "  Sheet[0] title: \"" << ws.title() << "\"\n";
        std::cout << "  Dimension: " << ws.calculate_dimension().to_string() << '\n';
        std::cout << "  Lowest row: " << ws.lowest_row() << ", Highest row: " << ws.highest_row() << '\n';

        std::cout << "\n  [xlntx] Rows (first 10):\n";
        int row_count = 0;
        for (auto &row : ws.rows(false))
        {
            if (row.front().row() >= param_start_row)
            {
                xlntx::cell cell = row[param_name_column];

                if (cell.to_string() == "")
                {
                    break;
                }
                fmt::print("param: {}\n", cell.to_string());
            }
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "  xlntx ERROR: " << ex.what() << '\n';
    }

    print_separator();
    std::cout << "Comparison done.\n";
    return 0;

}

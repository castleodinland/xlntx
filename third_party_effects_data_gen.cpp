// third_party_effects_data_gen (non-VS variant) — 基于 Excel 配置表生成游戏特效数据
#include <iostream>
#include "fmt/format.h"
#include "fmt/core.h"

#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>

#include "xlntx/xlntx.hpp"

#include <unordered_map>


#include "EffectInfo.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

const int param_start_row = 5;
const int param_name_column = 1;
const int param_type_column = 2;
const int param_content_column = 3;
const int param_default_column = 4;
const int param_param_config_column = 5;
const int param_skip_column = 6;
const int param_ui_type_column = 7;
const int param_unit_content_column = 8;
const int param_fract_column = 9;
const int param_ratio_column = 10;
const int param_tips_column = 11;
const int param_check_column = 12;
const int param_ui_row_column = 13;
const int param_ui_column_column = 14;

std::string xls_file = "UserLibsData.xlsx";
std::string work_folder = ".";
std::string check_bin_full_path = "";
bool is_auto = false;

std::vector<std::string> name_list;

std::string libs_info_str = "";
std::string libs_info_str_sub = "";
std::string api_info_str_2 = "";
std::string api_info_str_2_1 = "";
std::string engine_config_str_1 = "";
std::string engine_config_str_3 = "";
std::string engine_config_str_4 = "\n//effects parameters length\n";
std::string output_folder = "";
std::string robo_ver = "";
std::string macro_main = "";
std::string split_effect_name = "";
std::string macro_onoff_str = "";
std::string interface_template_str = "";

std::string params_ver_helper_str = ""; // help user to write the header of parameter array

std::vector<unsigned char> add_bin_data;

std::map<std::string, int> method_map = {
    {"METHOD_NONE", 0x00},
    {"METHOD_INIT", 0x01},
    {"METHOD_CFG_1", 0x02},
    {"METHOD_CFG_2", 0x04},
    {"METHOD_CFG_3", 0x08},
    {"METHOD_CFG_FADEOI", 0x10},
    {"METHOD_CFG_STEP", 0x20},
};

EffectInfo *main_info_data_ptr = nullptr;

bool hasExactlyOneComma(const std::string &str)
{
    return std::count(str.begin(), str.end(), ',') == 1;
}

unsigned char third_party_num = 0;

std::string to_absolute(const std::string &path_str)
{
    fs::path p(path_str);

    if (p.is_absolute())
    {
        return path_str;
    }

    // fs::path abs_p = fs::absolute(p);
    fs::path abs_p = fs::canonical(p);
    return abs_p.string();
}

std::string join_paths(std::initializer_list<std::string> paths)
{
    fs::path result;

    for (const auto &path : paths)
    {
        if (result.empty())
        { // first item
            result = path;
        }
        else
        {
            result /= path;
        }
    }

    return result.string();
}

std::string cell_to_string(xlntx::cell cell)
{
    std::string out_string;
    if (cell.has_value())
        out_string = "";
    else
        out_string = cell.to_string();
    return out_string;
}

int press_any_key(void)
{
    if (!is_auto)
    {
        fmt::print("Press any key to continue...\n");
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return -1;
    }
    return 0;
}

struct FzParseResult
{
    std::string type;
    std::string array;
    int length;
    std::string error;
};

FzParseResult parse_frame_size_type(const std::string &input_str, int max_gears = 16)
{
    FzParseResult result = {"", "", 0, ""};

    // 去除首尾空白
    std::string trimmed = input_str;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

    // 类型1：FZ_ANY
    if (trimmed == "FZ_ANY")
    {
        result.type = "FZ_ANY";
        result.array = "{0}";
        result.length = 0;
        return result;
    }

    // 类型2：{128,256,512} �? {512}，支持任意空�?
    boost::regex gear_pattern(R"(^\{\s*\d+\s*(,\s*\d+\s*)*\}$)");
    boost::smatch match_results;
    if (boost::regex_match(trimmed, match_results, gear_pattern))
    {
        // 提取数字
        boost::regex number_pattern(R"(\d+)");
        boost::sregex_iterator it(trimmed.begin(), trimmed.end(), number_pattern);
        boost::sregex_iterator end;
        std::vector<std::string> numbers;
        while (it != end)
        {
            numbers.push_back(it->str());
            ++it;
        }
        if (numbers.size() > static_cast<size_t>(max_gears))
        {
            std::stringstream ss;
            ss << "Too many gears: " << numbers.size() << " exceeds maximum " << max_gears;
            result.error = ss.str();
            return result;
        }
        result.type = "FZ_GEAR";
        result.array = trimmed;
        result.length = static_cast<int>(numbers.size());
        return result;
    }

    // 类型3：[1:256]，支持任意空�?
    boost::regex range_pattern(R"(^\[\s*\d+\s*:\s*\d+\s*\]$)");
    boost::smatch range_match;
    if (boost::regex_match(trimmed, range_match, range_pattern))
    {
        // 提取数字
        boost::regex number_pattern(R"(\d+)");
        boost::sregex_iterator it(trimmed.begin(), trimmed.end(), number_pattern);
        boost::sregex_iterator end;
        std::vector<std::string> numbers;
        while (it != end)
        {
            numbers.push_back(it->str());
            ++it;
        }
        if (numbers.size() == 2)
        {
            result.type = "FZ_RANGE";
            result.array = "{" + numbers[0] + "," + numbers[1] + "}";
            result.length = 2;
            return result;
        }
        else
        {
            result.error = "Range must contain exactly two numbers";
            return result;
        }
    }

    // 无效输入
    result.type = "FZ_ERROR";
    result.array = "{0}";
    result.error = "Invalid input format";
    return result;
}

static std::string GET_INTERFACE_TEMPLATE(std::string name)
{
    return fmt::format(R"(
//{} interface
bool roboeffect_{}_init_if(void *node);
bool roboeffect_{}_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);
bool roboeffect_{}_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
int32_t roboeffect_{}_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

)",
                       name, name, name, name, name);
}

static std::string GET_INTERFACE_TEMPLATE_CODES(std::string name)
{
    return fmt::format(R"(


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "roboeffect_api.h"
/**
 * {} interface codes
*/
bool roboeffect_{}_init_if(void *node)
{{
	uint8_t *context_ptr;
	roboeffect_user_defined_effect_info info;
	void *context_ptr;

	/**
	 * get information struct from instance
	*/
	roboeffect_user_defined_get_info(node, &info);

	/**
	 * get user defined struct context
	*/
	context_ptr = info.context_memory;

	/**
	 * Can get info.scratch_memory if used;
	*/

	/*
	* Todo: Init user defined algorithm.
	*/


	return TRUE;
}}

bool roboeffect_{}_config_if(void *node, int16_t *new_param, uint8_t param_num, uint8_t len)
{{
	int ret;
	uint8_t method_flag = 0;
	roboeffect_user_defined_effect_info info;
	void *context_ptr;

	/**
	 * check parameters and update to effect instance
	*/
	if(ROBOEFFECT_ERROR_OK > roboeffect_user_defined_params_check(node, new_param, param_num, len, &method_flag))
	{{
		return FALSE;
	}}

	/**
	 * get information struct from instance
	*/
	roboeffect_user_defined_get_info(node, &info);

	/**
	 * get user defined struct context
	*/
	context_ptr = info.context_memory;

	if((method_flag & METHOD_CFG_1) && info.is_active)
	{{
		//call config APIs if this parameter use METHOD_CFG_1 flag.
	}}

	return TRUE;
}}

bool roboeffect_{}_apply_if(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n)
{{
	int i, s;
	int32_t pregain, *pcm_in_24 = (int32_t*)pcm_in1, *pcm_out_24 = (int32_t*)pcm_out;
	roboeffect_user_defined_effect_info info;
	void *context_ptr;

	/**
	 * get information struct from instance
	*/
	roboeffect_user_defined_get_info(node, &info);

	/**
	 * get user defined struct context
	*/
	context_ptr = info.context_memory;
	
	/*
	* Todo: Apply user defined algorithm.
	*/
	

	return TRUE;
}}

bool roboeffect_{}_memory_size_if(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response)
{{
	//return real size of context memory
	response->context_memory_size = 4;

	//return additional memory size
	response->additional_memory_size = 0;

	//return scratch memory size
	response->scratch_memory_size = 0;

	return TRUE;
}}

)",
                       name, name, name, name, name);
}

static std::string GET_USER_DEFINED_API_H_TEMPLATE()
{
    return fmt::format(R"(
/**
 **************************************************************************************
 * @file    user_defined_effect_api.h
 * @brief   interface for user defined effect algorithm
 *          Automatically generated by the script, no manual modification required.
 *
 * @author  Castle Cai
 * @version V2.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include "roboeffect_api.h"
#include "roboeffect_config.h"



)");
}

int bits_width_convert(const std::string &str)
{
    if (str == "16")
    {
        return 0;
    }
    else if (str == "24")
    {
        return 1;
    }
    else if (str == "16/24" || str == "24/16")
    {
        return 2;
    }
    else
    {
        return 0;
    }
}

int channel_convert(const std::string &str)
{
    if (str == "N/A")
    {
        return 0;
    }
    else if (str == "mono")
    {
        return 1;
    }
    else if (str == "stereo")
    {
        return 2;
    }
    else if (str == "mono/stereo" || str == "stereo/mono")
    {
        return 3;
    }
    else
    {
        return 0;
    }
}

std::string channel_convert_2(const std::string &str)
{
    if (str == "N/A")
    {
        return "NA";
    }
    else if (str == "mono")
    {
        return "ROBOEFFECT_CH_MONO";
    }
    else if (str == "stereo")
    {
        return "ROBOEFFECT_CH_STEREO";
    }
    else if (str == "mono/stereo" || str == "stereo/mono")
    {
        return "ROBOEFFECT_CH_MONO_STEREO";
    }
    else
    {
        return "NA";
    }
}

int param_type_convert(const std::string &str)
{
    if (str == "bool")
    {
        return 0;
    }
    else if (str == "enum")
    {
        return 1;
    }
    else if (str == "value")
    {
        return 2;
    }
    else if (str == "display")
    {
        return 3;
    }
    else if (str == "sub-effect")
    {
        return 4;
    }
    else if (str == "vector")
    {
        return 5;
    }
    else
    {
        return 2;
    }
}

int vector_type_convert(const std::string &str)
{
    if (str == "int8_t")
    {
        return 0;
    }
    else if (str == "uint8_t")
    {
        return 1;
    }
    else if (str == "int16_t")
    {
        return 2;
    }
    else if (str == "uint16_t")
    {
        return 3;
    }
    else if (str == "int32_t")
    {
        return 4;
    }
    else if (str == "uint32_t")
    {
        return 5;
    }
    else if (str == "int64_t")
    {
        return 6;
    }
    else if (str == "uint64_t")
    {
        return 7;
    }
    else if (str == "float")
    {
        return 8;
    }
    else if (str == "double")
    {
        return 9;
    }
    else
    {
        return 0;
    }
}

int ui_display_type_convert(const std::string &type, const std::string &str)
{
    if (type == "bool")
    {
        if (str == "check_box")
        {
            return 0;
        }
        else if (str == "push_button")
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else if (type == "enum")
    {
        if (str == "comb_box")
        {
            return 0;
        }
        else
        {
            return 0;
        }
    }
    else if (type == "value")
    {
        if (str == "spinbox")
        {
            return 0;
        }
        else if (str == "hslider")
        {
            return 1;
        }
        else if (str == "vsilder")
        {
            return 2;
        }
        else if (str == "dial")
        {
            return 3;
        }
        else
        {
            return 0;
        }
    }
    else if (type == "display")
    {
        if (str == "label")
        {
            return 0;
        }
        else if (str == "progress_bar")
        {
            return 1;
        }
        else if (str == "on_off")
        {
            return 2;
        }
        else
        {
            return 0;
        }
    }
    else if (type == "sub-effect")
    {
        if (str == "push_button")
        {
            return 0;
        }
        else
        {
            return 0;
        }
    }
    else if (type == "vector")
    {
        if (str == "push_button")
        {
            return 0;
        }
        else
        {
            return 0;
        }
    }
    return 0; // 默�?�返回�?
}

int ui_layout_convert(const std::string &str)
{
    if (str == "auto")
    {
        return 0;
    }
    else if (str == "manual")
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

std::string aligned_macro_to_string(const std::string &str, int en)
{
    std::stringstream output;
    output << str << std::setw(35 - str.length()) << ' ' << '(' << en << ')';
    return output.str();
}

std::string aligned_param_to_string(const std::string &str)
{
    std::stringstream output;
    output << str << ',' << std::setw(6 - str.length()) << ' ';
    return output.str();
}

std::string aligned_config_to_string(const std::string &str)
{
    std::stringstream output;
    output << str << ',' << std::setw(14 - str.length()) << ' ';
    return output.str();
}

std::string aligned_param_len_to_string(const std::string &str)
{
    std::stringstream output;
    output << "ROBOEFFECT_" << str << "_PARAM_LEN " << std::setw(24 - str.length()) << ' ';
    return output.str();
}

std::string toUpperCase(const std::string &input)
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c)
                   { return std::toupper(c); });

    return result;
}

std::vector<int> split_content(const std::string &content, char delimiter)
{
    std::vector<int> value_check;
    std::istringstream iss(content);
    std::string token;

    while (std::getline(iss, token, delimiter))
    {
        value_check.push_back(std::stoi(token));
    }

    return value_check;
}

std::vector<std::string> split_content_str(const std::string &content, char delimiter)
{
    std::vector<std::string> value_check;
    std::istringstream iss(content);
    std::string token;

    while (std::getline(iss, token, delimiter))
    {
        value_check.push_back(token);
    }

    return value_check;
}

// if last char is ch, delete it
std::string del_last_ch(const std::string &str, char ch)
{
    size_t index = str.rfind(ch);

    if (index != std::string::npos)
    {
        return str.substr(0, index) + str.substr(index + 1);
    }
    else
    {
        return str;
    }
}

int count_bytes_in_string(const std::string &str)
{
    int bytesNum = 0;
    boost::regex reHexAByte("(0x[0-9a-fA-F][0-9a-fA-F])");

    auto begin = boost::sregex_iterator(str.begin(), str.end(), reHexAByte);
    auto end = boost::sregex_iterator();

    for (auto it = begin; it != end; ++it)
    {
        bytesNum++;
    }

    return bytesNum;
}

// string "0x12, 0x34, 0xAA" --> vector<unsigned char> 0x12, 0x34, 0xAA
void save_bytes_in_string_to_bytes_data(const std::string &str, std::vector<unsigned char> &bytesData)
{
    boost::regex reHexAByte("(0x[0-9a-fA-F][0-9a-fA-F])");

    auto begin = boost::sregex_iterator(str.begin(), str.end(), reHexAByte);
    auto end = boost::sregex_iterator();

    // 遍历匹配�?
    for (auto it = begin; it != end; ++it)
    {
        boost::smatch match = *it;
        std::string byteStr = match[0]; // 获取匹配到的整个字�?�串

        // 将字节字符串�?�?为整数并追加�? bytesData
        bytesData.push_back(static_cast<unsigned char>(std::stoi(byteStr, nullptr, 16)));
    }
}

// to check value string  is or is not end with ending string
bool ends_with(const std::string &value, const std::string &ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

/***************************************make CRC32*********************************************************/
/* * 标准 IEEE 802.3 CRC32 多项�? (反转形式)
 * 对应多项�?: x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
 */
#define POLY 0xEDB88320

/* 全局 CRC �? */
uint32_t crc32_table[256];

/* 初�?�化 CRC �? */
void make_crc_table(void)
{
    uint32_t c;
    int n, k;

    for (n = 0; n < 256; n++)
    {
        c = (uint32_t)n;
        for (k = 0; k < 8; k++)
        {
            if (c & 1)
            {
                c = POLY ^ (c >> 1);
            }
            else
            {
                c = c >> 1;
            }
        }
        crc32_table[n] = c;
    }
}

/* * 计算 CRC32
 * buf: 数据指针
 * len: 数据长度
 */
uint32_t crc32(const unsigned char *buf, size_t len)
{
    uint32_t crc = 0xFFFFFFFF; // 初�?��?
    // printf("cc %d: %02X, %02X, %02X, %02X\n", len, buf[0], buf[1], buf[2], buf[3]);

    for (size_t i = 0; i < len; i++)
    {

        /* (crc ^ data) & 0xFF 得到表索引，然后右移并异或表�? */
        crc = crc32_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF; // 结果异或反转
}

std::string update_file_checksum_name(const fs::path &filePath, uint32_t newChecksum, bool has_3rd)
{
    std::string filename = filePath.filename().string();

    // 构建新的十六进制字�?�串，例�?: 0x83c0b1d
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << newChecksum;
    std::string newCsumStr = ss.str();

    // 使用正则表达式匹�? _0x 后面跟着 8 位十�?进制数的模式
    // 这样�?以兼�? roboeffect_library_info_v2.41.0_0x09de9122_3rdparty.bin 这�?��?�杂格式
    std::string newFilename;
    boost::regex expr(R"(^(roboeffect_library_info)(?:_(v\d+\.\d+\.\d+))?)");
    boost::smatch what;
    std::string prefix_name, version_num;

    if (boost::regex_search(filename, what, expr)) {
        // what[0] 存放的是整个正则表达式匹配到的完整字符串
        prefix_name = what[1].str();
        if(what[2].matched)
            version_num = what[2].str();
    } else {
        fmt::print("Cannot get standrd info.");
    }

    if(has_3rd)
    {   
        if(what[2].matched)
            newFilename = prefix_name + "_" + version_num + "_"+ newCsumStr + "_3rdparty.bin";
        else
            newFilename = prefix_name + "_" + newCsumStr + "_3rdparty.bin";
    }
    else
    {
        if(what[2].matched)
            newFilename = prefix_name + "_" + version_num + ".bin";
        else
            newFilename = prefix_name + ".bin";
    }

    fs::path newPath = filePath.parent_path() / newFilename;

    if (filename != newFilename)
    {
        try
        {
            // if (fs::exists(newPath))
            // {
            //     fs::remove(newPath); // 如果�?标已存在，先删除
            // }
            fs::rename(filePath, newPath);
            return newPath.string();
        }
        catch (const fs::filesystem_error &e)
        {
            std::cerr << "Rename error: " << e.what() << std::endl;
        }
    }

    return filePath.string();
}

/***************************************make CRC32*********************************************************/

int main(int argc, char **argv)
{
    fmt::print("user effect libs data generator by excel, ver=5.2, table_ver=0.5.0\n");
    fmt::print("requires roboeffect version: 2.36.x or later \n");

    /*check command line parameters*/
    try
    {
        po::options_description desc("Options");
        desc.add_options()("help,h", "show help informations")("process,p", po::value<std::string>(), "processing folder")("check,c", po::value<std::string>(), "check bin file");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return 0;
        }

        if (vm.count("process"))
        {
            work_folder = vm["process"].as<std::string>();
            is_auto = true;
            // fmt::print("inifile set to {}.\n", ini_config_file);
        }
        else
        {
            // fmt::print("inifile not set.\n");
        }

        if (vm.count("check"))
        {
            check_bin_full_path = vm["check"].as<std::string>();
            try
            {
                main_info_data_ptr = new EffectInfo(check_bin_full_path);
            }
            catch (const std::exception &ex)
            {
                fmt::print(stderr, "Error: {}\n", ex.what());
                return 1;
            }
            fmt::print("buildin effect: {}\n", main_info_data_ptr->buildin_cnt);
            fmt::print("3rd party effect: {}\n", main_info_data_ptr->third_party_cnt);
            fmt::print("list 3rd party: \n");

            for (int i = main_info_data_ptr->buildin_cnt; i < main_info_data_ptr->buildin_cnt + main_info_data_ptr->third_party_cnt; i++)
            {
                // fmt::print("cnt: {}\n", i);
                const EffectInfo::Item *item_one = main_info_data_ptr->getOneItem(i);
                if (item_one)
                {
                    fmt::print("name={},  ver={}.{}.{}\n", item_one->name, item_one->version[0], item_one->version[1], item_one->version[2]);
                }
            }

            return 0;
        }
    }
    catch (const po::error &ex)
    {
        fmt::print(stderr, "Error: {}\n", ex.what());
        return 1;
    }

    /*first check excel file*/
    try
    {
        for (auto &entry : fs::directory_iterator(work_folder))
        {
            if (fs::is_directory(entry.path()))
            {
                std::string excel_full_path = join_paths({to_absolute(entry.path().string()), "UserLibsData.xlsx"});
                if (!fs::exists(excel_full_path))
                {
                    fmt::print("found folder {}, but UserLibsData.xlsx not existed.\n", entry.path().string());
                    press_any_key();
                }
            }
        }
    }
    catch (const fs::filesystem_error &ex)
    {
        std::cerr << "filesystem err: " << ex.what() << std::endl;
        press_any_key();
    }

    /*second check effect bin file*/
    bool found_info_bin = false;
    for (const auto &entry : fs::directory_iterator(work_folder))
    {
        if (fs::is_regular_file(entry.path()) && entry.path().filename().string().rfind("roboeffect_library_info", 0) == 0)
        {
            //判断info.bin�?否含有�??三方音效，不能只从名字判�?
            std::ifstream file(entry.path().string(), std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "Error opening file: " << entry.path() << std::endl;
                return 1;
            }

            std::vector<unsigned char> fileContent(std::istreambuf_iterator<char>(file), {});
            file.close();

            if(fileContent[10] == 0)
            {
                found_info_bin = true;
                break;
            }
        }
    }

    if (!found_info_bin)
    {
        fmt::print(stderr, "\n==============================>\n");
        fmt::print(stderr, "Error: Please place at least one roboeffect_library_info.bin with standrd version in the same directory level.\n");
        fmt::print(stderr, "<==============================\n\n");
        press_any_key();
        return 1;
    }

    try
    {
        for (auto &entry : fs::directory_iterator(work_folder))
        {
            if (fs::is_directory(entry.path()))
            {
                // fmt::print("found a folder: {}\n", entry.path().string());
                std::string excel_full_path = join_paths({to_absolute(entry.path().string()), "UserLibsData.xlsx"});

                xlntx::workbook wb;
                wb.load(excel_full_path);
                // fmt::print("1load excel: {}\n", excel_full_path);
                xlntx::worksheet ws = wb.sheet_by_index(0);
                // fmt::print("2load excel: {}\n", excel_full_path);

                std::string effect_name = ws.title();
                split_effect_name = effect_name;
                name_list.push_back(effect_name);
                // fmt::print("\n3rd party name: {}\n", effect_name);

                libs_info_str.clear();
                api_info_str_2.clear();
                api_info_str_2_1.clear();
                engine_config_str_3.clear();

                std::string effect_bitwidth = ws.cell("C2").to_string();
                std::string effect_version = ws.cell("B2").to_string();
                std::string effect_in1_ch = ws.cell("D2").to_string();
                std::string effect_in2_ch = ws.cell("E2").to_string();
                std::string effect_out_ch = ws.cell("F2").to_string();
                std::string effect_input_discript = ws.cell("N1").to_string();

                std::string effect_frame_size = ws.cell("H2").to_string();
                int effect_cnx_name = 4;
                std::string effect_ui_layout_mode = ws.cell("Q3").to_string();
                std::string effect_ui_grid_size = ws.cell("R3").to_string();
                int effect_param_number = 0;
                std::string effect_is_node = ws.cell("G2").to_string();

                macro_onoff_str += fmt::format("#ifndef {}\n", toUpperCase(effect_name + "_ENABLE"));
                macro_onoff_str += fmt::format("#define {}\n", aligned_macro_to_string(toUpperCase(effect_name + "_ENABLE"), 1));
                macro_onoff_str += fmt::format("#endif\n");

                // std::vector<std::unordered_map<std::string, std::string>> param_dict;
                std::vector<std::map<std::string, std::string>> param_dict;

                std::string default_params_str;

                for (auto &row : ws.rows(false))
                {
                    if (row.front().row() >= param_start_row)
                    {
                        xlntx::cell cell = row[param_name_column];

                        if (cell.to_string() == "")
                        {
                            break;
                        }
                        // fmt::print("param: {}\n", cell.to_string());
                        // fmt::print("has value: {}\n", cell_test.has_value());

                        /*parameters operations*/
                        std::map<std::string, std::string> unit;

                        unit["name"] = row[param_name_column].to_string();
                        unit["type"] = row[param_type_column].to_string();
                        unit["content"] = row[param_content_column].to_string();
                        unit["default"] = row[param_default_column].to_string();
                        unit["param_config"] = row[param_param_config_column].to_string();
                        unit["skip"] = row[param_skip_column].to_string();
                        unit["ui_type"] = row[param_ui_type_column].to_string();
                        unit["unit_content"] = row[param_unit_content_column].to_string();
                        unit["fract"] = row[param_fract_column].to_string();
                        unit["ratio"] = row[param_ratio_column].to_string();
                        unit["tips"] = row[param_tips_column].to_string();
                        unit["check"] = row[param_check_column].to_string();
                        unit["Row"] = row[param_ui_row_column].to_string();
                        unit["Column"] = row[param_ui_column_column].to_string();

                        effect_param_number++;

                        param_dict.push_back(unit);
                    }
                }

                for (const auto &param_map : param_dict)
                {
                    // std::cout << std::endl;
                    for (const auto &kvp : param_map)
                    {
                        // std::cout << "Key: " << kvp.first << ", Value: " << kvp.second << std::endl;
                    }
                }
                libs_info_str_sub.clear();
                api_info_str_2 += "\t{";
                api_info_str_2_1 += "\t{";

                engine_config_str_3 += GET_INTERFACE_TEMPLATE(effect_name);

                fmt::print("item: {}, ver={}, bitwidth={}, number={}\n", effect_name, effect_version, effect_bitwidth, effect_param_number);

                // effect version
                std::vector<int> effect_version_vector = split_content(effect_version, '.');
                // printf("-->%d, %d, %d\n", effect_version_vector[0], effect_version_vector[1], effect_version_vector[2]);

                libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, 0x{:02X}, /*version*/\\\n", effect_version_vector[0], effect_version_vector[1], effect_version_vector[2]);

                params_ver_helper_str += fmt::format("\n/*{}*/\n", effect_name);

                // effect name
                libs_info_str_sub += fmt::format("0x{:02X}, /*name length*/\\\n", effect_name.length());
                for (char c : effect_name)
                {
                    libs_info_str_sub += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                }
                libs_info_str_sub += fmt::format("/*{}*/\\\n", effect_name);

                // effect name helper
                params_ver_helper_str += fmt::format("0x{:02X}, // {} name length\n", effect_name.length(), effect_name);
                params_ver_helper_str += fmt::format("0x{:02X}, 0x{:02X}, 0x{:02X}, // effect version\n", effect_version_vector[0], effect_version_vector[1], effect_version_vector[2]);
                for (char c : effect_name)
                {
                    params_ver_helper_str += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                }
                params_ver_helper_str += "\n";

                // effect category, node type or not node type
                libs_info_str_sub += fmt::format("0x{:02x}, /*not node*/\\\n", effect_is_node == "true" ? 1 : 0);

                // bitwidth
                libs_info_str_sub += fmt::format("0x{:02X}, /*bitwidth:{}*/\\\n", bits_width_convert(effect_bitwidth), effect_bitwidth);

                // frame size formula
                libs_info_str_sub += fmt::format("0x{:02X}, /*frame size formula length*/\\\n", effect_frame_size.length());
                for (char c : effect_frame_size)
                {
                    libs_info_str_sub += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                }
                libs_info_str_sub += fmt::format("/*{}*/\\\n", effect_frame_size);

                // input counter
                int input_real_counter;
                if (effect_in1_ch == "N/A" && effect_in2_ch == "N/A")
                    input_real_counter = 0;
                else if (effect_in2_ch == "N/A")
                    input_real_counter = 1;
                else
                    input_real_counter = 2;
                libs_info_str_sub += fmt::format("0x{:02x}, /*input counter:{}*/\\\n", input_real_counter, input_real_counter);

                auto input_discript_str = split_content_str(effect_input_discript, ';');
                // fmt::print("No: {}\n", input_discript_str.size());
                if (input_real_counter == 1)
                {
                    if (input_discript_str.size() > 0) // enough items
                    {
                        libs_info_str_sub += fmt::format("0x{:02x}, /*input discript {}*/\\\n", input_discript_str[0].length(), input_discript_str[0]);
                        if (input_discript_str[0].length() > 0)
                        {
                            for (char c : input_discript_str[0])
                            {
                                libs_info_str_sub += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                            }
                            libs_info_str_sub += "\\\n";
                        }
                    }
                    else
                    {
                        libs_info_str_sub += fmt::format("0x{:02x}, /*no input discript*/\\\n", 0);
                    }
                    libs_info_str_sub += fmt::format("0x{:02X}, /*in1 ch:{}*/\\\n", channel_convert(effect_in1_ch), effect_in1_ch);
                }
                else if (input_real_counter == 2)
                {
                    if (input_discript_str.size() > 1) // enough items
                    {
                        libs_info_str_sub += fmt::format("0x{:02x}, /*input1 discript {}*/\\\n", input_discript_str[0].length(), input_discript_str[0]);
                        if (input_discript_str[0].length() > 0)
                        {
                            for (char c : input_discript_str[0])
                            {
                                libs_info_str_sub += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                            }
                            libs_info_str_sub += "\\\n";
                        }
                        libs_info_str_sub += fmt::format("0x{:02X}, /*in1 ch:{}*/\\\n", channel_convert(effect_in1_ch), effect_in1_ch);

                        libs_info_str_sub += fmt::format("0x{:02x}, /*input2 discript {}*/\\\n", input_discript_str[1].length(), input_discript_str[1]);
                        if (input_discript_str[1].length() > 0)
                        {
                            for (char c : input_discript_str[1])
                            {
                                libs_info_str_sub += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                            }
                            libs_info_str_sub += "\\\n";
                        }
                        libs_info_str_sub += fmt::format("0x{:02X}, /*in2 ch:{}*/\\\n", channel_convert(effect_in2_ch), effect_in2_ch);
                    }
                    else
                    {
                        libs_info_str_sub += fmt::format("0x{:02x}, /*in1 ch*/\\\n", 0);
                        libs_info_str_sub += fmt::format("0x{:02X}, \\\n", channel_convert(effect_in1_ch));
                        libs_info_str_sub += fmt::format("0x{:02x}, /*in2 ch*/\\\n", 0);
                        libs_info_str_sub += fmt::format("0x{:02X}, \\\n", channel_convert(effect_in2_ch));
                    }
                }

                libs_info_str_sub += fmt::format("0x{:02X}, /*output channel*/\\\n", channel_convert(effect_out_ch));

                FzParseResult res;
                res = parse_frame_size_type(effect_frame_size, 16);
                if (res.type == "FZ_ERROR")
                {
                    fmt::print("frame size format error @ {}: {}, {}\n", effect_name, effect_frame_size, res.error);
                    return -1;
                }

                api_info_str_2 += fmt::format("ROBOEFFECT_{}, {}, {}, {}, {}, {},\\\n", toUpperCase(effect_name), channel_convert_2(effect_out_ch), effect_param_number, res.type, res.length, res.array);
                api_info_str_2 += fmt::format("\troboeffect_{}_init_if, roboeffect_{}_config_if, roboeffect_{}_apply_if, roboeffect_{}_memory_size_if,\\\n", effect_name, effect_name, effect_name, effect_name);
                api_info_str_2_1 += fmt::format("ROBOEFFECT_{}, {}, {}, {}, {}, {},\\\n", toUpperCase(effect_name), channel_convert_2(effect_out_ch), effect_param_number, res.type, res.length, res.array);
                api_info_str_2_1 += fmt::format("\troboeffect_{}_init_if, roboeffect_{}_config_if, roboeffect_{}_apply_if, roboeffect_{}_memory_size_if,\\\n", "null", "null", "null", "null");

                std::string default_param_str = "\t{{\n";
                std::string param_config_str = "\t{{\n";

                engine_config_str_4 += fmt::format("#define {}{}\n\n", aligned_param_len_to_string(toUpperCase(effect_name)), effect_param_number);
                libs_info_str_sub += fmt::format("/*parameter start--->*/\\\n");
                libs_info_str_sub += fmt::format("0x{:02X}, /*effect param number:{}*/\\\n", effect_param_number, effect_param_number);

                int param_index = 0; // to show index for user

                // parameters config
                for (const auto &item : param_dict)
                {
                    // fmt::print("param name: {}\n", item.at("name"));
                    libs_info_str_sub += fmt::format("/*param name: {}*/\\\n", item.at("name"));
                    libs_info_str_sub += fmt::format("0x{:02X}, /*name size:{}*/\\\n", item.at("name").length(), item.at("name").length());

                    // param name
                    for (char c : item.at("name"))
                    {
                        libs_info_str_sub += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                    }

                    libs_info_str_sub += fmt::format("/*{}*/\\\n", item.at("name"));

                    // param type
                    libs_info_str_sub += fmt::format("0x{:02X}, /*param type:{}*/\\\n", param_type_convert(item.at("type")), item.at("type"));

                    // content
                    if (item.at("content") == "" || item.at("type") == "bool")
                    {
                    }
                    else if (item.at("type") == "enum")
                    {
                        libs_info_str_sub += fmt::format("0x{:02X}, \\\n", item.at("content").length());
                        for (char c : item.at("content"))
                        {
                            libs_info_str_sub += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                        }
                        libs_info_str_sub += fmt::format("/*enum:{}*/\\\n", item.at("content"));
                    }
                    else if (item.at("type") == "value")
                    {
                        std::vector<int> value_check;
                        value_check = split_content(item.at("content"), ',');
                        libs_info_str_sub += fmt::format("/*value:{}*/ \\\n", item.at("content"));
                        libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, \\\n", value_check[0] & 0xFF, (value_check[0] >> 8) & 0xFF);
                        libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, \\\n", value_check[1] & 0xFF, (value_check[1] >> 8) & 0xFF);
                        libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, \\\n", value_check[2] & 0xFF, (value_check[2] >> 8) & 0xFF);
                    }
                    else if (item.at("type") == "display")
                    {
                        std::vector<int> value_check;
                        value_check = split_content(item.at("content"), ',');
                        libs_info_str_sub += fmt::format("/*display:{}*/ \\\n", item.at("content"));
                        libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, \\\n", value_check[0] & 0xFF, (value_check[0] >> 8) & 0xFF);
                        libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, \\\n", value_check[1] & 0xFF, (value_check[1] >> 8) & 0xFF);
                    }
                    else if (item.at("type") == "sub-effect")
                    {
                        libs_info_str_sub += fmt::format("0x{:02X}, \\\n", item.at("content").length());
                        for (char c : item.at("content"))
                        {
                            libs_info_str_sub += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                        }
                        libs_info_str_sub += fmt::format("/*sub-effect:{}*/\\\n", item.at("content"));
                    }
                    else if (item.at("type") == "vector")
                    {
                        libs_info_str_sub += fmt::format("0x{:02X}, /*vector:{}*/\\\n", vector_type_convert(item.at("content")), item.at("content"));
                    }

                    // default
                    libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, \\\n", std::stoi(item.at("default")) & 0xFF, (std::stoi(item.at("default")) >> 8) & 0xFF);
                    default_param_str += fmt::format("\t\t{} \\\n", aligned_param_to_string(item.at("default")));
                    default_params_str += fmt::format("0x{:02X}, 0x{:02X},\t//{}\n", std::stoi(item.at("default")) & 0xFF, (std::stoi(item.at("default")) >> 8) & 0xFF, item.at("name"));

                    // param_config
                    libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, \\\n", method_map[item.at("param_config")] & 0xFF, (method_map[item.at("param_config")] >> 8) & 0xFF);
                    param_config_str += fmt::format("\t\t{} \\\n", aligned_config_to_string(item.at("param_config")));

                    if (item.at("skip") != "1") // for drc and eq_drc, one index has several parameters
                        param_index++;
                }

                libs_info_str_sub += fmt::format("/*ui display--->*/\\\n");

                // UI parameters
                for (const auto &item : param_dict)
                {
                    if (item.at("ui_type") == "")
                        continue;

                    libs_info_str_sub += fmt::format("0x{:02X}, /*type:{},{}*/\\\n", ui_display_type_convert(item.at("type"), item.at("ui_type")), item.at("type"), item.at("ui_type"));
                    if (item.at("type") == "value" || item.at("type") == "display")
                    {
                        if (item.at("unit_content") == "")
                        {
                            libs_info_str_sub += fmt::format("0x{:02X}, /*no unit content*/\\", 0);
                        }
                        else
                        {
                            libs_info_str_sub += fmt::format("0x{:02X}, /*unit content size:{}*/\\\n", item.at("unit_content").length(), item.at("unit_content").length());
                            for (char c : item.at("unit_content"))
                            {
                                libs_info_str_sub += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                            }
                            libs_info_str_sub += "\\"; // maybe has error.
                        }

                        libs_info_str_sub += "\n";
                    }

                    if (item.at("type") == "value" || item.at("type") == "display") // fract
                        libs_info_str_sub += fmt::format("0x{:02X}, /*fract*/\\\n", std::stoi(item.at("fract")));
                    if (item.at("type") == "value" || item.at("type") == "display") // fract
                        libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, /*ratio*/\\\n", std::stoi(item.at("ratio")) & 0xFF, (std::stoi(item.at("ratio")) >> 8) & 0xFF);

                    if (item.at("type") == "value" || item.at("type") == "display") // tips
                    {
                        if (item.at("tips") == "")
                        {
                            libs_info_str_sub += fmt::format("0x{:02X}, /*no tips*/\\", 0);
                        }
                        else
                        {
                            libs_info_str_sub += fmt::format("0x{:02X}, /*tips:{}*/\\\n", item.at("tips").length(), item.at("tips"));

                            for (char c : item.at("tips"))
                            {
                                libs_info_str_sub += fmt::format("0x{:02X}, ", static_cast<unsigned char>(c));
                            }
                            libs_info_str_sub += "\\";
                        }
                        libs_info_str_sub += "\n";
                    }
                }
                // UI layout
                if (param_dict.size() != 0)
                {
                    if (effect_ui_layout_mode == "auto" || effect_ui_layout_mode == "manual") // only auto or manual need layout config
                        libs_info_str_sub += fmt::format("0x{:02X}, /*ui layout:{}*/\\\n", ui_layout_convert(effect_ui_layout_mode), effect_ui_layout_mode);

                    if (effect_ui_layout_mode == "manual")
                    {
                        if (std::count(effect_ui_grid_size.begin(), effect_ui_grid_size.end(), ',') != 1)
                        {
                            fmt::print("grid_size invalid! should be AA,BB.\n");
                            throw std::invalid_argument("grid_size invalid! should be AA,BB.\n");
                        }

                        std::vector<int> grid_size_str = split_content(effect_ui_grid_size, ',');
                        int grid_row = grid_size_str[0];
                        int grid_column = grid_size_str[1];
                        libs_info_str_sub += fmt::format("/*ui layout*/\\\n");
                        libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, /*row size, column size*/\\\n", grid_row, grid_column);

                        for (const auto &item : param_dict)
                        { // UI Row and Column
                            grid_row = std::stoi(item.at("Row"));
                            grid_column = std::stoi(item.at("Column"));
                            libs_info_str_sub += fmt::format("0x{:02X}, 0x{:02X}, /*grid row, grid column*/\\\n", grid_row, grid_column);
                        }
                    }
                }
                else
                {
                    // no parameters existed, effect_ui_layout_mode should be "none"
                }

                default_param_str += "\t}},\\\n";
                param_config_str += "\t}},\\\n";

                int sub_len = count_bytes_in_string(libs_info_str_sub);

                libs_info_str += fmt::format("0x{:02X}, 0x{:02X}, \\\n", sub_len & 0xFF, (sub_len >> 8) & 0xFF);
                libs_info_str += libs_info_str_sub;

                api_info_str_2 += "\t},\n";
                api_info_str_2_1 += "\t},\n";

                // generate XXX_default_params.txt
                std::ofstream out_file(join_paths({work_folder, entry.path().string(), fmt::format("{}_default_params.txt", split_effect_name)}));
                std::string temp_str = fmt::format("0x{:02X}, /*length*/\n0x00, /*enable*/\n{}", (param_dict.size() * 2) + 1, default_params_str);
                out_file << temp_str;
                out_file << GET_INTERFACE_TEMPLATE_CODES(split_effect_name);
                // out_file << "aaaaaaa";
                out_file.close();

                // fmt::print(GET_INTERFACE_TEMPLATE_CODES(split_effect_name));

                macro_main += fmt::format("\n//Add {}_INFO_DATA to USER_DEFINED_LIBS_DATA\n", toUpperCase(split_effect_name));
                macro_main += fmt::format("#define {}_INFO_DATA \\\n", toUpperCase(split_effect_name));
                libs_info_str = del_last_ch(libs_info_str, '\\');
                macro_main += libs_info_str;

                macro_main += "\n" + engine_config_str_3;

                macro_main += fmt::format("\n//Add {}_INTERFACE to USER_DEFINED_INTERFACE\n", toUpperCase(split_effect_name));
                macro_main += fmt::format("#if {}_ENABLE\n", toUpperCase(split_effect_name));
                macro_main += fmt::format("#define {}_INTERFACE \\\n", toUpperCase(split_effect_name));
                macro_main += api_info_str_2;
                macro_main += fmt::format("#else\n");
                macro_main += fmt::format("#define {}_INTERFACE \\\n", toUpperCase(split_effect_name));
                macro_main += api_info_str_2_1;
                macro_main += fmt::format("#endif\n");

                save_bytes_in_string_to_bytes_data(libs_info_str, add_bin_data);
                third_party_num++;

                // printf("\nfinish one block\n");
                // break;//ONLY read the first sheet
                // bit_table.push_back(param_dict);
            }
        }
    }
    catch (const fs::filesystem_error &ex)
    {
        std::cerr << "filesystem err: " << ex.what() << std::endl;
        press_any_key();
    }
    catch (const xlntx::exception &ex)
    {
        std::cerr << "xlnt err: " << ex.what() << std::endl;
        press_any_key();
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught." << std::endl;
        press_any_key();
    }

    std::ofstream api_h_file(join_paths({work_folder, "user_defined_effect_api.h"}));
    api_h_file << GET_USER_DEFINED_API_H_TEMPLATE();

    for (const auto &name : name_list)
    {
        api_h_file << fmt::format("#include \"{}.h\" \n\n", name);
    }

    api_h_file << fmt::format("\n{}\n", macro_onoff_str);

    api_h_file << macro_main << "\n\n";

    // write USER_DEFINED_INTERFACE
    api_h_file << "#define USER_DEFINED_INTERFACE";
    for (const auto &name : name_list)
    {
        api_h_file << fmt::format(" \\\n\t{}_INTERFACE", toUpperCase(name));
    }
    api_h_file << "\n\n";

    // write USER_DEFINED_LIBS_DATA
    api_h_file << "#define USER_DEFINED_LIBS_DATA";
    for (const auto &name : name_list)
    {
        api_h_file << fmt::format(" \\\n\t{}_INFO_DATA", toUpperCase(name));
    }
    api_h_file << "\n\n";

    // write enum _roboeffect_user_effect_type_enum
    std::vector<std::string> name_enum = name_list;
    name_enum.push_back("total_max");

    api_h_file << "typedef enum _roboeffect_user_effect_type_enum\n{\n";
    for (const auto &name : name_enum)
    {
        std::size_t index = &name - &name_enum[0]; // C++11
        if (index == 0)
        {
            api_h_file << "\t/*Add user define effect ID here*/\n";
            api_h_file << fmt::format("\tROBOEFFECT_{} = ROBOEFFECT_USER_DEFINED_EFFECT_BEGIN,\n", toUpperCase(name));
        }
        else
        {
            api_h_file << fmt::format("\tROBOEFFECT_{},\n", toUpperCase(name));
        }
    }

    api_h_file << fmt::format("}} roboeffect_user_effect_type_enum;\n");

    api_h_file << "\n/*parameters version helper data:*/\n";
    api_h_file << "#if 0\n";
    params_ver_helper_str = fmt::format("0x{:02X}, //number of 3rd party items\n{}\n", third_party_num, params_ver_helper_str);
    int helper_bytes_counter = count_bytes_in_string(params_ver_helper_str);
    api_h_file << fmt::format("0x{:02X}, 0x{:02X}, //3rd party data total length\n", helper_bytes_counter & 0xff, (helper_bytes_counter >> 8) & 0xff);

    api_h_file << params_ver_helper_str;
    api_h_file << fmt::format("/*Total length: {}*/\n\n", helper_bytes_counter + 2);
    api_h_file << "#endif\n\n";

    // delete _new.bin
    for (const auto &entry : fs::directory_iterator(work_folder))
    {
        std::string filename = entry.path().filename().string();
        boost::regex temp_regext("roboeffect_library_info_(.*?)_v\\d+\\.\\d+\\.\\d+\\.bin");
        boost::smatch result;

        // if (boost::regex_match(filename, result, temp_regext)) {

        // 	auto fileSize = fs::file_size(entry.path());
        // 	fmt::print("{} is a 3rd party info bin file, size={}\n", filename, fileSize);
        // }

        if (fs::is_regular_file(entry.status()) && ends_with(filename, "_3rdparty.bin"))
        {
            // std::cout << "Deleting: " << entry.path().string() << std::endl;
            fs::remove(entry.path());
        }
    }

    // begin to modify roboeffect_library_info.bin
    uint32_t checksum;
    // 假�?? make_crc_table() 已经�? main 函数开头调用过一次了，这里可以省略，或者为了保险保�?
    for (const auto &entry : fs::directory_iterator(work_folder))
    {
        // 检查是否为常�?�文件且前缀匹配
        if (fs::is_regular_file(entry.path()) && entry.path().filename().string().rfind("roboeffect_library_info", 0) == 0)
        {
            std::ifstream file(entry.path().string(), std::ios::binary);

            if (!file.is_open())
            {
                std::cerr << "Error opening file: " << entry.path() << std::endl;
                return 1;
            }

            fmt::print("process: {}\n", entry.path().filename().string());

            // 1. 读取旧文件的二进制数�?
            std::vector<unsigned char> fileContent(std::istreambuf_iterator<char>(file), {});
            file.close();

            // [优化关键�? 1]：在�?改前，先创建一�?�?�?保存原�?�数�?
            // 对于 20KB 的文件，内存拷贝开销几乎�?以忽略不�?
            std::vector<unsigned char> originalContent = fileContent;

            // 2. 合并新数�? (�?�? fileContent)
            // 注意：�?�果 add_bin_data 不为空，size 就会变，内�?�肯定就不一样了
            fileContent.insert(fileContent.end(), add_bin_data.begin(), add_bin_data.end());

            // �?改特定位�?字节
            if (fileContent.size() > 10)
            {
                fileContent[10] = third_party_num;
            }

            // 3. 构造新�?�? (使用你提供的 #if 1 逻辑)
            fs::path originalPath = entry.path();
            fs::path newPath = originalPath.parent_path() / (originalPath.stem().string() + "_3rdparty" + originalPath.extension().string());

            // fmt::print("\nNew bin file: {}\n", newPath.string());
            // fmt::print("!!!Please COPY it to \\ACPWorkbench_V3.X.X\\roboeffect_library_info_set\\  \n\n");

            // 4. 写入新文�?
            std::ofstream outputFile(newPath.string(), std::ios::binary);
            if (outputFile.is_open())
            {
                outputFile.write(reinterpret_cast<char *>(fileContent.data()), fileContent.size());
                outputFile.close();
            }

            // ---------------------------------------------------------
            // [集成点] 计算 fileContent �? CRC32
            // ---------------------------------------------------------
            // �?保表已初始化 (如果在循�?外初始化过，这�?�可以去�?)
            make_crc_table();

            // 计算新文件的 CRC32
            uint32_t checksum = crc32(reinterpret_cast<const unsigned char *>(fileContent.data()), fileContent.size());

            // 打印 CRC32 方便调试
            fmt::print("Calculated CRC32: 0x{:08X}\n", checksum);

            // ---------------------------------------------------------
            // [优化需求]：�?�果新文件和旧文件完全一样，删除新文�?
            // ---------------------------------------------------------
            // std::vector 重载�? == 运算符，会比较大小和内�?�，非常方便
            if (originalContent == fileContent)
            {
                fs::remove(newPath);
                fmt::print("\nBin file not changed.\n");
            }
            else
            {
                // 如果内�?�不同，�?以在这里打印一条日�?
                // fmt::print("Content changed (Size: {} -> {}). Keeping old file.\n", originalContent.size(), fileContent.size());
                fmt::print("\nNew bin file: {}\n", newPath.string());
                fmt::print("!!!Please COPY it to \\ACPWorkbench_VX.X.X\\roboeffect_library_info_set\\  \n\n");

                update_file_checksum_name(newPath, checksum, (third_party_num)==0?(false):(true));
            }

            break; // do only one file
        }
    }

    api_h_file << fmt::format("#define EFFECT_PROPERTY_BIN_CRC32 (0x{:08X})\n", checksum);

    // save user_defined_effect_api.h
    api_h_file.close();

    press_any_key();

    return 0;
    

}
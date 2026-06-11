#include <iostream>
#include "fmt/format.h"
#include "fmt/core.h"

#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
// #include <optional>

#include "xlnt/xlnt.hpp"

#include <unordered_map>

#include "EffectInfo.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

std::vector<unsigned char> info_data_raw;

bool EffectInfo::ReadBinaryFile(const std::string &file_path)
{
    // 以二进制模式打开文件
    std::ifstream file(file_path, std::ios::binary);

    if (!file)
    {
        return false; // 如果文件打开失败，则返回false
    }

    // 确定文件大小
    file.seekg(0, std::ios::end);
    data_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 为整个文件内容分配空间
    raw_data.resize(data_size);

    // 读取文件至raw_data
    file.read(reinterpret_cast<char *>(raw_data.data()), data_size);

    return file.good(); // 返回文件读取操作是否成功
}

void EffectInfo::addItem(const EffectInfo::Item &item)
{
    items.push_back(item);
}

int EffectInfo::checkEnumRange(const uint8_t *start, uint8_t len)
{
    char temp_char[256] = {0};
    int count = 0;
    // strncpy(temp_char, (char*)(start), len);//copy name
    strncpy_s(temp_char, (char *)(start), len); // copy name

    std::string str(temp_char);
    std::istringstream iss(str);
    std::string token;

    while (std::getline(iss, token, ';'))
    {
        count++;
    }

    return count;
}

/**
 * @brief 将版本号字符串解析为整数向量
 *
 * 此函数接收一个形如 "x.y.z" 的字符串，并将其转换为一个整数向量 {x, y, z}。
 *
 * @param version 包含版本号的 const std::string&。
 * @return std::vector<int> 包含版本号各部分的整数向量。
 */
std::vector<int> EffectInfo::parseVersion(const std::string &version)
{
    std::vector<int> parts;
    std::stringstream ss(version);
    std::string item;
    // 使用'.'作为分隔符来切分字符串
    while (std::getline(ss, item, '.'))
    {
        try
        {
            // 将切分出的字符串转换为整数
            parts.push_back(std::stoi(item));
        }
        catch (const std::invalid_argument &e)
        {
            // 如果某一部分不是有效的数字，则视为0
            std::cerr << "警告: 版本号中存在无效部分 '" << item << "'，将视为 0。" << std::endl;
            parts.push_back(0);
        }
        catch (const std::out_of_range &e)
        {
            // 如果数字超出范围，则视为0
            std::cerr << "警告: 版本号中存在超出范围的数字 '" << item << "'，将视为 0。" << std::endl;
            parts.push_back(0);
        }
    }
    return parts;
}

/**
 * @brief 比较两个版本号字符串 (核心三路比较函数)
 *
 * 比较 version1 和 version2。
 *
 * @param version1 第一个版本号字符串。
 * @param version2 第二个版本号字符串。
 * @return int
 * - 正数 (1) 如果 version1 > version2
 * - 0 如果 version1 == version2
 * - 负数 (-1) 如果 version1 < version2
 */
int EffectInfo::compareVersions(const std::string &version1, const std::string &version2)
{
    std::vector<int> v1_parts = parseVersion(version1);
    std::vector<int> v2_parts = parseVersion(version2);

    // 获取两个版本号中较长的部分数量
    size_t max_len = std::max(v1_parts.size(), v2_parts.size());

    // 逐个部分进行比较
    for (size_t i = 0; i < max_len; ++i)
    {
        // 如果某个版本号的部分较少，则将其缺失的部分视为0（例如 "3.0" 和 "3.0.0" 是相等的）
        int part1 = (i < v1_parts.size()) ? v1_parts[i] : 0;
        int part2 = (i < v2_parts.size()) ? v2_parts[i] : 0;

        if (part1 > part2)
        {
            return 1;
        }
        if (part1 < part2)
        {
            return -1;
        }
    }

    // 如果所有部分都相等，则两个版本号相等
    return 0;
}

// --- 辅助布尔比较函数 ---

/**
 * @brief 判断 version1 是否大于 version2
 */
bool EffectInfo::isVersionGreater(const std::string &version1, const std::string &version2)
{
    return compareVersions(version1, version2) > 0;
}

/**
 * @brief 判断 version1 是否小于 version2
 */
bool EffectInfo::isVersionLess(const std::string &version1, const std::string &version2)
{
    return compareVersions(version1, version2) < 0;
}

/**
 * @brief 判断 version1 是否等于 version2
 */
bool EffectInfo::isVersionEqual(const std::string &version1, const std::string &version2)
{
    return compareVersions(version1, version2) == 0;
}

/**
 * @brief 判断 version1 是否大于等于 version2
 */
bool EffectInfo::isVersionGreaterOrEqual(const std::string &version1, const std::string &version2)
{
    return compareVersions(version1, version2) >= 0;
}

/**
 * @brief 判断 version1 是否小于等于 version2
 */
bool EffectInfo::isVersionLessOrEqual(const std::string &version1, const std::string &version2)
{
    return compareVersions(version1, version2) <= 0;
}

void EffectInfo::printData(void)
{
    for (auto &item : items)
    {
        fmt::print("\nname: {}, {}\n", item.name, item.param_cnt);
        for (auto &one_param : item.attributes)
        {
            const EffectInfo::ParamStruct &pp = one_param.second;
            fmt::print("param: {}, {}, {}~{}:{}\n", pp.param_name, pp.param_type, pp.minValue, pp.maxValue, pp.defaultValue);
        }
    }
}

const EffectInfo::ParamStruct &EffectInfo::getParamStruct(const std::string &effect_name, const std::string &param_name)
{
    for (auto &item : items)
    {
        if (item.name == effect_name)
        {
            auto it = item.attributes.find(param_name);
            if (it != item.attributes.end())
            {
                return it->second;
            }
            else
            {
                throw std::runtime_error("EffectInfo->param not existed: " + param_name + " in " + effect_name + "\n");
            }
        }
    }

    throw std::runtime_error("Effect not existed: " + effect_name + "\n");
}

const std::string EffectInfo::getRoboLibVer(void)
{
    return robo_ver;
}

const std::string EffectInfo::getEffectLibVer(void)
{
    return effect_lib_ver;
}

const std::streamsize EffectInfo::getRawDataFileSize(void)
{
    return data_size;
}

EffectInfo::EffectInfo(const std::string &file_path)
{
    char name[64];
    if (ReadBinaryFile(file_path))
    {
        unsigned char *ptr, *inner_p;
        unsigned char *start = raw_data.data();
        unsigned char *end = start + raw_data.size();
        buildin_cnt = raw_data[9];
        third_party_cnt = raw_data[10];
        total_cnt = buildin_cnt + third_party_cnt;
        // fmt::print("file read ok, effect counter: {}\n", total_cnt);

        try
        {
            effect_info_ver = fmt::format("{}.{}.{}", raw_data[0], raw_data[1], raw_data[2]);
            robo_ver = fmt::format("{}.{}.{}", raw_data[3], raw_data[4], raw_data[5]);
            effect_lib_ver = fmt::format("{}.{}.{}", raw_data[6], raw_data[7], raw_data[8]);

            // from first item
            ptr = &raw_data[11];

            int temp_id = 0;
            while (temp_id < total_cnt)
            {
                int param_counter, input_counter;

                inner_p = ptr + 5;
                strncpy(name_raw_buf, (char *)(inner_p + 1), *inner_p);   // copy name
                strncpy_s(name_raw_buf, (char *)(inner_p + 1), *inner_p); // copy name
                name_raw_buf[*inner_p] = '\0';
                // fmt::print("\neffect name: {}\n", name_raw_buf);

                inner_p += 1 + *inner_p; // skip name

                inner_p += 1 + 1; // effect category + bits_width

                if (isVersionGreaterOrEqual(effect_info_ver, "0.5.0"))
                {
                    // fmt::print("new ver {},{}\n", effect_info_ver, "0.5.0");
                    inner_p += 1 + *inner_p; // skip frame size
                }

                input_counter = *inner_p;
                inner_p += 1; // input counter

                // skip input description
                for (int i = 0; i < input_counter; i++)
                {
                    inner_p += 1 + *inner_p; // skip input name
                    inner_p += 1;            // skip input channel
                }

                inner_p += 1; // skip output_channels
                param_counter = *inner_p;
                inner_p += 1; // skip params_n

                EffectInfo::Item item_temp(temp_id, param_counter, name_raw_buf);
                item_temp.version[0] = ptr[2];
                item_temp.version[1] = ptr[3];
                item_temp.version[2] = ptr[4];

                int param_index = 0;
                // now inner_p pointer to the first parameter item
                while (param_counter > 0)
                {
                    int type, minValue, maxValue, defaultValue;

                    memset(name, 0x00, sizeof(name));

                    // strncpy(name_raw_buf, (char*)(inner_p + 1), *inner_p);//copy name
                    strncpy_s(name_raw_buf, (char *)(inner_p + 1), *inner_p); // copy name
                    name_raw_buf[*inner_p] = '\0';

                    // strncpy(name, (char*)(inner_p + 1), *inner_p);//copy name
                    // fmt::print("\param name: {}\n", name);

                    inner_p += 1 + *inner_p; // param_name_len + name_string

                    if (*inner_p == p_type_bool) // bool
                    {
                        type = *inner_p;
                        minValue = 0;
                        maxValue = 1;

                        inner_p += 1;
                    }
                    else if (*inner_p == p_type_enum) // enum
                    {
                        type = *inner_p;
                        minValue = 0;
                        maxValue = checkEnumRange(inner_p + 2, *(inner_p + 1)) - 1; // 0 ~ N

                        inner_p += 1 + 1 + *(inner_p + 1);
                    }
                    else if (*inner_p == p_type_value) // value
                    {
                        type = *inner_p;
                        minValue = *((int16_t *)(inner_p + 1));
                        maxValue = *((int16_t *)(inner_p + 3));

                        inner_p += 1 + 2 * 3;
                    }
                    else if (*inner_p == p_type_display) // display
                    {
                        type = *inner_p;
                        minValue = *((int16_t *)(inner_p + 1));
                        maxValue = *((int16_t *)(inner_p + 3));

                        inner_p += 1 + 2 * 2;
                    }
                    else if (*inner_p == p_type_sub) // sub-effect
                    {
                        type = *inner_p;
                        minValue = 0;
                        maxValue = 0;

                        inner_p += 1 + 1 + inner_p[1];
                    }
                    else if (*inner_p == p_type_vector) // vector
                    {
                        type = *inner_p;
                        minValue = 0;
                        maxValue = 1024;

                        inner_p += 1 + 1; // skip vector unit type
                    }
                    else
                    {
                        throw std::runtime_error(fmt::format("Wrong parameter type: {} @ {}\n", *inner_p, name_raw_buf));
                    }

                    defaultValue = *((int16_t *)inner_p);
                    inner_p += 2; // skip default

                    inner_p += 2; // skip method

                    EffectInfo::ParamStruct pps(name_raw_buf, type, minValue, maxValue, defaultValue, param_index);
                    item_temp.addAttribute(name_raw_buf, pps);
                    // fmt::print("param: {}, {}, {}~{}:{}\n", name_raw_buf, type, minValue, maxValue, defaultValue);

                    param_counter--;
                    param_index++;
                }

                //?add enable parameter:
                EffectInfo::ParamStruct pps_0("enable", p_type_bool, 0, 1, 0, 255);
                item_temp.addAttribute("enable", pps_0);

                //?add [ALL] parameters
                EffectInfo::ParamStruct pps_1("[ALL]", p_type_value, 0, 1, 0, 255);
                item_temp.addAttribute("[ALL]", pps_1);

                items.push_back(item_temp);

                ptr += *((uint16_t *)ptr) + 2;
                temp_id++;

                if (ptr >= end) //! data pointer out of range!
                    break;
            }
        }
        catch (const std::out_of_range &e)
        {
            throw std::runtime_error(std::string("Raw data out of range, maybe item counter not correct:") + e.what() + "\n");
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("Error while reading info data:") + e.what() + "\n");
        }
        catch (...)
        {
            throw std::runtime_error("Unkown Error while reading info data.\n");
        }
    }
    else
    {
        // fmt::print("File read failed: {}\n", file_path);
        throw std::runtime_error("File read failed: " + file_path + "\n");
    }
}

const EffectInfo::Item *EffectInfo::getOneItem(int index)
{
    if (index >= total_cnt)
    {
        return NULL;
    }
    else
    {
        return (const EffectInfo::Item *)&(items[index]);
    }
}
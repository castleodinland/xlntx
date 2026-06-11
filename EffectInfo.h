// EffectInfo.h
#ifndef __EFFECTINFO_H__
#define __EFFECTINFO_H__

#include <string>
#include <vector>

// 如果你使用的fmt库是作为一个外部库包含的话，确保这个库可以被链接程序访问。
#include <fmt/core.h> // 如果你用的fmt库，确保包含正确的fmt库头文件

class EffectInfo
{
public:
    struct ParamStruct
    {
        std::string param_name;
        int param_type;
        int minValue;
        int maxValue;
        int defaultValue;
        int param_index;

        ParamStruct() : param_name(""), param_type(0), minValue(0), maxValue(0), defaultValue(0), param_index(0) {}

        ParamStruct(std::string name, int type, int minVal, int maxVal, int defaultv, int pIndex)
            : param_name(name), param_type(type), minValue(minVal), maxValue(maxVal), defaultValue(defaultv), param_index(pIndex) {}
    };

    struct Item
    {
        int id;
        int param_cnt;
        unsigned char version[3];
        std::string name;
        std::map<std::string, ParamStruct> attributes;

        // Item 构造函数
        Item(int item_id, int param_cnt, std::string item_name) : id(item_id), param_cnt(param_cnt), name(item_name) {}

        // 添加属性的函数
        void addAttribute(const std::string &key, const ParamStruct &param)
        {
            attributes[key] = param;
        }
    };

private:
    std::vector<Item> items;
    std::vector<unsigned char> raw_data;
    int total_cnt;

    std::string effect_info_ver;
    std::string robo_ver;
    std::string effect_lib_ver;
    char name_raw_buf[512];
    std::streamsize data_size;

    bool ReadBinaryFile(const std::string &file_path);
    void addItem(const Item &item);
    int checkEnumRange(const uint8_t *start, uint8_t len);
    std::vector<int> parseVersion(const std::string &version);
    int compareVersions(const std::string &version1, const std::string &version2);
    bool EffectInfo::isVersionGreater(const std::string &version1, const std::string &version2);
    bool EffectInfo::isVersionLess(const std::string &version1, const std::string &version2);
    bool EffectInfo::isVersionEqual(const std::string &version1, const std::string &version2);
    bool EffectInfo::isVersionGreaterOrEqual(const std::string &version1, const std::string &version2);
    bool EffectInfo::isVersionLessOrEqual(const std::string &version1, const std::string &version2);

public:
    static constexpr int p_type_bool = 0x00;
    static constexpr int p_type_enum = 0x01;
    static constexpr int p_type_value = 0x02;
    static constexpr int p_type_display = 0x03;
    static constexpr int p_type_sub = 0x04;
    static constexpr int p_type_vector = 0x05;
    int buildin_cnt;
    int third_party_cnt;

    // 带有参数的构造函数声明
    EffectInfo(const std::string &file_path);

    void printData(void);
    const EffectInfo::ParamStruct &getParamStruct(const std::string &effect_name, const std::string &param_name);
    const std::string getRoboLibVer(void);
    const std::string getEffectLibVer(void);
    const std::streamsize getRawDataFileSize(void);
    const Item *getOneItem(int index);
};

#endif // __EFFECTINFO_H__
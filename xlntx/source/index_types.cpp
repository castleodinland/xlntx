#include <xlntx/cell/index_types.hpp>

#include <cctype>
#include <stdexcept>

namespace xlntx {

column_t::column_t(const std::string &column_string)
    : index(column_index_from_string(column_string))
{
}

column_t::column_t(const char *column_string)
    : index(column_index_from_string(std::string(column_string)))
{
}

column_t::index_t column_t::column_index_from_string(const std::string &column_string)
{
    if (column_string.empty())
    {
        throw std::invalid_argument("empty column string");
    }

    index_t result = 0;

    for (char c : column_string)
    {
        if (!std::isalpha(static_cast<unsigned char>(c)))
        {
            throw std::invalid_argument("invalid character in column string: " + column_string);
        }
        result = result * 26 + (std::toupper(static_cast<unsigned char>(c)) - 'A' + 1);
    }

    return result;
}

std::string column_t::column_string_from_index(index_t column_index)
{
    std::string result;

    while (column_index > 0)
    {
        column_index--;
        result.insert(result.begin(), static_cast<char>('A' + (column_index % 26)));
        column_index /= 26;
    }

    return result;
}

std::string column_t::column_string() const
{
    return column_string_from_index(index);
}

column_t &column_t::operator=(const std::string &column_string)
{
    index = column_index_from_string(column_string);
    return *this;
}

column_t &column_t::operator=(const char *column_string)
{
    index = column_index_from_string(std::string(column_string));
    return *this;
}

} // namespace xlntx

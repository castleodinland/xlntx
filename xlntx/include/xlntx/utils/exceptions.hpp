#pragma once

#include <stdexcept>
#include <string>

#include <xlntx/xlntx_config.hpp>
#include <xlntx/cell/index_types.hpp>

namespace xlntx {

class XLNTX_API exception : public std::runtime_error
{
public:
    exception(const std::string &message);
    virtual ~exception();

    void message(const std::string &message);

private:
    std::string message_;
};

class XLNTX_API invalid_parameter : public exception
{
public:
    invalid_parameter();
};

class XLNTX_API invalid_sheet_title : public exception
{
public:
    invalid_sheet_title(const std::string &title);
};

class XLNTX_API missing_number_format : public exception
{
public:
    missing_number_format();
};

class XLNTX_API invalid_file : public exception
{
public:
    invalid_file(const std::string &filename);
};

class XLNTX_API illegal_character : public exception
{
public:
    illegal_character(char c);
};

class XLNTX_API invalid_data_type : public exception
{
public:
    invalid_data_type();
};

class XLNTX_API invalid_column_index : public exception
{
public:
    invalid_column_index();
};

class XLNTX_API invalid_cell_reference : public exception
{
public:
    invalid_cell_reference(column_t column, row_t row);
    invalid_cell_reference(const std::string &reference_string);
};

class XLNTX_API invalid_attribute : public exception
{
public:
    invalid_attribute();
};

class XLNTX_API key_not_found : public exception
{
public:
    key_not_found();
};

class XLNTX_API no_visible_worksheets : public exception
{
public:
    no_visible_worksheets();
};

class XLNTX_API unhandled_switch_case : public exception
{
public:
    unhandled_switch_case();
};

class XLNTX_API unsupported : public exception
{
public:
    unsupported(const std::string &message);
};

} // namespace xlntx

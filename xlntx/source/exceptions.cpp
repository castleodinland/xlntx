#include <xlntx/utils/exceptions.hpp>

namespace xlntx {

exception::exception(const std::string &message)
    : std::runtime_error(message), message_(message)
{
}

exception::~exception() = default;

void exception::message(const std::string &message)
{
    message_ = message;
}

invalid_parameter::invalid_parameter()
    : exception("invalid parameter")
{
}

invalid_sheet_title::invalid_sheet_title(const std::string &title)
    : exception("invalid sheet title: " + title)
{
}

missing_number_format::missing_number_format()
    : exception("missing number format")
{
}

invalid_file::invalid_file(const std::string &filename)
    : exception("invalid file: " + filename)
{
}

illegal_character::illegal_character(char c)
    : exception("illegal character: " + std::string(1, c))
{
}

invalid_data_type::invalid_data_type()
    : exception("invalid data type")
{
}

invalid_column_index::invalid_column_index()
    : exception("invalid column index")
{
}

invalid_cell_reference::invalid_cell_reference(column_t column, row_t row)
    : exception("invalid cell reference: " + column.column_string() + ":" + std::to_string(row))
{
}

invalid_cell_reference::invalid_cell_reference(const std::string &reference_string)
    : exception("invalid cell reference: " + reference_string)
{
}

invalid_attribute::invalid_attribute()
    : exception("invalid attribute")
{
}

key_not_found::key_not_found()
    : exception("key not found")
{
}

no_visible_worksheets::no_visible_worksheets()
    : exception("no visible worksheets")
{
}

unhandled_switch_case::unhandled_switch_case()
    : exception("unhandled switch case")
{
}

unsupported::unsupported(const std::string &message)
    : exception("unsupported: " + message)
{
}

} // namespace xlntx

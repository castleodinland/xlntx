#pragma once

#include <xlntx/xlntx_config.hpp>

namespace xlntx {

enum class cell_type
{
    empty,
    boolean,
    date,
    error,
    inline_string,
    number,
    shared_string,
    formula_string
};

} // namespace xlntx

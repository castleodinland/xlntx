#pragma once

#include <xlntx/xlntx_config.hpp>
#include <xlntx/cell/index_types.hpp>

namespace xlntx {

class cell;
class worksheet;

class XLNTX_API cell_vector
{
public:
    cell_vector(worksheet &ws, row_t row);
    cell_vector(const worksheet &ws, row_t row);

    class cell operator[](std::size_t cell_index);
    const class cell operator[](std::size_t cell_index) const;

    class cell front();
    const class cell front() const;

    class cell back();
    const class cell back() const;

    std::size_t length() const;
    row_t row() const { return row_; }

    bool operator==(const cell_vector &other) const;
    bool operator!=(const cell_vector &other) const;

private:
    worksheet *ws_;
    const worksheet *const_ws_;
    row_t row_;
};

} // namespace xlntx

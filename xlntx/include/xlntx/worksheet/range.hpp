#pragma once

#include <cstddef>
#include <functional>
#include <iterator>

#include <xlntx/xlntx_config.hpp>
#include <xlntx/cell/index_types.hpp>
#include <xlntx/cell/cell_reference.hpp>
#include <xlntx/worksheet/cell_vector.hpp>
#include <xlntx/worksheet/major_order.hpp>
#include <xlntx/worksheet/range_reference.hpp>
#include <xlntx/worksheet/worksheet.hpp>

// Forward declarations for style stubs
#include <xlntx/styles/alignment.hpp>
#include <xlntx/styles/border.hpp>
#include <xlntx/styles/fill.hpp>
#include <xlntx/styles/font.hpp>
#include <xlntx/styles/format.hpp>
#include <xlntx/styles/number_format.hpp>
#include <xlntx/styles/protection.hpp>
#include <xlntx/styles/style.hpp>
#include <xlntx/styles/conditional_format.hpp>

namespace xlntx {

class cell;

class XLNTX_API range
{
public:
    using iterator = range_iterator;
    using const_iterator = const_range_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    range(worksheet ws, const range_reference &reference,
          major_order order = major_order::row, bool skip_null = false);

    void clear_cells();
    cell_vector vector(std::size_t n);
    const cell_vector vector(std::size_t n) const;
    class cell cell(const cell_reference &ref);
    const class cell cell(const cell_reference &ref) const;

    const worksheet &target_worksheet() const;
    range_reference reference() const;
    std::size_t length() const;
    bool contains(const cell_reference &ref);

    // Style application methods (stubs for read-only)
    range alignment(const xlntx::alignment &a) { return *this; }
    range border(const xlntx::border &b) { return *this; }
    range fill(const xlntx::fill &f) { return *this; }
    range font(const xlntx::font &f) { return *this; }
    range number_format(const xlntx::number_format &nf) { return *this; }
    range protection(const xlntx::protection &p) { return *this; }
    range style(const class style &s) { return *this; }
    range style(const std::string &) { return *this; }
    xlntx::conditional_format conditional_format(const condition &) { return xlntx::conditional_format(); }

    cell_vector front();
    const cell_vector front() const;
    cell_vector back();
    const cell_vector back() const;

    // Iteration
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    cell_vector operator[](std::size_t n);
    const cell_vector operator[](std::size_t n) const;

    bool operator==(const range &other) const;
    bool operator!=(const range &other) const;

    void apply(std::function<void(class cell)> f);

private:
    worksheet ws_;
    range_reference ref_;
    major_order order_;
    bool skip_null_;
};

// Minimal range_iterator for row iteration
class XLNTX_API range_iterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = cell_vector;
    using difference_type = std::ptrdiff_t;
    using pointer = cell_vector *;
    using reference = cell_vector;

    range_iterator();
    range_iterator(worksheet &ws, row_t row, bool skip_null);

    cell_vector operator*();
    range_iterator &operator++();
    range_iterator operator++(int);
    bool operator==(const range_iterator &other) const;
    bool operator!=(const range_iterator &other) const;

private:
    worksheet *ws_ = nullptr;
    row_t row_ = 0;
    bool skip_null_ = true;
    row_t max_row_ = 0;
    void advance_to_valid();
};

class XLNTX_API const_range_iterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const cell_vector;
    using difference_type = std::ptrdiff_t;

    const_range_iterator();
    const_range_iterator(const worksheet &ws, row_t row, bool skip_null);

    const cell_vector operator*();
    const_range_iterator &operator++();
    const_range_iterator operator++(int);
    bool operator==(const const_range_iterator &other) const;
    bool operator!=(const const_range_iterator &other) const;

private:
    const worksheet *ws_ = nullptr;
    row_t row_ = 0;
    bool skip_null_ = true;
    row_t max_row_ = 0;
    void advance_to_valid();
};

} // namespace xlntx

#pragma once
#include <xlntx/xlntx_config.hpp>
#include <xlntx/packaging/uri.hpp>
#include <string>
namespace xlntx {
enum class relationship_type {
    office_document, workbook, worksheet, shared_strings, styles,
    theme, image, chart, comments, vml_drawing,
    core_properties, extended_properties, custom_properties,
    custom_xml, thumbnail, calculation_chain, connections,
    external_link, printer_settings, unknown
};
class XLNTX_API relationship {
public:
    relationship() = default;
    relationship(const std::string &id, relationship_type type,
                 const uri &source, const uri &target);
private:
    std::string id_;
    relationship_type type_ = relationship_type::unknown;
    uri source_;
    uri target_;
};
} // namespace xlntx

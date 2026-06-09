#pragma once

#include <cstdint>
#include <istream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace xlntx {
namespace detail {

struct zip_header
{
    std::uint16_t version = 20;
    std::uint16_t flags = 0;
    std::uint16_t compression_type = 8;
    std::uint16_t stamp_date = 0;
    std::uint16_t stamp_time = 0;
    std::uint32_t crc = 0;
    std::uint32_t compressed_size = 0;
    std::uint32_t uncompressed_size = 0;
    std::string filename;
    std::string comment;
    std::vector<std::uint8_t> extra;
    std::uint32_t header_offset = 0;
};

class zip_reader
{
public:
    explicit zip_reader(std::istream &stream);
    ~zip_reader();

    std::unique_ptr<std::streambuf> open(const std::string &filename) const;
    std::string read(const std::string &filename) const;
    std::vector<std::string> files() const;
    bool has_file(const std::string &filename) const;

private:
    bool read_central_header();
    std::unordered_map<std::string, zip_header> file_headers_;
    std::istream &source_stream_;
};

std::string read_stream_to_string(std::istream &stream);

} // namespace detail
} // namespace xlntx

#include "zip_reader.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <miniz.h>

namespace xlntx {
namespace detail {

namespace {

template <class T>
T read_int(std::istream &stream)
{
    T value;
    stream.read(reinterpret_cast<char *>(&value), sizeof(T));
    return value;
}

zip_header read_local_header(std::istream &istream)
{
    zip_header header;
    auto sig = read_int<std::uint32_t>(istream);
    if (sig != 0x04034b50)
        throw std::runtime_error("missing local header signature");

    header.version = read_int<std::uint16_t>(istream);
    header.flags = read_int<std::uint16_t>(istream);
    header.compression_type = read_int<std::uint16_t>(istream);
    header.stamp_date = read_int<std::uint16_t>(istream);
    header.stamp_time = read_int<std::uint16_t>(istream);
    header.crc = read_int<std::uint32_t>(istream);
    header.compressed_size = read_int<std::uint32_t>(istream);
    header.uncompressed_size = read_int<std::uint32_t>(istream);

    auto filename_length = read_int<std::uint16_t>(istream);
    auto extra_length = read_int<std::uint16_t>(istream);

    header.filename.resize(filename_length, '\0');
    istream.read(&header.filename[0], filename_length);
    header.extra.resize(extra_length, 0);
    istream.read(reinterpret_cast<char *>(header.extra.data()), extra_length);

    return header;
}

zip_header read_global_header(std::istream &istream)
{
    zip_header header;
    auto sig = read_int<std::uint32_t>(istream);
    if (sig != 0x02014b50)
        throw std::runtime_error("missing global header signature");

    header.version = read_int<std::uint16_t>(istream);
    header.version = read_int<std::uint16_t>(istream); // read twice
    header.flags = read_int<std::uint16_t>(istream);
    header.compression_type = read_int<std::uint16_t>(istream);
    header.stamp_date = read_int<std::uint16_t>(istream);
    header.stamp_time = read_int<std::uint16_t>(istream);
    header.crc = read_int<std::uint32_t>(istream);
    header.compressed_size = read_int<std::uint32_t>(istream);
    header.uncompressed_size = read_int<std::uint32_t>(istream);

    auto filename_length = read_int<std::uint16_t>(istream);
    auto extra_length = read_int<std::uint16_t>(istream);
    auto comment_length = read_int<std::uint16_t>(istream);

    /* disk_number_start */ read_int<std::uint16_t>(istream);
    /* int_file_attrib */ read_int<std::uint16_t>(istream);
    /* ext_file_attrib */ read_int<std::uint32_t>(istream);
    header.header_offset = read_int<std::uint32_t>(istream);

    header.filename.resize(filename_length, '\0');
    istream.read(&header.filename[0], filename_length);
    header.extra.resize(extra_length, 0);
    istream.read(reinterpret_cast<char *>(header.extra.data()), extra_length);
    header.comment.resize(comment_length, '\0');
    istream.read(&header.comment[0], comment_length);

    return header;
}

constexpr std::size_t buffer_size = 512;

class zip_streambuf_decompress : public std::streambuf
{
    std::istream &istream_;
    z_stream strm_;
    std::array<char, buffer_size> in_;
    std::array<char, buffer_size> out_;
    zip_header header_;
    std::size_t total_read_ = 0;
    std::size_t total_uncompressed_ = 0;
    bool valid_ = true;
    bool compressed_data_ = false;

    static const unsigned short DEFLATE = 8;
    static const unsigned short UNCOMPRESSED = 0;

public:
    zip_streambuf_decompress(std::istream &stream, zip_header central_header)
        : istream_(stream), header_(central_header)
    {
        in_.fill(0);
        out_.fill(0);

        strm_.zalloc = nullptr;
        strm_.zfree = nullptr;
        strm_.opaque = nullptr;
        strm_.avail_in = 0;
        strm_.next_in = nullptr;

        setg(in_.data(), in_.data(), in_.data());
        setp(nullptr, nullptr);

        // Skip the local header
        read_local_header(istream_);

        if (header_.compression_type == DEFLATE)
            compressed_data_ = true;
        else if (header_.compression_type == UNCOMPRESSED)
            compressed_data_ = false;
        else
            throw std::runtime_error("unsupported compression type");

        if (compressed_data_ && valid_)
        {
            int result = inflateInit2(&strm_, -MAX_WBITS);
            if (result != Z_OK)
                throw std::runtime_error("couldn't inflate ZIP, possibly corrupted");
        }
    }

    ~zip_streambuf_decompress() override
    {
        if (compressed_data_ && valid_)
            inflateEnd(&strm_);
    }

    int process()
    {
        if (!valid_) return -1;

        if (compressed_data_)
        {
            strm_.avail_out = buffer_size - 4;
            strm_.next_out = reinterpret_cast<Bytef *>(out_.data() + 4);

            while (strm_.avail_out != 0)
            {
                if (strm_.avail_in == 0)
                {
                    istream_.read(in_.data(),
                        static_cast<std::streamsize>(std::min(buffer_size,
                            static_cast<std::size_t>(header_.compressed_size - total_read_))));
                    strm_.avail_in = static_cast<unsigned int>(istream_.gcount());
                    total_read_ += strm_.avail_in;
                    strm_.next_in = reinterpret_cast<Bytef *>(in_.data());
                }

                auto ret = inflate(&strm_, Z_NO_FLUSH);
                if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR)
                    throw std::runtime_error("couldn't inflate ZIP, possibly corrupted");
                if (ret == Z_STREAM_END) break;
            }

            auto unzip_count = buffer_size - strm_.avail_out - 4;
            total_uncompressed_ += unzip_count;
            return static_cast<int>(unzip_count);
        }

        // Uncompressed
        istream_.read(out_.data() + 4,
            static_cast<std::streamsize>(std::min(buffer_size - 4,
                static_cast<std::size_t>(header_.uncompressed_size - total_read_))));
        auto count = istream_.gcount();
        total_read_ += static_cast<std::size_t>(count);
        return static_cast<int>(count);
    }

    int underflow() override
    {
        if (gptr() && (gptr() < egptr()))
            return traits_type::to_int_type(*gptr());

        auto put_back_count = gptr() - eback();
        if (put_back_count > 4) put_back_count = 4;
        std::memmove(out_.data() + (4 - put_back_count),
                     gptr() - put_back_count, static_cast<std::size_t>(put_back_count));

        int num = process();
        setg(out_.data() + 4 - put_back_count, out_.data() + 4, out_.data() + 4 + num);
        if (num <= 0) return EOF;
        return traits_type::to_int_type(*gptr());
    }

    int overflow(int = EOF) override
    {
        throw std::runtime_error("writing to read-only buffer");
    }
};

} // anonymous namespace

zip_reader::zip_reader(std::istream &stream)
    : source_stream_(stream)
{
    if (!stream)
        throw std::runtime_error("invalid file handle");
    read_central_header();
}

zip_reader::~zip_reader() = default;

bool zip_reader::read_central_header()
{
    source_stream_.seekg(0, std::ios_base::end);
    auto end_position = source_stream_.tellg();

    auto max_comment_size = std::uint32_t(0xffff);
    auto read_size_before_comment = std::uint32_t(22);
    std::streamoff read_start = max_comment_size + read_size_before_comment;

    if (read_start > end_position)
        read_start = end_position;

    source_stream_.seekg(end_position - read_start);
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(read_start), '\0');

    if (read_start <= 0)
        throw std::runtime_error("file is empty");

    source_stream_.read(reinterpret_cast<char *>(buf.data()), read_start);

    if (buf[0] == 0xd0 && buf[1] == 0xcf && buf[2] == 0x11 && buf[3] == 0xe0)
        throw std::runtime_error("encrypted xlsx, password required");

    bool found_header = false;
    std::streamoff header_index = 0;

    for (std::streamoff i = 0; i < read_start - 3; ++i)
    {
        if (buf[static_cast<std::size_t>(i)] == 0x50
            && buf[static_cast<std::size_t>(i) + 1] == 0x4b
            && buf[static_cast<std::size_t>(i) + 2] == 0x05
            && buf[static_cast<std::size_t>(i) + 3] == 0x06)
        {
            found_header = true;
            header_index = i;
            break;
        }
    }

    if (!found_header)
        throw std::runtime_error("failed to find zip header");

    source_stream_.seekg(end_position - (read_start - header_index));

    /* eocd sig */ read_int<std::uint32_t>(source_stream_);
    auto disk_number1 = read_int<std::uint16_t>(source_stream_);
    auto disk_number2 = read_int<std::uint16_t>(source_stream_);

    if (disk_number1 != disk_number2 || disk_number1 != 0)
        throw std::runtime_error("multiple disk zip files are not supported");

    auto num_files = read_int<std::uint16_t>(source_stream_);
    auto num_files_this_disk = read_int<std::uint16_t>(source_stream_);

    if (num_files != num_files_this_disk)
        throw std::runtime_error("multi disk zip files are not supported");

    /* size_of_header */ read_int<std::uint32_t>(source_stream_);
    auto header_offset = read_int<std::uint32_t>(source_stream_);

    source_stream_.seekg(header_offset);

    for (std::uint16_t i = 0; i < num_files; ++i)
    {
        auto header = read_global_header(source_stream_);
        file_headers_[header.filename] = header;
    }

    return true;
}

std::unique_ptr<std::streambuf> zip_reader::open(const std::string &filename) const
{
    auto it = file_headers_.find(filename);
    if (it == file_headers_.end())
        throw std::runtime_error("file not found in zip: " + filename);

    auto &header = it->second;
    source_stream_.seekg(header.header_offset);
    return std::unique_ptr<std::streambuf>(
        new zip_streambuf_decompress(source_stream_, header));
}

std::string zip_reader::read(const std::string &filename) const
{
    auto buffer = open(filename);
    std::istream stream(buffer.get());
    return read_stream_to_string(stream);
}

std::vector<std::string> zip_reader::files() const
{
    std::vector<std::string> filenames;
    for (const auto &pair : file_headers_)
        filenames.push_back(pair.first);
    return filenames;
}

bool zip_reader::has_file(const std::string &filename) const
{
    return file_headers_.find(filename) != file_headers_.end();
}

std::string read_stream_to_string(std::istream &stream)
{
    return std::string(std::istreambuf_iterator<char>(stream),
                       std::istreambuf_iterator<char>());
}

} // namespace detail
} // namespace xlntx

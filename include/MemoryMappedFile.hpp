// MemoryMappedFile.hpp
#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

class MemoryMappedFile {
public:
    explicit MemoryMappedFile(const std::filesystem::path& pcap_filepath);

    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

    ~MemoryMappedFile();

    std::span<const uint8_t> file_reader() const;

private:
    int m_fd = -1;
    uint8_t* m_data = nullptr;
    size_t m_size = 0;
};

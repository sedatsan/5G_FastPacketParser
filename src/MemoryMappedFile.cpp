// MemoryMappedFile.cpp
#include "MemoryMappedFile.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <stdexcept>

MemoryMappedFile::MemoryMappedFile(const std::filesystem::path &pcap_filepath) {
  m_fd = open(pcap_filepath.c_str(), O_RDONLY);

  if (m_fd == -1) {
    throw std::runtime_error("Failed to open PCAP file");
  }

  struct stat file_stat;
  int result = fstat(m_fd, &file_stat);
  if (result == -1) {
    throw std::runtime_error("Failed to open PCAP file");
  }

  m_size = file_stat.st_size;

  void *mapped_ptr = mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0);
  if (mapped_ptr == MAP_FAILED) {
    throw std::runtime_error("Failed to open PCAP file");
  }

  m_data = static_cast<uint8_t *>(mapped_ptr);
}

MemoryMappedFile::~MemoryMappedFile() {
  if (m_data != nullptr) {
    munmap(m_data, m_size);
  }
  if (m_fd >= 0) {
    close(m_fd);
  }
}

std::span<const uint8_t> MemoryMappedFile::file_reader() const {
  return {m_data, m_size};
}

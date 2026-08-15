// MacParser.hpp
#pragma once

#include <cstdint>
#include <span>

class MacParser {
public:
  explicit MacParser(std::span<const uint8_t> pcap_data);

  void parse() const;

private:
  std::span<const uint8_t> m_data;

  void process_payload(const uint8_t *payload_start,
                       uint32_t payload_size) const;
};

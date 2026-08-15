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

  void process_payload(std::span<const uint8_t> payload) const;
};

// MacParser.cpp
#include "MacParser.hpp"
#include "PcapStructs.hpp"

#include <bit>
#include <cstdint>
#include <iostream>
#include <span>

MacParser::MacParser(std::span<const uint8_t> pcap_data) : m_data(pcap_data) {}

void MacParser::parse() const {

  const PcapGlobalHeader *header =
      reinterpret_cast<const PcapGlobalHeader *>(m_data.data());

  std::cout << "Magic Number: 0x" << std::hex << header->magic_number
            << std::endl
            << std::dec << "Link Type: " << header->link_type << std::endl;

  size_t current_offset = 24;
  int packet_count = 0;

  while (current_offset < m_data.size()) {

    const PcapRecordHeader *record_header =
        reinterpret_cast<const PcapRecordHeader *>(m_data.data() +
                                                   current_offset);

    current_offset += sizeof(PcapRecordHeader) + record_header->captured_len;
    packet_count++;

    const uint8_t *payload_ptr =
        reinterpret_cast<const uint8_t *>(record_header) +
        sizeof(PcapRecordHeader);

    const uint16_t *raw_ptr = reinterpret_cast<const uint16_t *>(payload_ptr);
    uint16_t swapped_payload = std::byteswap(*raw_ptr);

    std::cout << "first 2 payload bytes before flip: " << std::hex << *raw_ptr
              << std::endl
              << "after flip : " << std::hex << swapped_payload << std::endl;
  }

  std::cout << "Packet Count: " << packet_count << std::endl;
}

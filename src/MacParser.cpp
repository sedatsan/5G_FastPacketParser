// MacParser.cpp
#include "MacParser.hpp"
#include "PcapStructs.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>

MacParser::MacParser(std::span<const uint8_t> pcap_data) : m_data(pcap_data) {}

void MacParser::process_payload(const uint8_t *payload_start,
                                uint32_t payload_size) const {
  size_t tlv_offset = 0;
  while (tlv_offset < payload_size) {

    const WiresharkTlvHeader *tlv_header =
        reinterpret_cast<const WiresharkTlvHeader *>(payload_start +
                                                     tlv_offset);
    uint16_t tag = std::byteswap(tlv_header->tag);
    uint16_t length = std::byteswap(tlv_header->length);

    if (tag == 0) {
      tlv_offset += sizeof(WiresharkTlvHeader);
      break;
    };

    tlv_offset += sizeof(WiresharkTlvHeader) + length;
  }
  std::cout << "MAC PDU starts at offset: " << tlv_offset << std::endl;

  const uint8_t *mac_start = payload_start + tlv_offset + 9;
  uint8_t first_byte = *mac_start;
  uint8_t format = (first_byte & 0b01000000U) >> 6;
  uint8_t reserve = (first_byte & 0b10000000U) >> 7;
  first_byte &= 0b00111111U;

  std::cout << static_cast<uint32_t>(first_byte) << std::endl
            << static_cast<uint32_t>(format) << std::endl
            << static_cast<uint32_t>(reserve) << std::endl;
  uint8_t payload_length = mac_start[1];
  std::cout << static_cast<uint32_t>(payload_length) << std::endl;

  std::cout << "Hex Dump: ";
  for (int i = 0; i < 16; i++) {
    std::cout << std::hex << (unsigned)mac_start[i] << " ";
  }
  std::cout << std::dec << std::endl;
}

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

    process_payload(payload_ptr, record_header->captured_len);
  }

  std::cout << "Packet Count: " << packet_count << std::endl;
}

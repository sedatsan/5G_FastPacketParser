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

  int subpdu_index = 0;
  size_t mac_offset = tlv_offset + 9;
  while (mac_offset < payload_size) {
    const uint8_t *mac_start = payload_start + mac_offset;
    MacSubheader mac_subheader(mac_start);

    uint8_t lcid = mac_subheader.get_lcid();
    uint16_t len = mac_subheader.get_length();
    uint8_t mac_header_size = mac_subheader.get_header_size();

    std::cout << "  [SubPDU " << subpdu_index++ << "] "
              << "LCID: " << static_cast<uint32_t>(lcid)
              << ", Payload Len: " << len
              << ", Hdr Size: " << static_cast<uint32_t>(mac_header_size)
              << " bytes" << '\n';

    // If Padding (LCID 63), we reached the end of useful subPDUs
    if (lcid == 63) {
      std::cout << "  [Padding Detected - End of Transport Block]" << '\n';
      break;
    }
    mac_offset += mac_header_size + len;
  }
}

void MacParser::parse() const {

  const PcapGlobalHeader *header =
      reinterpret_cast<const PcapGlobalHeader *>(m_data.data());

  std::cout << "Magic Number: 0x" << std::hex << header->magic_number << '\n'
            << std::dec << "Link Type: " << header->link_type << '\n';

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

  std::cout << "Packet Count: " << packet_count << '\n';
}

// PcapStructs.hpp
#pragma once

#include <cstdint>

struct __attribute__((__packed__)) PcapGlobalHeader {
  uint32_t magic_number;
  uint16_t major_version;
  uint16_t minor_version;
  uint64_t reserve; // deprecated
  uint32_t snap_len;
  uint32_t link_type;
};

struct __attribute__((__packed__)) PcapRecordHeader {
  uint32_t timestamp_sec;
  uint32_t timestamp_usec;
  uint32_t captured_len;
  uint32_t origin_len;
};

struct __attribute__((__packed__)) WiresharkTlvHeader {
  uint16_t tag;
  uint16_t length;
};

class MacSubheader {
public:
  static constexpr uint8_t lcid_mask = 0b00111111U;
  static constexpr uint8_t format_mask = 0b01000000U;
  static constexpr uint8_t format_shift = 6;
  static constexpr uint8_t min_sdu_lcid = 1;
  static constexpr uint8_t max_sdu_lcid = 32;
  static constexpr uint8_t padding_lcid = 63;

private:
  uint8_t m_first_byte{0};
  uint16_t m_length{0};
  uint8_t m_header_size{1};

public:
  MacSubheader() = default;

  // Parses the subheader from memory, determines header size and payload length
  explicit MacSubheader(const uint8_t *data) : m_first_byte(data[0]) {

    uint8_t lcid = get_lcid();

    if (lcid >= min_sdu_lcid && lcid <= max_sdu_lcid) {
      uint8_t format = get_format();
      if (format == 0) {

        m_length = data[1];
        m_header_size = 2;
      } else {

        m_length = (static_cast<uint16_t>(data[1]) << 8) | data[2];
        m_header_size = 3;
      }

    } else if (lcid == padding_lcid) {
      // Padding subheader
      m_length = 0;
      m_header_size = 1;
    }
  }
  [[nodiscard]] auto get_lcid() const -> uint8_t {
    return m_first_byte & lcid_mask;
  }

  [[nodiscard]] auto get_format() const -> uint8_t {
    return (m_first_byte & format_mask) >> format_shift;
  }

  [[nodiscard]] auto get_length() const -> uint16_t { return m_length; }

  [[nodiscard]] auto get_header_size() const -> uint8_t {
    return m_header_size;
  }
};

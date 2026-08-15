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

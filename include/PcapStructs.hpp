#include <cstdint>

struct __attribute__((__packed__)) PcapGlobalHeader {
    uint32_t magic_number;
    uint16_t major_version;
    uint16_t minor_version;
    uint64_t reserve;  // deprecated
    uint32_t snap_len;
    uint32_t link_type;
};

# 5G Fast Packet Parser

A high-performance C++23 parser designed to extract and decode 5G NR MAC Protocol Data Units (PDUs) from raw PCAP captures. Built for high-throughput packet processing environments (simulating a 100MHz 4x2 cell bypassing the OCUDU core network), it demonstrates zero-copy packet ingestion, packed structure memory mapping, and bit-level protocol dissection.

---

## System Architecture & Zero-Copy Pipeline

The parser avoids user-space buffer allocations and payload copies by mapping PCAP files directly into process virtual address space via POSIX `mmap`. Memory spans (`std::span`) provide safe, bounded views, allowing packed struct overlays (`reinterpret_cast`) and pointer arithmetic to traverse nested protocol headers.

```mermaid
flowchart TD
    subgraph Disk ["1. Storage Layer"]
        FILE["gnb_mac.pcap<br/>Raw PCAP on Disk"]
    end

    subgraph Ingestion ["2. Zero-Copy Ingestion Layer (RAII)"]
        MMF["MemoryMappedFile<br/>open() -> fstat() -> mmap()"]
        SPAN["std::span&lt;const uint8_t&gt;<br/>Non-owning contiguous byte view"]
    end

    subgraph Parsing ["3. In-Memory Protocol Dissection (MacParser)"]
        direction TB
        GH["PcapGlobalHeader (24 Bytes)<br/>reinterpret_cast&lt;const PcapGlobalHeader*&gt;(data)<br/>Validates Magic (0xa1b2c3d4) & LinkType (252)"]
        
        subgraph RecordLoop ["Packet Iteration Loop (offset += 16B + captured_len)"]
            RH["PcapRecordHeader (16 Bytes)<br/>reinterpret_cast&lt;const PcapRecordHeader*&gt;(data + offset)"]
            PAYLOAD["Raw Payload Slice<br/>record_ptr + sizeof(PcapRecordHeader)"]
            
            subgraph TLVLoop ["Wireshark Exported PDU Navigation"]
                TLV["WiresharkTlvHeader<br/>reinterpret_cast&lt;const WiresharkTlvHeader*&gt;()<br/>std::byteswap(tag), std::byteswap(len)"]
                END_TAG["Tag == 0 (End of Options)<br/>Calculates dynamic TLV offset"]
            end
            
            MAC_PDU["5G NR MAC PDU Target<br/>payload + tlv_offset + 9B meta<br/>Bitfield extraction: Format, LCID, Length"]
        end
    end

    FILE -->|"POSIX file descriptor"| MMF
    MMF -->|"PROT_READ | MAP_PRIVATE"| SPAN
    SPAN -->|"Zero-copy view injection"| GH
    GH -->|"Offset 0x18"| RH
    RH -->|"Pointer arithmetic"| PAYLOAD
    PAYLOAD -->|"Iterate options"| TLV
    TLV -->|"Find Tag 0"| END_TAG
    END_TAG -->|"Offset + 9"| MAC_PDU
```

---

## Detailed Memory Layout & Struct Mapping

Below is the byte-level mapping applied directly to the contiguous mapped memory:

```mermaid
classDiagram
    class PcapGlobalHeader {
        +uint32_t magic_number (0xa1b2c3d4)
        +uint16_t major_version (2)
        +uint16_t minor_version (4)
        +uint64_t reserve (deprecated)
        +uint32_t snap_len
        +uint32_t link_type (252: LINKTYPE_WIRESHARK_UPPER_PDU)
    }

    class PcapRecordHeader {
        +uint32_t timestamp_sec
        +uint32_t timestamp_usec
        +uint32_t captured_len
        +uint32_t origin_len
    }

    class WiresharkTlvHeader {
        +uint16_t tag (Big-Endian -> std::byteswap)
        +uint16_t length (Big-Endian -> std::byteswap)
    }

    class MacPduPayload {
        +uint8_t R_F_LCID_byte
        +uint8_t payload_length
        +uint8_t[] mac_sdu_data
    }

    PcapGlobalHeader -- PcapRecordHeader : Located at Offset 0x00 (24B)
    PcapRecordHeader -- WiresharkTlvHeader : Followed by Record Payload
    WiresharkTlvHeader -- MacPduPayload : Followed by Tag 0 + 9B Meta Offset
```

---

## Technical Deep-Dive

### 1. Memory-Mapped File Ingestion (Zero-Copy)
File descriptors are obtained via `open()` and sized using `fstat()`. The memory region is mapped using `mmap()` with `PROT_READ | MAP_PRIVATE`. Complete resource lifecycle is strictly bound to RAII semantics inside `MemoryMappedFile`, executing `munmap()` and `close()` upon destruction.

### 2. Non-Owning Memory Views (`std::span`)
Instead of copying buffers into `std::vector` or raw dynamically allocated arrays, `MemoryMappedFile::file_reader()` returns a `std::span<const uint8_t>`. This gives safe array-indexing semantics, bounds checking, and iterator support without duplicating a single byte in memory.

### 3. Packed Struct Overlays via `reinterpret_cast`
All packet headers use GCC packed attributes (`__attribute__((__packed__))`) to eliminate structure padding and ensure memory alignment matches the binary specification on disk:
```cpp
const PcapRecordHeader* record_header =
    reinterpret_cast<const PcapRecordHeader*>(m_data.data() + current_offset);

const uint8_t* payload_ptr =
    reinterpret_cast<const uint8_t*>(record_header) + sizeof(PcapRecordHeader);
```

### 4. Endianness Conversion & TLV Navigation
Wireshark encapsulates 5G upper layers in Exported PDU TLV blocks encoded in Network Byte Order (Big-Endian). Using C++23 `<bit>` (`std::byteswap`), headers are converted to host byte order on the fly to dynamically navigate variable-length tags until encountering the `0` (End of Options) terminator tag.

### 5. 5G NR MAC PDU Dissection
Once past the 9-byte MAC-NR metadata header, bit masking extracts the 3GPP TS 38.321 header fields:
- **R (Reserved bit)**: `(first_byte & 0b10000000U) >> 7`
- **F (Format bit)**: `(first_byte & 0b01000000U) >> 6`
- **LCID (Logical Channel ID)**: `first_byte & 0b00111111U`

---

## Build & Verification

Requirements:
- C++23 compliant compiler (`g++-13` / `clang++-16` or newer)
- CMake 3.20+

```bash
# Generate build files and compile
mkdir -p build && cd build
cmake ..
make

# Run the parser against the captured PCAP trace
./parser
```

---

## Roadmap
- [x] Phase 1: Zero-copy file mapping (`mmap`) & Global PCAP Header validation
- [x] Phase 2: Record iteration & C++23 byte-swapping
- [x] Phase 3: Wireshark Exported PDU TLV decoding & raw 5G MAC PDU isolation
- [ ] Phase 4: Full 3GPP TS 38.321 MAC subPDU and SDU decoding table implementation

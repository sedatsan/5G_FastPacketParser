# 5G Fast Packet Parser

[![CI/CD](https://github.com/sedatsan/5G_FastPacketParser/actions/workflows/ci.yml/badge.svg)](https://github.com/sedatsan/5G_FastPacketParser/actions)
![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=flat&logo=c%2B%2B)
![CMake](https://img.shields.io/badge/CMake-3.22%2B-064F8C.svg?style=flat&logo=cmake)
![GoogleTest](https://img.shields.io/badge/GoogleTest-v1.14.0-success.svg?style=flat&logo=google)
![GoogleBenchmark](https://img.shields.io/badge/GoogleBenchmark-v1.8.4-orange.svg?style=flat&logo=google)
![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat)

A high-throughput, zero-copy C++23 parser engineered to extract, dissect, and decode 5G NR MAC Protocol Data Units (PDUs) from raw PCAP captures. Designed for ultra-low latency packet inspection and RAN telemetry (simulating a 100MHz 4x2 cell bypassing the OCUDU core network), it leverages POSIX `mmap` ingestion, non-owning `std::span` memory views, packed struct overlays, and C++23 bitwise transformations to achieve multi-million packet-per-second decoding.

---

## Performance & Microbenchmarks

Microbenchmarks are executed using **Google Benchmark** to measure both isolated bitwise header decoding and end-to-end PCAP parsing throughput:

| Benchmark Target | Metric | Throughput / Rate | Memory Allocations |
| :--- | :--- | :--- | :--- |
| **`BM_MacSubheaderDecode`**<br/>(3GPP Subheader Bitwise Extraction) | **~0.65 ns** / op | **~1.41 Billion** subheaders/sec | **0 allocs** (Stack only) |
| **`BM_ZeroCopyPcapParser`**<br/>(End-to-End PCAP + TLV + MAC SDU Ingestion) | **~1.45 µs** / file iteration | **~460–640+ MiB/s**<br/>(~3.10–4.33M packets/sec) | **0 allocs** in parsing loop |

### Why It's Fast
- **Zero Heap Allocations in Hot Path:** All header overlays use compile-time packed structs mapped directly over raw memory pages.
- **Cache-Friendly Ingestion:** POSIX `mmap` (`PROT_READ | MAP_PRIVATE`) allows the Linux kernel page cache to feed non-owning `std::span<const uint8_t>` buffers without kernel-to-userspace `memcpy` copies.
- **Hardware-Accelerated Endianness:** C++23 `std::byteswap` compiles to native single-instruction byte swaps (`bswap` on x86-64).

---

## System Architecture & Zero-Copy Pipeline

The ingestion pipeline maps the binary stream into virtual memory and navigates nested protocol encapsulations with pointer arithmetic:

```mermaid
flowchart TD
    subgraph Disk ["1. Storage Layer"]
        FILE["gnb_mac.pcap<br/>Raw 5G NR PCAP"]
    end

    subgraph Ingestion ["2. Zero-Copy Ingestion Layer (RAII)"]
        MMF["MemoryMappedFile<br/>open() &rarr; fstat() &rarr; mmap()"]
        SPAN["std::span&lt;const uint8_t&gt;<br/>Non-owning contiguous memory view"]
    end

    subgraph Parsing ["3. In-Memory Protocol Dissection (MacParser)"]
        direction TB
        GH["PcapGlobalHeader (24 Bytes)<br/>reinterpret_cast&lt;const PcapGlobalHeader*&gt;(data)<br/>Validates Magic (0xa1b2c3d4) & LinkType (252)"]
        
        subgraph RecordLoop ["Packet Iteration Loop (offset += 16B + captured_len)"]
            RH["PcapRecordHeader (16 Bytes)<br/>reinterpret_cast&lt;const PcapRecordHeader*&gt;(data + offset)"]
            PAYLOAD["Raw Payload Slice<br/>record_ptr + sizeof(PcapRecordHeader)"]
            
            subgraph TLVLoop ["Wireshark Exported PDU Navigation (LinkType 252)"]
                TLV["WiresharkTlvHeader<br/>reinterpret_cast&lt;const WiresharkTlvHeader*&gt;()<br/>std::byteswap(tag), std::byteswap(len)"]
                END_TAG["Tag == 0 (End of Options)<br/>Calculates dynamic TLV offset"]
            end
            
            subgraph SubPDULoop ["3GPP TS 38.321 SubPDU Decoding"]
                SUBHDR["MacSubheader(data)<br/>Bitmask: R, F, LCID & Length"]
                PAYLOAD_STEP["Payload offset jump<br/>offset += header_size + length"]
                PADDING["Padding Check (LCID 63)<br/>Terminate Transport Block"]
            end
        end
    end

    FILE -->|"POSIX fd"| MMF
    MMF -->|"PROT_READ | MAP_PRIVATE"| SPAN
    SPAN -->|"Zero-copy view injection"| GH
    GH -->|"Offset 0x18"| RH
    RH -->|"Pointer arithmetic"| PAYLOAD
    PAYLOAD -->|"Iterate options"| TLV
    TLV -->|"Find Tag 0"| END_TAG
    END_TAG -->|"Offset + 9B meta"| SUBHDR
    SUBHDR --> PAYLOAD_STEP
    SUBHDR --> PADDING
```

---

## Memory Layout & Protocol Mapping

The parser overlays structured C++ representations over mapped pages without serialization overhead:

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
        +uint16_t tag (Big-Endian &rarr; std::byteswap)
        +uint16_t length (Big-Endian &rarr; std::byteswap)
    }

    class MacSubheader {
        +uint8_t m_first_byte (R:1, F:1, LCID:6)
        +uint16_t m_length (8-bit if F=0, 16-bit if F=1)
        +uint8_t m_header_size (1B, 2B, or 3B)
        +get_lcid() uint8_t
        +get_format() uint8_t
        +get_length() uint16_t
        +get_header_size() uint8_t
    }

    PcapGlobalHeader -- PcapRecordHeader : Located at Offset 0x00 (24B)
    PcapRecordHeader -- WiresharkTlvHeader : Followed by Record Payload
    WiresharkTlvHeader -- MacSubheader : Tag 0 + 9B MAC-NR Metadata Offset
```

### 3GPP TS 38.321 Subheader Dissection
Every 5G NR MAC subPDU begins with a subheader parsed dynamically according to the **Format (`F`)** and **Logical Channel ID (`LCID`)** flags:
- **Format 0 (`F = 0`)**: 1-byte length field (`m_header_size = 2` bytes total). Used for payloads $\le 255$ bytes.
- **Format 1 (`F = 1`)**: 2-byte length field (`m_header_size = 3` bytes total, Big-Endian). Used for payloads up to 65,535 bytes.
- **Padding (`LCID = 63`)**: 1-byte subheader with no length field (`m_length = 0`, `m_header_size = 1`), signaling the end of multiplexed subPDUs in the Transport Block.

---

## Unit Testing & Verification

Unit tests are written with **Google Test (GTest)** to rigorously validate protocol edge cases and memory bounds:

```cpp
// Test 1: LCID 4 with Format=0 (8-bit length: 17 bytes)
TEST(MacSubheaderTest, ParseFormat0Subheader);

// Test 2: LCID 32 with Format=1 (16-bit Big-Endian length: 1408 bytes)
TEST(MacSubheaderTest, ParseFormat1Subheader);

// Test 3: Padding Subheader (LCID 63, single byte)
TEST(MacSubheaderTest, ParsePaddingSubheader);
```

Run test suite via CTest:
```bash
ctest --test-dir build --output-on-failure
```

---

## CI/CD Pipeline & Code Quality

The repository includes an automated **GitHub Actions CI/CD** matrix (`.github/workflows/ci.yml`):
- **GCC 14 (Debug + Sanitizers)**: Compiles with `-fsanitize=address,undefined` to guarantee memory safety, buffer bounds, and pointer alignment.
- **Clang 18 (Release)**: Builds with `-O3` and executes microbenchmarks as a sanity and performance regression gate.
- **Static Analysis & Formatting**: Enforced via `.clang-format` and `.clang-tidy` rules adhering to modern C++ Core Guidelines.

---

## Build & Execution

### Prerequisites
- **Compiler**: C++23 compliant (`g++-13+` / `clang++-16+`)
- **Build System**: CMake 3.22+
- **Platform**: Linux / POSIX (uses `mmap`, `munmap`, `fstat`)

### Commands

```bash
# 1. Configure and generate build files
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Build all targets (parser, tests, benchmarks)
cmake --build build -j$(nproc)

# 3. Run the 5G MAC PDU parser against captured PCAP
./build/parser

# 4. Run Google Test suite
./build/parser_tests

# 5. Run Google Benchmark suite
./build/parser_benchmarks
```

---

## Project Status & Roadmap

**Status: Finalized & Production-Ready (v1.0.0)**

- [x] **Phase 1: Zero-Copy Ingestion Engine** — POSIX `mmap` wrapper with strict RAII lifecycle and `PcapGlobalHeader` validation.
- [x] **Phase 2: Packet Record Traversal** — Pointer arithmetic record loop with C++23 `<bit>` (`std::byteswap`) endianness conversion.
- [x] **Phase 3: Wireshark Exported PDU Dissection** — Dynamic TLV option parsing loop to isolate raw 5G MAC PDUs from LinkType 252 wrappers.
- [x] **Phase 4: 3GPP TS 38.321 MAC SubPDU Decoder** — Full subheader decoding supporting Format 0 (8-bit), Format 1 (16-bit), and Padding (LCID 63).
- [x] **Phase 5: Google Test Harness** — Unit test coverage for all subheader bitfield permutations and boundary conditions.
- [x] **Phase 6: Google Benchmark Suite** — Microbenchmarks for bitwise decoding latency and multi-gigabit parsing throughput.
- [x] **Phase 7: Multi-Compiler CI/CD** — Automated GitHub Actions workflow with AddressSanitizer, UBSan, and Clang 18 Release builds.

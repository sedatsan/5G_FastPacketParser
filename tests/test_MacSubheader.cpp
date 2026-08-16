#include "PcapStructs.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

// Test 1: LCID 4 with Format=0 (8-bit length: 17 bytes)
TEST(MacSubheaderTest, ParseFormat0Subheader) {
  // 0x04 = 0b00000100 (R=0, F=0, LCID=4)
  // 0x11 = 17 bytes length
  const std::vector<uint8_t> raw_bytes = {0x04, 0x11};

  MacSubheader subheader(raw_bytes.data());

  EXPECT_EQ(subheader.get_lcid(), 4);
  EXPECT_EQ(subheader.get_format(), 0);
  EXPECT_EQ(subheader.get_length(), 17);
  EXPECT_EQ(subheader.get_header_size(), 2);
}

// Test 2: LCID 32 with Format=1 (16-bit length: 1408 bytes = 0x0580)
TEST(MacSubheaderTest, ParseFormat1Subheader) {
  // 0x60 = 0b01100000 (R=0, F=1, LCID=32)
  // 0x05, 0x80 = 1408 bytes length (Big-Endian)
  const std::vector<uint8_t> raw_bytes = {0x60, 0x05, 0x80};

  MacSubheader subheader(raw_bytes.data());

  EXPECT_EQ(subheader.get_lcid(), 32);
  EXPECT_EQ(subheader.get_format(), 1);
  EXPECT_EQ(subheader.get_length(), 1408);
  EXPECT_EQ(subheader.get_header_size(), 3);
}

// Test 3: Padding Subheader (LCID 63)
TEST(MacSubheaderTest, ParsePaddingSubheader) {
  // 0x3F = 0b00111111 (R=0, F=0, LCID=63)
  const std::vector<uint8_t> raw_bytes = {0x3F};

  MacSubheader subheader(raw_bytes.data());

  EXPECT_EQ(subheader.get_lcid(), 63);
  EXPECT_EQ(subheader.get_length(), 0);
  EXPECT_EQ(subheader.get_header_size(), 1);
}

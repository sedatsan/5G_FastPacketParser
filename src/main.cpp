// main.cpp
#include "MacParser.hpp"
#include "MemoryMappedFile.hpp"

int main() {
  // map the file to memory for zero copy reading
  MemoryMappedFile my_file("../gnb_mac.pcap");
  auto memory_span = my_file.file_reader();

  // Turn on the parser
  MacParser parser(memory_span);
  parser.parse();

  return 0;
}

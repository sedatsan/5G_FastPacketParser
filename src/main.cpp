#include <iomanip>
#include <iostream>

#include "../include/MemoryMappedFile.hpp"
#include "../include/PcapStructs.hpp"

int main() {
    MemoryMappedFile my_file("../gnb_mac.pcap");
    auto memory_span = my_file.file_reader();

    if (memory_span.size() < sizeof(PcapGlobalHeader)) {
        throw std::runtime_error("File is too small.");
    }

    const PcapGlobalHeader* header = reinterpret_cast<const PcapGlobalHeader*>(memory_span.data());

    std::cout << "Magic Number: 0x" << std::hex << header->magic_number << std::endl;

    return 0;
}

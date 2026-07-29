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

    size_t current_offset = sizeof(PcapGlobalHeader);

    int packet_count = 0;

    while (current_offset < memory_span.size()) {
        const PcapRecordHeader* record_header =
            reinterpret_cast<const PcapRecordHeader*>(memory_span.data() + current_offset);
        current_offset += sizeof(PcapRecordHeader) + record_header->captured_len;
        packet_count++;
    }

    std::cout << "Packet Count: " << packet_count << std::endl;

    return 0;
}

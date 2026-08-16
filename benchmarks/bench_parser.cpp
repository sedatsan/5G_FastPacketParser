#include "MacParser.hpp"
#include "MemoryMappedFile.hpp"
#include "PcapStructs.hpp"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <iostream>
#include <vector>

// Benchmark 1: Subheader Bitwise Decoding Speed
static void BM_MacSubheaderDecode(benchmark::State &state) {
  const std::vector<uint8_t> raw_subpdu = {0x60, 0x05, 0x80};

  for (auto _ : state) {
    MacSubheader subheader(raw_subpdu.data());
    benchmark::DoNotOptimize(subheader.get_lcid());
    benchmark::DoNotOptimize(subheader.get_length());
    benchmark::DoNotOptimize(subheader.get_format());
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MacSubheaderDecode);

// Benchmark 2: End-to-End Zero-Copy PCAP Parsing Throughput
static void BM_ZeroCopyPcapParser(benchmark::State &state) {
  // Search for PCAP across common working directories
  const std::vector<std::string> search_paths = {
      "gnb_mac.pcap", "../gnb_mac.pcap", "../../gnb_mac.pcap",
      "data/gnb_mac.pcap", "../data/gnb_mac.pcap"};

  std::string pcap_path;
  for (const auto &path : search_paths) {
    if (std::filesystem::exists(path)) {
      pcap_path = path;
      break;
    }
  }

  if (pcap_path.empty()) {
    state.SkipWithError("gnb_mac.pcap file not found");
    return;
  }

  MemoryMappedFile pcap_file(pcap_path);
  auto span = pcap_file.file_reader();
  MacParser parser(span);

  auto *old_buf = std::cout.rdbuf(nullptr); // Mute std::cout

  size_t total_bytes = 0;
  for (auto _ : state) {
    parser.parse();
    total_bytes += span.size();
  }

  std::cout.rdbuf(old_buf); // Restore std::cout

  state.SetBytesProcessed(total_bytes);
  state.SetItemsProcessed(state.iterations() * 5);
}
BENCHMARK(BM_ZeroCopyPcapParser);

int main(int argc, char **argv) {
  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
    return 1;
  }
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}

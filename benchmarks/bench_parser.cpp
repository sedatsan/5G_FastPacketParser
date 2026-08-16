#include "MacParser.hpp"
#include "MemoryMappedFile.hpp"
#include "PcapStructs.hpp"
#include <benchmark/benchmark.h>
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
  MemoryMappedFile pcap_file("../gnb_mac.pcap");
  auto span = pcap_file.file_reader();
  MacParser parser(span);

  // Mute std::cout during hot loop benchmarking
  std::cout.setstate(std::ios_base::failbit);

  size_t total_bytes = 0;
  for (auto _ : state) {
    parser.parse();
    total_bytes += span.size();
  }

  std::cout.clear(); // Restore std::cout

  state.SetBytesProcessed(total_bytes);
  state.SetItemsProcessed(state.iterations() * 5); // 5 packets per iteration
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

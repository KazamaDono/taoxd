// build: clang++ -g -O1 -fsanitize=fuzzer,address,undefined \
//        fdp_harness.cc target.c -o fdp_fuzz
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <fuzzer/FuzzedDataProvider.h>

extern "C" int parse_record(uint8_t version,
                            const char *name, size_t name_len,
                            const uint8_t *payload, size_t payload_len);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    FuzzedDataProvider fdp(data, size);                          // ❶
    uint8_t version = fdp.ConsumeIntegralInRange<uint8_t>(1, 4); // ❷
    std::string name = fdp.ConsumeRandomLengthString(64);        // ❸
    std::vector<uint8_t> payload = fdp.ConsumeRemainingBytes<uint8_t>();
    parse_record(version, name.data(), name.size(),
                 payload.data(), payload.size());                // ❹
    return 0;
}

// build: clang++ -g -O1 -fsanitize=fuzzer,address json_diff.cc \
//        -o json_diff -ljsoncpp -lyyjson
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <json/json.h>          // jsoncpp
#include <yyjson.h>             // yyjson

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 64 * 1024) return 0;
    std::string s(reinterpret_cast<const char *>(data), size);

    Json::Value a;
    Json::CharReaderBuilder rb;
    std::string err;
    auto reader = std::unique_ptr<Json::CharReader>(rb.newCharReader());
    bool ok_a = reader->parse(s.data(), s.data() + s.size(), &a, &err);   // ❶

    yyjson_doc *doc = yyjson_read(s.data(), s.size(), 0);
    bool ok_b = (doc != nullptr);                                         // ❷

    if (ok_a != ok_b) __builtin_trap();                                   // ❸

    if (ok_a && ok_b) {                                                   // ❹
        std::string norm_a = Json::FastWriter().write(a);
        size_t n = 0;
        char *norm_b = yyjson_write(doc, YYJSON_WRITE_NOFLAG, &n);
        std::string sb(norm_b ? norm_b : "", n);
        free(norm_b);
        // Trim jsoncpp's trailing newline before comparing.
        if (!norm_a.empty() && norm_a.back() == '\n') norm_a.pop_back();
        if (norm_a != sb) __builtin_trap();                               // ❺
    }
    if (doc) yyjson_doc_free(doc);
    return 0;
}

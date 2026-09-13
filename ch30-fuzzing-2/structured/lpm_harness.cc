// ch30-fuzzing-2/structured/lpm_harness.cc
//
// libprotobuf-mutator harness that mutates a typed `ch30::Config` message
// (schema in config.proto), emits it as textual config, and hands the bytes
// to `parse_config`. See lst-lpm-harness in the chapter.
//
// build (inside the lab image; libprotobuf-mutator-dev + protobuf-compiler):
//     protoc --cpp_out=. config.proto
//     clang++ -g -O1 -std=c++17 -fsanitize=fuzzer,address \
//         lpm_harness.cc parse_config.c config.pb.cc \
//         -I. -I/usr/include \
//         -lprotobuf-mutator-libfuzzer -lprotobuf-mutator -lprotobuf \
//         -o lpm_fuzz

#include <cstddef>
#include <cstdint>
#include <string>

#include "src/libfuzzer/libfuzzer_macro.h"
#include "config.pb.h"

extern "C" int parse_config(const char *text, size_t len);

/* Deterministic, total emitter: every field of the schema is written into
 * a textual form the target parser accepts at the lexical level. Keys are
 * escaped so a mutator-produced weird key still round-trips through the
 * lexer instead of being rejected — see the tip in the chapter. */
static void emit_string(const std::string &s, std::string &out) {
    out.push_back('"');
    for (char c : s) {
        if (c == '\\' || c == '"') out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
}

static void emit_value(const ch30::Value &v, std::string &out) {
    switch (v.v_case()) {
        case ch30::Value::kI: out += std::to_string(v.i()); break;
        case ch30::Value::kF: out += std::to_string(v.f()); break;
        case ch30::Value::kS: emit_string(v.s(), out);      break;
        case ch30::Value::kB: out += v.b() ? "true" : "false"; break;
        default:              out += "0"; break;
    }
}

static void emit_kv(const ch30::KV &kv, std::string &out) {
    emit_string(kv.key(), out);
    out += " = ";
    emit_value(kv.val(), out);
    out += ";\n";
}

static void emit_group(const ch30::Group &g, std::string &out) {
    out += "group ";
    emit_string(g.name(), out);
    out += " {\n";
    for (const auto &kv : g.kvs())  emit_kv(kv, out);
    for (const auto &sg : g.sub())  emit_group(sg, out);
    out += "}\n";
}

static void emit(const ch30::Config &c, std::string &out) {
    out += "version = " + std::to_string(c.version()) + ";\n";
    for (const auto &kv : c.top())    emit_kv(kv, out);
    for (const auto &g  : c.groups()) emit_group(g, out);
}

DEFINE_PROTO_FUZZER(const ch30::Config &cfg) {               // ❶
    if (cfg.version() < 1 || cfg.version() > 3) return;      // ❷ prune early
    std::string text;
    emit(cfg, text);                                         // ❸ typed -> bytes
    parse_config(text.data(), text.size());                  // ❹
}

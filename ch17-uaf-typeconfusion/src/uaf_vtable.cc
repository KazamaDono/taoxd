struct Session {                                  // ❶
    virtual void greet() { printf("hello, user %u (%s)\n", uid, note); }
    virtual ~Session() = default;
    unsigned uid  = 1000;
    char     note[24] = "guest";
};

static Session *g_sess = nullptr;
static std::vector<std::string> g_holder;         // keeps reclaim buffers alive

static void cmd_del() { delete g_sess; puts("deleted"); } // ❷ pointer NOT nulled
static void cmd_use() { if (g_sess) g_sess->greet(); }    // ❸ virtual call on freed

static void cmd_alloc(size_t len, const std::string &raw) {
    std::string s; s.resize(len);                 // ❹ heap buffer, sized to bin
    memcpy(s.data(), raw.data(), std::min(len, raw.size()));
    g_holder.push_back(std::move(s));
    printf("alloc %zu bytes @ %p\n", len, (void*)g_holder.back().data());
}

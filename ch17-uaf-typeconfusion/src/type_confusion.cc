enum class Kind : uint32_t { Circle = 1, Command = 2 };

struct Node {                                              // ❶ 8-byte tag header
    Kind kind;
    uint32_t _pad;
    virtual ~Node() = default;
};

struct Circle  : Node { double radius; };                  // fields: r
struct Command : Node { void (*fn)(const char*); char arg[64]; }; // fields: fp, arg

// Parses one frame from the wire: [u32 kind][u32 pad][payload...].
// BUG: the constructor picks the type from `kind`, but `handle_area` below
// looks at a *different* copy of `kind` that the attacker can desynchronize.
static Node *decode(const uint8_t *buf, size_t n) {        // ❷
    if (n < 8) return nullptr;
    Kind k = static_cast<Kind>(*reinterpret_cast<const uint32_t *>(buf));
    if (k == Kind::Circle && n >= 16) {
        auto *c = new Circle(); c->kind = k;
        memcpy(&c->radius, buf + 8, sizeof(double));
        return c;
    }
    if (k == Kind::Command && n >= 8 + sizeof(void*)) {
        auto *cmd = new Command(); cmd->kind = k;
        memcpy(&cmd->fn, buf + 8, sizeof(void*));
        strncpy(cmd->arg, (const char *)buf + 8 + sizeof(void*), 63);
        return cmd;
    }
    return nullptr;
}

// One code path — a fast area query — trusts the caller's tag byte, not the object's.
static void handle_area(Node *n, Kind caller_tag) {         // ❸
    if (caller_tag != Kind::Circle) return;                 // ❹ wrong witness
    auto *c = static_cast<Circle *>(n);                     // ❺ unchecked downcast
    printf("area = %.3f\n", 3.14159265 * c->radius * c->radius);
}

# full source in ch30-fuzzing-2/grammar/generate.py
import random, sys
MAX_DEPTH = 6                                          # ❶ recursion budget

GRAMMAR = {
    "prog":   [("stmt_list", 1.0)],
    "stmt_list": [("stmt", 0.55), ("stmt SEMI stmt_list", 0.45)],
    "stmt":   [("assign", 0.5), ("ifstmt", 0.25), ("group", 0.25)],
    "assign": [("IDENT EQ expr", 1.0)],
    "ifstmt": [("IF LP expr RP LB stmt_list RB", 1.0)],
    "group":  [("GROUP IDENT LB stmt_list RB", 1.0)],
    "expr":   [("INT", 0.4), ("IDENT", 0.3),
               ("expr PLUS expr", 0.15), ("expr LT expr", 0.15)],
}
TOKENS = {                                             # ❷ terminals
    "SEMI":";", "EQ":"=", "IF":"if", "GROUP":"group",
    "LP":"(", "RP":")", "LB":"{", "RB":"}",
    "PLUS":"+", "LT":"<",
}
IDS = ["x","y","z","count","name","flag","ratio"]

def gen(sym, depth):
    if sym in TOKENS: return TOKENS[sym]
    if sym == "IDENT": return random.choice(IDS)
    if sym == "INT":   return str(random.randint(-9, 999))
    prods = GRAMMAR[sym]
    if depth >= MAX_DEPTH:                             # ❸ collapse to terminal
        prods = [p for p in prods if " " not in p[0]] or prods
    weights = [w for _, w in prods]
    body    = random.choices([b for b, _ in prods], weights=weights)[0]
    return " ".join(gen(s, depth + 1) for s in body.split())

if __name__ == "__main__":
    random.seed(int(sys.argv[1]) if len(sys.argv) > 1 else 0)
    print(gen("prog", 0))

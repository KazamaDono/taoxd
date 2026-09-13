#!/usr/bin/env python3
"""Rank matched functions by how much they changed, using a Diaphora export.
Prints the short-list a human should read, most-changed first."""
import sqlite3, sys

def diff(db1_path, db2_path):
    con = sqlite3.connect(db1_path)                          # ❶
    con.execute("ATTACH DATABASE ? AS post", (db2_path,))
    rows = con.execute("""
        SELECT a.name, a.address, b.address,
               a.nodes, a.edges, b.nodes, b.edges,
               a.md_index, b.md_index,
               a.pseudocode_primes, b.pseudocode_primes
        FROM main.functions   AS a
        JOIN post.functions   AS b
          ON a.name = b.name                                 -- ❷
        WHERE a.md_index != b.md_index                       -- ❸
           OR a.pseudocode_primes != b.pseudocode_primes
    """).fetchall()
    for r in rows:
        name = r[0]
        d_nodes = r[5] - r[3]
        d_edges = r[6] - r[4]
        print(f"{name:32s}  v1={r[1]}  v2={r[2]}  "
              f"nodes {r[3]}->{r[5]} ({d_nodes:+d})  "
              f"edges {r[4]}->{r[6]} ({d_edges:+d})")        # ❹

if __name__ == "__main__":
    diff(sys.argv[1], sys.argv[2])

#!/usr/bin/env python3
"""Group crash inputs by ASan stack signature. Usage: triage.py FUZZER CRASHDIR"""
import hashlib, os, re, subprocess, sys, collections

if len(sys.argv) != 3:
    sys.exit("usage: triage.py FUZZER CRASHDIR")

FUZZER, CRASHDIR = sys.argv[1], sys.argv[2]
FRAME = re.compile(rb'#\d+ 0x[0-9a-f]+ in (\S+)')                       # ❶

groups = collections.defaultdict(list)
for name in sorted(os.listdir(CRASHDIR)):
    path = os.path.join(CRASHDIR, name)
    if not os.path.isfile(path) or name.startswith('README'):
        continue
    env = {**os.environ, 'ASAN_OPTIONS':
           'abort_on_error=0:exitcode=42:symbolize=1'}                  # ❷
    try:
        r = subprocess.run([FUZZER, path], env=env,
                           stdout=subprocess.DEVNULL,
                           stderr=subprocess.PIPE, timeout=30)
    except subprocess.TimeoutExpired:
        continue
    top = FRAME.findall(r.stderr)[:3]                                   # ❸
    sig = hashlib.sha1(b'|'.join(top)).hexdigest()[:12] if top else 'nofault'
    groups[sig].append(name)

for sig, names in sorted(groups.items(), key=lambda kv: -len(kv[1])):
    print(f'{sig}  {len(names):4d}  first={names[0]}')                  # ❹

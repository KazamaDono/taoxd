# Package marker so `python3 -m toy_ipc.driver` and `from toy_ipc import ...`
# resolve when the ch19 directory is on sys.path. The dash in the on-disk
# directory name (`toy-ipc/`) is not a valid Python identifier, so callers
# import via the `toy_ipc` alias installed by the driver / test harness.

# Simplified Mojo-shaped broker. Renderer connects over an inherited
# socketpair; every request is a length-prefixed JSON message.
import json, os, socket, struct, sys

ALLOWED_PREFIXES = ("/tmp/toy-ipc-lab/",)                        # ❶

def recv_msg(sock):
    hdr = _recvall(sock, 4)
    if not hdr: return None
    (n,) = struct.unpack("!I", hdr)
    if n > 65536:                                                # ❷
        raise ValueError("oversize message")
    return json.loads(_recvall(sock, n))

def send_msg(sock, obj, fds=()):
    data = json.dumps(obj).encode()
    hdr = struct.pack("!I", len(data))
    if fds:
        socket.send_fds(sock, [hdr + data], list(fds))           # ❸
    else:
        sock.sendall(hdr + data)

def handle(req, sock):
    if req.get("op") != "open":
        return send_msg(sock, {"error": "unknown op"})
    path = os.path.realpath(req.get("path", ""))                 # ❹
    if not any(path.startswith(p) for p in ALLOWED_PREFIXES):    # ❺
        return send_msg(sock, {"error": "policy: path denied"})
    flags = int(req.get("flags", os.O_RDONLY)) & (os.O_RDONLY | os.O_WRONLY)
    try:
        fd = os.open(path, flags)                                # ❻
    except OSError as e:
        return send_msg(sock, {"error": str(e)})
    send_msg(sock, {"ok": True, "path": path}, fds=(fd,))
    os.close(fd)

def main():
    sock = socket.fromfd(int(sys.argv[1]), socket.AF_UNIX, socket.SOCK_STREAM)
    while True:
        req = recv_msg(sock)
        if req is None: return
        handle(req, sock)


# --- helpers below the fold (not shown in the book listing) -----------------
# _recvall and the __main__ guard live outside the excerpt in {{ref:lst-broker}}
# because they add nothing to the trust discussion; the excerpt above is what
# the chapter dissects at ❶–❻.

def _recvall(sock, n):
    """Read exactly n bytes from sock; return None on clean EOF, raise on
    truncated stream. Kept out of the book excerpt to stay under a screenful."""
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            return None if not buf else (_ for _ in ()).throw(
                ConnectionError("short read from renderer"))
        buf += chunk
    return buf


if __name__ == "__main__":
    main()

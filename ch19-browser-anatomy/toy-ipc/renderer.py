"""Sandbox-shaped renderer stub.

Reflects, in one file, what a compromised renderer has to work with: an
inherited socket to the broker and no other privileged capability. It cannot
call `open` itself; every file it wants must be requested over the pipe.

The three scripted requests live in `driver.py` so this file can also be
imported as a plain client library for the CI test.
"""
import json, os, socket, struct, sys


MAX_MSG = 65536


def _recvall(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            if not buf:
                return None
            raise ConnectionError("short read from broker")
        buf += chunk
    return buf


def send_request(sock, req):
    """Serialize req as a length-prefixed JSON message; write to the pipe."""
    data = json.dumps(req).encode()
    sock.sendall(struct.pack("!I", len(data)) + data)


def recv_response(sock):
    """Read one length-prefixed JSON reply plus any file descriptors the
    broker attached via SCM_RIGHTS. Returns (obj, [fd, ...]).

    The header MUST be read with recv_fds too: on a SOCK_STREAM socket,
    ancillary data is delivered with whichever recv() consumes the byte
    it was attached to. A plain recv() of the 4-byte header would silently
    drop the SCM_RIGHTS if the broker attached fds to the same send."""
    # Read header + up to MAX_MSG body in one recvmsg; broker always sends
    # header and body in a single send/send_fds call so this suffices.
    buf, fds, _flags, _addr = socket.recv_fds(sock, 4 + MAX_MSG, 4)
    if not buf:
        return None, []
    if len(buf) < 4:
        raise ConnectionError("short header from broker")
    (n,) = struct.unpack("!I", buf[:4])
    if n > MAX_MSG:
        raise ValueError("oversize reply from broker")
    body = buf[4:4 + n]
    # If the reply was chunked below the header line, top up with plain
    # recv (the ancillary has already been harvested above).
    while len(body) < n:
        rest = _recvall(sock, n - len(body))
        if rest is None:
            raise ConnectionError("truncated reply from broker")
        body += rest
    return json.loads(body), list(fds)


def open_via_broker(sock, path, flags=os.O_RDONLY):
    """Convenience: issue one Open request and return (response, fd_or_None)."""
    send_request(sock, {"op": "open", "path": path, "flags": flags})
    obj, fds = recv_response(sock)
    return obj, (fds[0] if fds else None)


def main():
    """Renderer standalone entry point: takes an inherited socket fd on argv
    and a single request path, prints the JSON response. Used by the driver
    only when it wants to demonstrate the renderer as a separate process."""
    sock = socket.fromfd(int(sys.argv[1]), socket.AF_UNIX, socket.SOCK_STREAM)
    path = sys.argv[2]
    resp, fd = open_via_broker(sock, path)
    print(json.dumps(resp))
    if fd is not None:
        os.close(fd)


if __name__ == "__main__":
    main()

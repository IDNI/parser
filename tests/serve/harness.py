#!/usr/bin/env python3
# Process and socket helpers for the tgf serve tests. A reader thread per
# stream delivers whole lines with a timeout, so no select() is needed and
# the helpers work on every platform.

import json
import socket
import subprocess
import threading
import time


class HarnessError(Exception):
    pass


class Process:
    """A child process whose stdout and stderr are read line by line."""

    def __init__(self, command):
        self.proc = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        self._cv = threading.Condition()
        self._lines = {1: [], 2: []}
        self._eof = {1: False, 2: False}
        self._threads = []
        for stream, key in ((self.proc.stdout, 1), (self.proc.stderr, 2)):
            thread = threading.Thread(
                target=self._pump, args=(stream, key), daemon=True)
            thread.start()
            self._threads.append(thread)

    def _pump(self, stream, key):
        try:
            for raw in iter(stream.readline, b""):
                line = raw.decode("utf-8", "replace").rstrip("\r\n")
                with self._cv:
                    self._lines[key].append(line)
                    self._cv.notify_all()
        finally:
            with self._cv:
                self._eof[key] = True
                self._cv.notify_all()

    def read_line(self, key, timeout):
        """One line from stream @p key (1 stdout, 2 stderr), or None."""
        deadline = time.monotonic() + timeout
        with self._cv:
            while True:
                if self._lines[key]:
                    return self._lines[key].pop(0)
                if self._eof[key]:
                    return None
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    return None
                self._cv.wait(remaining)

    def write_line(self, line):
        self.proc.stdin.write((line + "\n").encode("utf-8"))
        self.proc.stdin.flush()

    def stop(self):
        if self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=10)


class Client:
    """One TCP client with a timeout on every socket call."""

    def __init__(self, port, timeout=30):
        self.sock = socket.create_connection(("127.0.0.1", port),
                                             timeout=timeout)
        self.sock.settimeout(timeout)
        self.buf = b""

    def send(self, line):
        self.sock.sendall((line + "\n").encode("utf-8"))

    def read_line(self):
        while b"\n" not in self.buf:
            chunk = self.sock.recv(65536)
            if not chunk:
                raise HarnessError("the connection closed before a line")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return json.loads(line.decode("utf-8"))

    def closed(self):
        if self.buf:
            return False
        try:
            return self.sock.recv(1) == b""
        except socket.timeout:
            return False

    def close(self):
        self.sock.close()


def read_listening(proc, timeout=30):
    """The port from the server's first stdout line."""
    line = proc.read_line(1, timeout)
    if line is None:
        raise HarnessError("the server wrote no listening line")
    try:
        value = json.loads(line)
    except ValueError as exc:
        raise HarnessError("the first stdout line is not JSON: %s" % exc)
    if "listening" not in value:
        raise HarnessError("the first stdout line is not a listening line")
    return value["listening"]

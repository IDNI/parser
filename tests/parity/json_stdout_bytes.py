#!/usr/bin/env python3
"""Check the real stdout bytes of the JSON API.

Every JSON message must end with one LF and carry no CR, on every
platform. An in-process capture of std::cout cannot see the line-ending
translation a text-mode stdout does, so this runs the program and reads
the bytes it wrote.

Usage:
    json_stdout_bytes.py <grammar> <command...>

<command...> is the tgf program, for example ./build/release/tgf, or
wine ./build/release-mingw/tgf.exe.
"""

import json
import os
import subprocess
import sys
import tempfile

REQUESTS = b'{"id":1,"cmd":"version"}\n{"id":2,"cmd":"quit"}\n'


def fail(message, out=None, err=None):
    sys.stderr.write("json_stdout_bytes: FAIL %s\n" % message)
    if out is not None:
        sys.stderr.write("stdout: %r\n" % out[:400])
    if err is not None:
        sys.stderr.write(
            "stderr: %s\n" % err.decode("utf-8", "replace")[-2000:])
    sys.exit(1)


def run(command, stdin_bytes):
    fd, name = tempfile.mkstemp(prefix="json_stdout_bytes")
    try:
        os.write(fd, stdin_bytes)
        os.close(fd)
        env = dict(os.environ)
        # Wine startup chatter must not reach the byte counts.
        env["WINEDEBUG"] = "-all"
        with open(name, "rb") as stdin:
            return subprocess.run(command, stdin=stdin,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, env=env)
    finally:
        os.unlink(name)


def check(name, proc):
    out, err = proc.stdout, proc.stderr
    if proc.returncode != 0:
        fail("%s exited with %d" % (name, proc.returncode), out, err)
    if not out:
        fail("%s wrote no stdout" % name, out, err)
    if b"\r" in out:
        fail("%s wrote a CR" % name, out, err)
    if not out.endswith(b"\n"):
        fail("%s did not end with LF" % name, out, err)
    lines = out.split(b"\n")[:-1]
    for line in lines:
        if not line:
            fail("%s wrote an empty line" % name, out, err)
        try:
            json.loads(line.decode("utf-8"))
        except ValueError as e:
            fail("%s wrote a line that is not JSON (%s)" % (name, e),
                 out, err)
    print("json_stdout_bytes: %s OK, %d line(s), no CR" % (name, len(lines)))


def main():
    if len(sys.argv) < 3:
        sys.stderr.write(
            "usage: json_stdout_bytes.py <grammar> <command...>\n")
        return 2
    grammar = sys.argv[1]
    program = sys.argv[2:]
    check("repl --json",
          run(program + [grammar, "repl", "--json"], REQUESTS))
    check("parse --json",
          run(program + [grammar, "parse", "--json", "-e", "123"], b""))
    print("json_stdout_bytes: OK (%s)" % " ".join(program))
    return 0


if __name__ == "__main__":
    sys.exit(main())

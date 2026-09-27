#!/usr/bin/env python3
# Smoke test for the tgf TCP server: start it on a free port, read the
# port from its stdout, run a short session and check the responses. The
# process and socket helpers come from harness.py.

import sys

from harness import Client, HarnessError, Process, read_listening


def fail(message):
    print("tgf_serve_smoke: " + message, file=sys.stderr)
    sys.exit(1)


def main():
    if len(sys.argv) < 3:
        fail("usage: tgf_serve_smoke.py <grammar> <command...>")
    grammar = sys.argv[1]
    program = sys.argv[2:]

    proc = Process(program + [grammar, "serve", "--port", "0", "--no-log"])
    try:
        port = read_listening(proc)
        c = Client(port)
        try:
            c.send('{"cmd":"new"}')
            hello = c.read_line()
            if "hello" not in hello or "session" not in hello["hello"]:
                fail("the hello line carries no session id")
            if len(hello["hello"]["session"]) != 32:
                fail("the session id is not 128 bits in hex")

            c.send('{"id":1,"cmd":"version"}')
            version = c.read_line()
            if version.get("status") != "ok" or "version" not in version.get(
                    "result", {}):
                fail("the version request failed")

            c.send('{"id":2,"cmd":"quit"}')
            quit_line = c.read_line()
            if quit_line.get("status") != "quit" or quit_line.get(
                    "cmd") != "quit":
                fail("quit was not answered with status quit")
            if not c.closed():
                fail("quit did not close the connection")
        finally:
            c.close()
    except HarnessError as exc:
        fail(str(exc))
    finally:
        proc.stop()

    print("tgf_serve_smoke: ok")


if __name__ == "__main__":
    main()

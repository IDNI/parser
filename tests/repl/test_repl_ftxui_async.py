#!/usr/bin/env python3
"""Exercise asynchronous evaluation in the FTXUI REPL."""

import argparse
import sys
import time

import pexpect

from test_repl_ftxui import ReplTester


def wait_for(tester, predicate, timeout=10.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        tester._drain()
        if predicate(tester.render()):
            return True
        time.sleep(0.02)
    return False


def fail(message, tester):
    print(message, file=sys.stderr)
    print("\n".join(tester.render()), file=sys.stderr)
    raise SystemExit(1)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("fixture_bin")
    args = parser.parse_args()

    tester = ReplTester(args.fixture_bin, "unused", prompt=b"test>")
    try:
        if not tester.wait_ready():
            fail("initial prompt did not appear", tester)

        command = "slow"
        tester._drain()
        tester.send(command)
        if not wait_for(tester,
                lambda lines: any("evaluating" in line for line in lines)):
            fail("evaluation spinner did not appear", tester)
        if not any("evaluating" in line for line in tester.render()):
            fail("spinner stopped before evaluation completed", tester)
        if not any(command in line for line in tester.render()):
            fail("submitted command disappeared during evaluation", tester)

        tester.send_text("ignored while evaluating")
        if not wait_for(tester, lambda lines: not any(
                "evaluating" in line for line in lines)):
            fail("evaluation did not finish", tester)
        tester._stabilize(0.2)
        if "ignored while evaluating" in tester.get_input_line():
            fail("input changed during evaluation", tester)

        tester.send_text("\x1b[A")
        tester._stabilize(0.1)
        if command not in tester.get_input_line():
            fail("completed evaluation was not stored in history", tester)

        tester.send_ctrl("u")
        tester.send("incomplete")
        if not wait_for(tester, lambda lines: not any(
                "evaluating" in line for line in lines)):
            fail("incomplete evaluation did not finish", tester)
        tester._stabilize(0.1)
        if not any("incomplete" in line for line in tester.render()):
            fail("incomplete input was not restored", tester)
        tester.send_ctrl("c")
        tester.send("quit")
        tester.child.expect_exact(b"Quit.", timeout=10)
        tester.child.expect(pexpect.EOF, timeout=10)
    finally:
        tester.close()


if __name__ == "__main__":
    main()

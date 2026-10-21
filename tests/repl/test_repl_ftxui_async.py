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


def check_plain_stop(fixture_bin):
    """Ctrl-C stops a loop of the plain REPL, which goes on."""
    child = pexpect.spawn(fixture_bin, ["--plain"], timeout=10,
        dimensions=(24, 80))
    try:
        child.expect_exact(b"test> ")
        child.sendline(b"loop")
        child.expect_exact(b"loop start, stop: no")
        child.expect_exact(b"loop step 1")
        child.sendcontrol("c")
        child.expect_exact(b"loop stopped", timeout=3)
        child.expect_exact(b"test> ")
        child.sendline(b"check")
        child.expect_exact(b"stop: no")
        child.sendline(b"quit")
        child.expect_exact(b"Quit.")
        child.expect(pexpect.EOF)
    except (pexpect.TIMEOUT, pexpect.EOF) as error:
        print("plain REPL: " + type(error).__name__, file=sys.stderr)
        print(child.before.decode(errors="replace"), file=sys.stderr)
        raise SystemExit(1)
    finally:
        if child.isalive():
            child.kill(9)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("fixture_bin")
    args = parser.parse_args()

    check_plain_stop(args.fixture_bin)

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
        if not any(line.lstrip().startswith("...>")
                for line in tester.render()):
            fail("the continuation line shows no continuation prompt", tester)
        tester.send_ctrl("c")
        tester.send("loop")
        # the output of a command shows only at its end, so the spinner
        # tells that the loop runs
        if not wait_for(tester,
                lambda lines: any("evaluating" in line for line in lines)):
            fail("the loop did not start", tester)
        tester._stabilize(0.3)
        tester.send_ctrl("c")
        if not wait_for(tester, lambda lines: any("loop stopped" in line
                for line in lines), timeout=3.0):
            fail("Ctrl-C did not stop the loop", tester)
        if not any("loop start, stop: no" in line
                for line in tester.render()):
            fail("the loop started with a stop request", tester)
        if not wait_for(tester, lambda lines: not any(
                "evaluating" in line for line in lines)):
            fail("the stopped loop did not give the prompt back", tester)
        tester._stabilize(0.2)
        if not tester.child.isalive():
            fail("Ctrl-C during the loop ended the REPL", tester)
        tester.send("check")
        if not wait_for(tester, lambda lines: any("stop: no" == line.strip()
                for line in lines)):
            fail("the stop request stayed for the next command", tester)
        tester.send("quit")
        tester.child.expect_exact(b"Quit.", timeout=10)
        tester.child.expect(pexpect.EOF, timeout=10)
    finally:
        tester.close()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Test key bindings in the TGF REPL (legacy and FTXUI modes).

Verification reads the rendered terminal screen via pyte (reusing
ReplTester from test_repl_ftxui), so it stays correct under FTXUI's
differential redraw - where killed text is erased in place and the
surviving text is never re-emitted on the byte stream.

Usage:
  ./test_repl_keybindings.py <tgf_binary> <grammar_file>
"""

import sys
import time
import argparse

from test_repl_ftxui import ReplTester

# tau.tgf is large; allow a generous initial load before the prompt appears.
LOAD_TIMEOUT = 180


class KeyTester:
    """Drives one REPL session and reads its buffer via the pyte screen."""

    def __init__(self, tgf_bin, grammar_file, ftxui):
        self.ftxui = ftxui
        self.t = ReplTester(tgf_bin, grammar_file,
                            timeout=LOAD_TIMEOUT, legacy=not ftxui)

    def ready(self):
        return self.t.wait_ready(timeout=LOAD_TIMEOUT)

    def clear(self):
        """Empty the input buffer without evaluating it."""
        self.t.send_ctrl("e")          # cursor to end
        self.t.send_text("\x7f" * 40)  # backspace the whole line
        self.t._stabilize(0.25)

    def buffer(self):
        """Current input line as shown on screen (prompt + typed text)."""
        self.t._stabilize(0.3)
        return self.t.get_input_line()

    def left(self, n):
        for _ in range(n):
            self.t.child.send(b"\x1b[D")

    def close(self):
        self.t.close()


def run_tests(tgf_bin, grammar_file, ftxui):
    kt = KeyTester(tgf_bin, grammar_file, ftxui)
    if not kt.ready():
        print("error: REPL did not show prompt", file=sys.stderr)
        kt.close()
        return 0, 1

    passed = 0
    failed = 0
    mode = "FTXUI" if ftxui else "legacy"

    def test(name, fn):
        nonlocal passed, failed
        kt.clear()
        ok, msg = fn()
        print(f"  {'PASS' if ok else 'FAIL'} [{mode}] {name}: {msg}")
        if ok:
            passed += 1
        else:
            failed += 1

    # --- Ctrl+A (Home) ---
    def ctrl_home():
        kt.t.send_text("abc")
        kt.t.send_ctrl("a")
        kt.t.send_text("X")
        line = kt.buffer()
        if "Xabc" not in line:
            return False, f"expected Xabc, got {line!r}"
        return True, "Xabc"

    # --- Ctrl+E (End) ---
    def ctrl_end():
        kt.t.send_text("abc")
        kt.t.send_ctrl("a")
        kt.t.send_ctrl("e")
        kt.t.send_text("X")
        line = kt.buffer()
        if "abcX" not in line:
            return False, f"expected abcX, got {line!r}"
        return True, "abcX"

    # --- Ctrl+K (kill to end) - FTXUI only ---
    def ctrl_kill():
        if not ftxui:
            return True, "skipped (FTXUI only)"
        kt.t.send_text("hello world")
        kt.left(5)                     # cursor before "world"
        kt.t.send_ctrl("k")
        line = kt.buffer()
        if "hello" not in line or "world" in line:
            return False, f"expected 'hello ', got {line!r}"
        return True, "hello "

    # --- Ctrl+U (kill to start) - FTXUI only ---
    def ctrl_kill_start():
        if not ftxui:
            return True, "skipped (FTXUI only)"
        kt.t.send_text("hello world")
        kt.left(5)                     # cursor before "world"
        kt.t.send_ctrl("u")
        line = kt.buffer()
        if "world" not in line or "hello" in line:
            return False, f"expected 'world', got {line!r}"
        return True, "world"

    # --- Ctrl+W (delete word) - FTXUI only ---
    def ctrl_word():
        if not ftxui:
            return True, "skipped (FTXUI only)"
        kt.t.send_text("hello world")
        kt.t.send_ctrl("w")
        line = kt.buffer()
        if "hello" not in line or "world" in line:
            return False, f"expected 'hello ', got {line!r}"
        return True, "hello "

    print(f"Key binding tests [{mode}]:")
    test("Ctrl+A (home)", ctrl_home)
    test("Ctrl+E (end)", ctrl_end)
    test("Ctrl+K (kill to end)", ctrl_kill)
    test("Ctrl+U (kill to start)", ctrl_kill_start)
    test("Ctrl+W (delete word)", ctrl_word)

    kt.close()
    return passed, failed


def main():
    p = argparse.ArgumentParser(description="Test key bindings in TGF REPL")
    p.add_argument("tgf_bin")
    p.add_argument("grammar")
    args = p.parse_args()

    passed, failed = 0, 0
    for ftxui in (False, True):
        pp, ff = run_tests(args.tgf_bin, args.grammar, ftxui=ftxui)
        passed += pp
        failed += ff

    print(f"\nKey bindings total: {passed} passed, {failed} failed")
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()

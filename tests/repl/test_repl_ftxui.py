#!/usr/bin/env python3
"""Test harness for the FTXUI-based tgf REPL.

Spawns the tgf REPL in a pseudo-terminal (so FTXUI renders its full
alternate-screen UI) and uses pyte to emulate the terminal, recovering
the visible screen as plain text - exactly what a user would see.

Usage:
  ./scripts/test_repl_ftxui.py <tgf_binary> <grammar_file> [<command> ...]

  With commands, each is sent and the screen is captured afterward.
  Without commands, prints the initial screen and exits (ctest smoke mode).
  Pass --interactive to enter stdin-driven mode (one command per line).

Options:
  -r, --rows     Terminal rows (default 40)
  -c, --cols     Terminal columns (default 120)
  -d, --delay    Stabilization delay in seconds after each command (default 0.3)
  -p, --prompt   Prompt pattern to wait for (default 'tgf>')

Requirements:
  pip install pexpect pyte   (or install the system packages)
"""

import sys
import time
import argparse
import pexpect
import pyte


class _PyteFeeder:
    """Receives output as pexpect reads it and feeds it into pyte."""

    def __init__(self, screen: pyte.Screen):
        self.stream = pyte.Stream(screen)

    def write(self, data: bytes | str) -> None:
        if isinstance(data, bytes):
            data = data.decode("utf-8", errors="replace")
        self.stream.feed(data)

    def flush(self) -> None:
        pass


class ReplTester:
    """Manages a tgf REPL under a pty and captures its visible screen."""

    def __init__(
        self,
        tgf_bin: str,
        grammar_file: str,
        prompt: bytes = b"tgf>",
        rows: int = 40,
        cols: int = 120,
        timeout: int = 180,
        legacy: bool = False,
    ):
        self.prompt = prompt
        self.rows = rows
        self.cols = cols
        self.timeout = timeout

        self.screen = pyte.Screen(cols, rows)
        self.feeder = _PyteFeeder(self.screen)

        cmd = f"{tgf_bin} {grammar_file} repl"
        if legacy:
            cmd += " -X"
        self.child = pexpect.spawn(
            cmd,
            dimensions=(rows, cols),
            encoding=None,  # raw bytes for pyte
            timeout=timeout,
            env=self._clean_env(),
        )
        self.child.logfile_read = self.feeder

    @staticmethod
    def _clean_env():
        """Strip env vars that might interfere with the REPL."""
        import os
        env = {}
        for k in ("PATH", "HOME", "USER", "TERM", "COLORTERM"):
            if k in os.environ:
                env[k] = os.environ[k]
        env.setdefault("TERM", "xterm-256color")
        # Keep locale neutral so TGF doesn't trip on unicode
        env["LC_ALL"] = "C.UTF-8"
        return env

    # --- low-level I/O -------------------------------------------------------

    def _drain(self, timeout: float = 0.05) -> None:
        """Consume all pending bytes from the child (feeds pyte)."""
        try:
            while True:
                self.child.read_nonblocking(size=4096, timeout=timeout)
        except (pexpect.TIMEOUT, pexpect.EOF):
            pass

    def _wait_prompt(self, timeout: float | None = None) -> bool:
        """Block until *prompt* appears in the output stream.

        Returns True if the prompt was seen, False on timeout/EOF.
        """
        if timeout is None:
            timeout = self.timeout
        try:
            self.child.expect(self.prompt, timeout=timeout)
            return True
        except pexpect.TIMEOUT:
            return False
        except pexpect.EOF:
            return False

    def _stabilize(self, delay: float) -> None:
        """Wait *delay* seconds, then drain so pyte is current."""
        time.sleep(delay)
        self._drain()

    # --- public API ----------------------------------------------------------

    def wait_ready(self, timeout: float = 180) -> bool:
        """Wait until the REPL has rendered its first frame."""
        ok = self._wait_prompt(timeout)
        self._stabilize(0.2)
        return ok

    def send(self, text: str) -> None:
        """Send a line of input (newline appended)."""
        self.child.sendline(text)

    def send_text(self, text: str) -> None:
        """Send text without a trailing newline."""
        self.child.send(text)

    def send_ctrl(self, char: str) -> None:
        """Send a control character (e.g. 'a' for Ctrl+A)."""
        self.child.sendcontrol(char)

    def get_input_line(self) -> str:
        """Return the current input line (prompt + typed chars)."""
        lines = self.render()
        for line in reversed(lines):
            s = line.rstrip()
            if s:
                return s
        return ""

    def capture(self, command: str, delay: float = 0.3) -> list[str]:
        """Send *command*, wait for the prompt, and return rendered lines."""
        self._drain()               # flush stale frames
        self.send(command)
        self._wait_prompt()
        self._stabilize(delay)
        return self.render()

    def render(self) -> list[str]:
        """Return the visible screen as a list of strings (no trailing \n)."""
        return [line.rstrip() for line in self.screen.display]

    def close(self) -> None:
        """Terminate the child process."""
        if not self.child.isalive():
            return
        try:
            self.child.sendline(b"quit")
            self.child.expect(pexpect.EOF, timeout=10)
        except (pexpect.TIMEOUT, pexpect.EOF):
            pass
        if self.child.isalive():
            self.child.kill(9)
            try:
                self.child.wait()
            except pexpect.ExceptionPexpect:
                pass


# ---------------------------------------------------------------------------


def trim_trailing(lines: list[str]) -> list[str]:
    """Drop trailing blank lines."""
    while lines and not lines[-1].strip():
        lines.pop()
    return lines


def print_screen(lines: list[str], prefix: str = "") -> None:
    """Print screen lines with an optional prefix on each line."""
    for line in lines:
        print(f"{prefix}{line}")


def print_screen_boxed(lines: list[str], title: str = "") -> None:
    """Print screen lines inside a rectangle."""
    width = max((len(line) for line in lines), default=0)
    width = min(width, 120)  # clamp
    top = "┌" + "─" * (width + 2) + "┐"
    bot = "└" + "─" * (width + 2) + "┘"
    if title:
        print(f"── {title} " + "─" * max(0, width - len(title) - 3))
    else:
        print(top)
    for line in lines:
        # pad to width for clean right border
        padded = line.ljust(width)
        print(f"│ {padded} │")
    print(bot if not title else bot)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Test FTXUI tgf REPL in a pseudo-terminal"
    )
    parser.add_argument("tgf_bin", help="Path to the tgf binary")
    parser.add_argument("grammar", help="Path to a TGF grammar file")
    parser.add_argument(
        "commands", nargs="*", help="Commands to send to the REPL"
    )
    parser.add_argument("-r", "--rows", type=int, default=40)
    parser.add_argument("-c", "--cols", type=int, default=120)
    parser.add_argument(
        "-d", "--delay", type=float, default=0.3,
        help="Stabilization delay after each command (default: 0.3s)",
    )
    parser.add_argument(
        "-p", "--prompt", default="tgf>",
        help="Prompt string to wait for (default: 'tgf>')",
    )
    parser.add_argument(
        "--raw", action="store_true",
        help="Print raw lines (one per line, no box)",
    )
    parser.add_argument(
        "--trim", action="store_true",
        help="Strip trailing blank lines from screen output",
    )
    parser.add_argument(
        "--legacy", action="store_true",
        help="Use legacy terminal REPL (-X flag) instead of FTXUI",
    )
    parser.add_argument(
        "--interactive", action="store_true",
        help="Read commands from stdin after the initial screen",
    )
    args = parser.parse_args()

    tester = ReplTester(
        args.tgf_bin,
        args.grammar,
        prompt=args.prompt.encode("utf-8"),
        rows=args.rows,
        cols=args.cols,
        legacy=args.legacy,
    )

    try:
        if not tester.wait_ready():
            print("error: REPL did not show prompt in time", file=sys.stderr)
            print("(last screen):", file=sys.stderr)
            print_screen(tester.render(), prefix="  ")
            sys.exit(1)

        # Show initial screen
        lines = tester.render()
        if args.trim:
            lines = trim_trailing(lines)
        if not args.raw:
            print_screen_boxed(lines, "Initial screen")
        else:
            print("\n".join(lines))

        # Batch commands from argv
        for cmd in args.commands:
            lines = tester.capture(cmd, delay=args.delay)
            if args.trim:
                lines = trim_trailing(lines)
            if not args.raw:
                print_screen_boxed(lines, f"After: {cmd}")
            else:
                print("\n".join(lines))

        # Stdin mode (explicit opt-in for manual use)
        if not args.commands and args.interactive:
            print("(type commands, Ctrl+D to exit)\n")
            for line in sys.stdin:
                line = line.strip()
                if not line:
                    continue
                if line in ("exit", "quit", ":q"):
                    break
                lines = tester.capture(line, delay=args.delay)
                if args.trim:
                    lines = trim_trailing(lines)
                if not args.raw:
                    print_screen_boxed(lines, f"After: {line}")
                else:
                    print("\n".join(lines))

    except KeyboardInterrupt:
        pass
    finally:
        tester.close()


if __name__ == "__main__":
    main()

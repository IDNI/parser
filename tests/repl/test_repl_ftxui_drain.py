#!/usr/bin/env python3
"""Reproduce lost drain_output text on the rendered FTXUI screen.

Usage:
  ./test_repl_ftxui_drain.py <tgf_binary> <grammar_file>
"""

import argparse
import sys

from test_repl_ftxui import ReplTester
from test_repl_ftxui_async import wait_for


def fail(message, tester):
    print(message, file=sys.stderr)
    print("\n".join(tester.render()), file=sys.stderr)
    raise SystemExit(1)


def not_evaluating(lines):
    return not any("evaluating" in line for line in lines)


def check_no_spinner_fragment(screen, tester):
    # "evaluating" is drawn over a row that the next draw only partly
    # overwrites when the drain misaligns the cursor; the tail then
    # survives as "aluating" glued to whatever replaced it.
    for line in screen:
        if "aluating" in line:
            fail(f"spinner fragment glued to another word: {line!r}", tester)


def find_line(lines, needle, start=0):
    for i in range(start, len(lines)):
        if needle in lines[i]:
            return i
    return -1


def find_line_ending(lines, suffix, start=0):
    for i in range(start, len(lines)):
        if lines[i].endswith(suffix):
            return i
    return -1


def check_prompt_before(screen, tester, prompt_text, output_text, context,
        exact=False, start=0):
    prompt_idx = (find_line_ending(screen, prompt_text, start) if exact
        else find_line(screen, prompt_text, start))
    if prompt_idx == -1:
        fail(f"{context}: prompt line {prompt_text!r} missing", tester)
    output_idx = find_line(screen, output_text, prompt_idx + 1)
    if output_idx == -1:
        fail(f"{context}: output line {output_text!r} missing", tester)
    if prompt_idx >= output_idx:
        fail(f"{context}: prompt line {prompt_text!r} not before "
            f"{output_text!r}", tester)
    return prompt_idx


def check_no_column_zero_echo(screen, tester, submitted_text):
    for line in screen:
        if line.startswith(submitted_text):
            fail(f"submitted text echoed at column 0: {line!r}", tester)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("tgf_bin")
    parser.add_argument("grammar")
    args = parser.parse_args()

    tester = ReplTester(args.tgf_bin, args.grammar)
    try:
        if not tester.wait_ready():
            fail("initial prompt did not appear", tester)

        tester.send("p x = 0 || y = 1")
        if not wait_for(tester, not_evaluating):
            fail("evaluation of a valid formula did not finish", tester)
        tester._stabilize(0.2)
        screen = tester.render()
        if not any("parsed terminals" in line for line in screen):
            fail("'parsed terminals' missing after a valid parse", tester)
        check_no_spinner_fragment(screen, tester)
        check_prompt_before(screen, tester, "tgf> p x = 0 || y = 1",
            "parsed terminals", "valid formula")
        check_no_column_zero_echo(screen, tester, "p x = 0 || y = 1")

        tester.send("p x = 2")
        if not wait_for(tester, not_evaluating):
            fail("evaluation of a syntax error did not finish", tester)
        tester._stabilize(0.2)
        screen = tester.render()
        if not any("Syntax Error" in line for line in screen):
            fail("'Syntax Error' missing after a bad parse", tester)
        if any("evaluating" in line for line in screen):
            fail("spinner line still present after evaluation finished",
                tester)
        check_no_spinner_fragment(screen, tester)
        check_prompt_before(screen, tester, "tgf> p x = 2",
            "Syntax Error", "syntax error")
        check_no_column_zero_echo(screen, tester, "p x = 2")

        # "p" alone is incomplete input, so the next line continues it.
        tester.send("p")
        if not wait_for(tester, not_evaluating):
            fail("evaluation of the incomplete formula did not finish",
                tester)
        tester._stabilize(0.2)

        tester.send("x = 0 || y = 1")
        if not wait_for(tester, not_evaluating):
            fail("evaluation of the completed multiline formula did not "
                "finish", tester)
        tester._stabilize(0.2)
        screen = tester.render()
        if not any("parsed terminals" in line for line in screen):
            fail("'parsed terminals' missing after a multiline parse",
                tester)
        check_no_spinner_fragment(screen, tester)
        prompt_idx = check_prompt_before(screen, tester, "tgf> p",
            "parsed terminals", "multiline formula", exact=True)
        check_prompt_before(screen, tester, "x = 0 || y = 1",
            "parsed terminals", "multiline formula continuation",
            start=prompt_idx + 1)
        check_no_column_zero_echo(screen, tester, "x = 0 || y = 1")

        tester.send("quit")
        if not wait_for(tester, not_evaluating):
            fail("evaluation of quit did not finish", tester)
        tester._stabilize(0.2)
        screen = tester.render()
        if not any(line.strip() == "Quit." for line in screen):
            fail("'Quit.' missing or not on its own line after quit", tester)
        check_no_spinner_fragment(screen, tester)
    finally:
        tester.close()

    print("PASS: drain_output rendering")


if __name__ == "__main__":
    main()

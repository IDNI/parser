# testing

## test_parser

This program tests the parser library

Use `-h` or `--help` to see options and how to enable/disable test groups, tests and subtests

## test_input

Test the `parser::input` input data wrapper

## test_tgf

Tests the TGF language

## grammar tests

A `.tgf.test` file lists inputs for one grammar and asserts the shape of each parse tree.

Suites live in `tests/**/*.tgf.test`, in `tests/format/**/*.tgf.test`, or in `src/format/**/*.tgf.test`.

Run one suite with `tgf <grammar.tgf> test <file.tgf.test>`.

`ctest` runs every suite when the `TAU_RUN_GRAMMAR_TESTS` CMake option is on. `tests/grammar_tests.cmake` lists them.

Page [`TGF test files`](../docs/tgf.test.md) describes the file format and the ctest setup.

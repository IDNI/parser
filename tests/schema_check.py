#!/usr/bin/env python3
# Validate real tgf JSON output against the AST schema or the whole API schema.
import json
import os
import subprocess
import sys
import tempfile


def usage():
    print("usage: schema_check.py --ast <tgf> <fixture.tgf> "
          "<ast.schema.json>")
    print("       schema_check.py --api <tgf> <fixture.tgf> "
          "<tgf_api.schema.json> <report.schema.json> <ast.schema.json>")
    return 2


def load_schema(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def require_jsonschema():
    try:
        import jsonschema
        import referencing
    except ImportError:
        print("jsonschema module is not available")
        return None
    return jsonschema, referencing


def build_validator(jsonschema, referencing, api_path, ref_paths):
    api = load_schema(api_path)
    resources = []
    for path in ref_paths:
        schema = load_schema(path)
        resources.append((schema["$id"],
            referencing.Resource.from_contents(schema)))
    resources.append((api["$id"],
        referencing.Resource.from_contents(api)))
    registry = referencing.Registry().with_resources(resources)
    return jsonschema.Draft202012Validator(api, registry=registry)


def run_ast(tgf, fixture, schema_path):
    mods = require_jsonschema()
    if mods is None:
        return 77
    jsonschema, _ = mods
    schema = load_schema(schema_path)

    proc = subprocess.run([tgf, fixture, "parse", "--json", "-e", "12"],
        capture_output=True, text=True)
    if proc.returncode != 0:
        print("tgf parse failed with code %d" % proc.returncode)
        print(proc.stdout)
        print(proc.stderr)
        return 1
    lines = [l for l in proc.stdout.strip().splitlines() if l]
    if not lines:
        print("tgf wrote no response line")
        return 1
    response = json.loads(lines[-1])
    tree = response.get("result", {}).get("tree")
    if tree is None:
        print("tgf response has no result.tree")
        print(lines[-1])
        return 1

    # Wrap the bare node in a document so the schema $ref resolves.
    jsonschema.validate(instance={"ast": tree}, schema=schema)
    print("ast schema check: %s validates" % fixture)
    return 0


def validate_line(validator, line, label):
    try:
        instance = json.loads(line)
    except ValueError as exc:
        print("api schema check: %s is not JSON: %s" % (label, exc))
        return False
    errors = sorted(validator.iter_errors(instance),
        key=lambda e: list(e.path))
    if not errors:
        return True
    print("api schema check: %s does not validate" % label)
    print("  instance: %s" % line[:400])
    for err in errors[:3]:
        print("  %s: %s" % (list(err.path), err.message))
    return False


def run_oneshot(tgf, validator, args, label):
    proc = subprocess.run([tgf] + args, capture_output=True, text=True)
    ok = True
    lines = [l for l in proc.stdout.splitlines() if l.strip()]
    if not lines:
        print("api schema check: %s wrote no response line" % label)
        return False
    for line in lines:
        if not validate_line(validator, line, "oneshot " + label):
            ok = False
    return ok


def run_repl(tgf, fixture, validator, requests):
    proc = subprocess.Popen([tgf, fixture, "repl", "--json"],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL, text=True, bufsize=1)
    ok = True
    responses = []
    hello = proc.stdout.readline()
    if not validate_line(validator, hello, "hello"):
        ok = False
    for label, request, check_request in requests:
        line = request if isinstance(request, str) \
            else json.dumps(request)
        if check_request and not validate_line(validator, line,
                "request " + label):
            ok = False
        proc.stdin.write(line + "\n")
        proc.stdin.flush()
        response = proc.stdout.readline()
        responses.append(response)
        if not validate_line(validator, response,
                "response " + label):
            ok = False
    proc.stdin.close()
    try:
        proc.wait(timeout=30)
    except subprocess.TimeoutExpired:
        proc.kill()
        print("api schema check: tgf repl did not exit")
        ok = False
    return ok, responses


def run_api(tgf, fixture, api_path, report_path, ast_path):
    mods = require_jsonschema()
    if mods is None:
        return 77
    jsonschema, referencing = mods
    validator = build_validator(jsonschema, referencing, api_path,
        [report_path, ast_path])

    with tempfile.TemporaryDirectory() as tmp:
        input_path = os.path.join(tmp, "input.txt")
        with open(input_path, "w", encoding="utf-8") as f:
            f.write("12")
        gen_dir = os.path.join(tmp, "gen")

        requests = [
            ("eval", {"id": 1, "cmd": "eval", "src": "version"}, True),
            ("parse", {"id": 2, "cmd": "parse", "input": "12"}, True),
            ("parse file", {"id": 3, "cmd": "parse file",
                "file": input_path}, True),
            ("grammar", {"id": 4, "cmd": "grammar"}, True),
            ("internal-grammar", {"id": 5,
                "cmd": "internal-grammar"}, True),
            ("start", {"id": 6, "cmd": "start", "symbol": "start"},
                True),
            ("unreachable", {"id": 7, "cmd": "unreachable"}, True),
            ("reload", {"id": 8, "cmd": "reload"}, True),
            ("load", {"id": 9, "cmd": "load", "file": fixture}, True),
            ("help", {"id": 10, "cmd": "help"}, True),
            ("version", {"id": 11, "cmd": "version"}, True),
            ("license", {"id": 12, "cmd": "license"}, True),
            ("clear", {"id": 13, "cmd": "clear"}, True),
            ("get all", {"id": 14, "cmd": "get"}, True),
            ("get one", {"id": 15, "cmd": "get",
                "option": "trim"}, True),
            ("set", {"id": 16, "cmd": "set", "option": "trim",
                "value": ["a", "b"]}, True),
            ("toggle", {"id": 17, "cmd": "toggle",
                "option": "print-ambiguity"}, True),
            ("enable", {"id": 18, "cmd": "enable",
                "option": "print-ambiguity"}, True),
            ("disable", {"id": 19, "cmd": "disable",
                "option": "print-ambiguity"}, True),
            ("add", {"id": 20, "cmd": "add", "option": "trim",
                "value": ["c"]}, True),
            ("delete", {"id": 21, "cmd": "delete",
                "option": "trim", "value": ["c"]}, True),
            ("error before command", {"id": 22, "cmd": "bogus"},
                False),
            ("error in command", {"id": 23, "cmd": "parse",
                "input": "!"}, True),
            ("incomplete", {"id": 24, "cmd": "eval", "src": "set"},
                True),
            ("invalid json", "{ not json", False),
            ("quit", {"id": 25, "cmd": "quit"}, True),
        ]

        ok, _ = run_repl(tgf, fixture, validator, requests)

        # the top status of an eval response is quit when a statement
        # quits, even when an earlier statement errored
        quit_request = [("eval quit", {"id": 1, "cmd": "eval",
            "src": "parse ! . quit"}, True)]
        quit_ok, quit_responses = run_repl(tgf, fixture, validator,
            quit_request)
        if quit_ok:
            response = json.loads(quit_responses[-1])
            if response.get("status") != "quit":
                print("api schema check: eval quit top status is "
                      "not quit")
                quit_ok = False
        ok &= quit_ok

        ok &= run_oneshot(tgf, validator,
            [fixture, "parse", "--json", "-e", "12"], "parse")
        ok &= run_oneshot(tgf, validator,
            [fixture, "parse", "--json", "--grammar", "--measure",
             "-e", "12"], "parse --grammar --measure")
        ok &= run_oneshot(tgf, validator,
            [fixture, "grammar", "--json", "--nullable"], "grammar")
        ok &= run_oneshot(tgf, validator,
            [fixture, "gen", "--json", "--name", "schema_check",
             "--header-only", "false", "--output-dir", gen_dir], "gen")

    if not ok:
        print("api schema check: FAILED")
        return 1
    print("api schema check: %s validates" % fixture)
    return 0


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("--ast", "--api"):
        return usage()
    if sys.argv[1] == "--ast":
        if len(sys.argv) != 5:
            return usage()
        return run_ast(sys.argv[2], sys.argv[3], sys.argv[4])
    if len(sys.argv) != 7:
        return usage()
    return run_api(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5],
        sys.argv[6])


if __name__ == "__main__":
    sys.exit(main())

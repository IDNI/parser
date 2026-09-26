#!/usr/bin/env python3
# Validate the tree of a real tgf parse against src/format/ast.json/ast.schema.json.
import json
import subprocess
import sys


def main():
    if len(sys.argv) != 4:
        print("usage: ast_schema_check.py <tgf> <fixture.tgf> <schema.json>")
        return 2
    tgf, fixture, schema_path = sys.argv[1], sys.argv[2], sys.argv[3]
    with open(schema_path, "r", encoding="utf-8") as f:
        schema = json.load(f)

    try:
        import jsonschema
    except ImportError:
        print("jsonschema module is not available")
        return 77

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


if __name__ == "__main__":
    sys.exit(main())

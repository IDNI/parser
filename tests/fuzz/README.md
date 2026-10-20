# Fuzz targets

Each `fuzz_*.cpp` file is one libFuzzer target. Build the targets with
clang and `-DTAU_PARSER_BUILD_FUZZ=ON`:

```bash
./dev preset release-tests -DTAU_PARSER_BUILD_FUZZ=ON --target fuzz_json
```

Give the fuzzer an empty work folder first and the seed corpus second.
The fuzzer writes new inputs only to the first folder, so the seeds in the
repository stay as they are.

```bash
mkdir -p build/fuzz/json
build/release/fuzz_json build/fuzz/json tests/fuzz/corpus/json \
	-max_total_time=60
```

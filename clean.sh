#!/bin/bash

if [[ "$1" = "all" ]]; then
	rm -rf build build-* tau-lang
fi

rm -f tree*.dot graph*.dot forest*.tml forest*.dot graph*.tml parse_rules*.tml

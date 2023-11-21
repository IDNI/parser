#!/bin/sh

if [[ "$1" = "all" ]]; then
	rm -rf build build-*
fi

rm -f tree*.dot graph*.dot forest*.tml forest*.dot graph*.tml

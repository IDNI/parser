#!/bin/bash

./dev build Debug -DTAU_PARSER_BUILD_EMSCRIPTEN=ON -DEMSCRIPTEN_DIR="$(pwd)/external/emsdk/upstream/emscripten" $@
cp js/tauparser.html js/tauparser.node.js build-Debug/

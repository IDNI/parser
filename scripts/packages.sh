#!/bin/bash

./dev build Release \
        -DTAU_PARSER_BUILD_TGF=ON \
        -DTAU_PARSER_BUILD_EXAMPLES=ON \
        $@
cd ./build-Release
cpack -C Release
cd ..

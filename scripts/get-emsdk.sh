#!/bin/env bash

EMSDK_DIR="./external/emsdk"

# Download the emsdk
curl -L https://github.com/emscripten-core/emsdk/archive/refs/heads/main.zip -o emsdk.zip
unzip -q emsdk.zip
mkdir -p $EMSDK_DIR
mv emsdk-main/* $EMSDK_DIR
rmdir emsdk-main
rm emsdk.zip

# Install the latest emsdk version
cd $EMSDK_DIR
./emsdk install latest
./emsdk activate latest

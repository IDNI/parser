#!/bin/env bash

EMSDK_SRC="emsdk-main"
EMSDK_DIR="emsdk"

mkdir -p ./external
cd ./external

# Download the emsdk
curl -L https://github.com/emscripten-core/emsdk/archive/refs/heads/main.zip -o emsdk.zip
unzip -q emsdk.zip
mkdir -p $EMSDK_DIR
mv $EMSDK_SRC/* $EMSDK_DIR
rmdir $EMSDK_SRC
rm emsdk.zip

# Install the latest emsdk version
cd $EMSDK_DIR
./emsdk install latest
./emsdk activate latest

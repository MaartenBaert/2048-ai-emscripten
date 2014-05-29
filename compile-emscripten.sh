#!/bin/bash

# Path to emscripten compiler:
EMCC="/static/maarten/nobackup/build-from-source/emscripten/emcc"

#TARGET="Speed2048.js" # for Node.js
TARGET="Speed2048ff.html" # for browsers

set -e

"$EMCC" -O3 -std=c++11 -DNDEBUG -s TOTAL_MEMORY=33554432 --closure 1 *.cpp -o "$TARGET"

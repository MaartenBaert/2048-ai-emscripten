#!/bin/bash

set -e

clang++ -O3 -std=c++11 -DNDEBUG *.cpp -o Speed2048-clang

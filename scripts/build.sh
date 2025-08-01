#!/bin/bash
set -e

# 1. Build the app (debug mode)
cd ..
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --config Debug

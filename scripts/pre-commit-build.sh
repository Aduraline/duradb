#!/usr/bin/env bash
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
cd "$root"

cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure

# Contributing to DuraDB

Thanks for your interest in contributing. This guide covers the basics to get started.

## Build

You need a C++20 compiler (clang 13+, GCC 10+, or MSVC 19.29+) and CMake 3.16+.

```bash
cmake -S . -B build
cmake --build build
```

The binary is at `./build/duradb`. Run it for the interactive SQL shell.

Optional: install [pre-commit](https://pre-commit.com/) hooks so each commit runs formatting, build, and tests locally:

```bash
pre-commit install
```

## Run tests

After building:

```bash
ctest --test-dir build --output-on-failure
```

Add new tests in `tests/` using GoogleTest. Every PR must pass all tests. CI runs the same build and test steps on every pull request.

## Open a PR

1. Fork the repo and create a branch from `main` (e.g. `feat/my-feature`, `fix/my-fix`, `docs/my-doc`).
2. Make your changes. Keep PRs focused on one thing.
3. Run tests locally before pushing.
4. Open a pull request against `main` with a short description of what changed and why.
5. Use commit messages in this format: `type(scope): short description` (e.g. `feat(storage): add zone maps`, `docs(readme): update build steps`).

We review PRs as quickly as we can. If you are unsure where to start, check open issues labeled `good first issue`.

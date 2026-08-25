# Buna's Math Library (BML)

**BML 0.5.0** is an experimental C11 symbolic expression engine for scalar real-valued arithmetic. It parses, evaluates, differentiates, simplifies, verifies, and emits C for a deliberately small expression language.

## Quick Start

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bml_cli simplify '(x + 0) * 1'
./build/bml_cli diff 'x^3 + cos(x)' x
./build/bml_cli emit-c --optimize '(x + y) * (x + y)'
./build/bml_bench --csv results.csv --markdown report.md
```

Parse failures include a source offset and caret context. Run `bml_cli --version` to identify a release. The supported input operators are `+ - * / ^`, unary minus, parentheses, numbers, variables, and single-argument `sin`, `cos`, `exp`, and `log`.

## Install and Consume

```sh
cmake --install build --prefix /tmp/bml
```

```cmake
find_package(BML CONFIG REQUIRED)
add_executable(app app.c)
target_link_libraries(app PRIVATE BML::bml)
```

The installation also provides `bml.pc` for `pkg-config`. The public umbrella header is `#include <bml/bml.h>`; `bml/internal/*` is not public.

## Development

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
cmake --build build/dev --target format-check
cmake --build build/dev --target clang-tidy
cmake --build build/dev --target cppcheck
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize
```

`bml_bench` writes stable CSV metrics to stdout or `--csv FILE`; `--markdown FILE` adds a reproducible report header with the version, UTC timestamp, documented command, and aggregate metrics. Timing values intentionally are not checked in because they vary by machine.

See [release support](docs/release-support.md), [semantics](docs/semantics.md), [architecture](docs/architecture.md), and [CHANGELOG.md](CHANGELOG.md). The local optimizer is the supported path. The opt-in `egraph` engine is experimental, bounded, and not a general e-graph.

## Scope

BML is a research and educational project, not a general-purpose CAS. It does not promise ABI stability before 1.0, broad mathematical domains, or proof-complete optimization. The v0.5 release preserves the v0.4 language, scalar semantics, and rewrite set.

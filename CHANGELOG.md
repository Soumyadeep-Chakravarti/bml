# Changelog

## 0.5.0 - 2026-08-25

- Added CMake package export (`BML::bml`), generated config/version files, header installation, and `pkg-config` metadata.
- Added `BML_VERSION_*` macros and CLI `--version`.
- Hardened CLI diagnostics, including parser offsets and source context.
- Added CSV and optional Markdown benchmark reports.
- Added deterministic parser fuzzing, optional libFuzzer support, and optional SymPy differential checks.
- Added GCC/Clang CI, sanitizer coverage, quality-tool checks, and installed-package smoke testing.

No symbolic domains, local rewrite rules, or experimental e-graph rules changed.

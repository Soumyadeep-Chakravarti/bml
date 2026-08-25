# Release Support

BML 0.5 supports ISO C11 consumers on Linux with GCC or Clang and CMake 3.20 or newer. The packaged library is static and is consumed through `find_package(BML CONFIG REQUIRED)` and `BML::bml`; `pkg-config --cflags --libs bml` is also provided on Unix-like installations.

The public API is the headers installed under `include/bml`, beginning with `bml/bml.h`. Headers under `bml/internal` are implementation details and are not installed. This pre-1.0 project makes no ABI stability guarantee across minor releases.

The local simplifier is the supported optimizer. `BML_OPTIMIZER_EGRAPH` is explicitly experimental, bounded, opt-in, and not a general equality-saturation implementation. It must not be relied on for proof completeness or stable optimization choices.

Quality coverage is Linux GCC/Clang CTest, AddressSanitizer/UndefinedBehaviorSanitizer, formatting, clang-tidy, Cppcheck, deterministic parser fuzzing, and optional SymPy differential tests. SymPy is only a test-time optional dependency.

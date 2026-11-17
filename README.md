# PrometheusModeller

C++23 library using CMake and vcpkg, with Armadillo, nlohmann-json, and GoogleTest.
Requires CMake 3.25+, Ninja, and a C++23 compiler.

```sh
module load vcpkg
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

CMake uses `VCPKG_ROOT` from the module to load vcpkg and install dependencies.
On another system, set `VCPKG_ROOT` to your vcpkg directory before configuring.

The main `prometheus_modeller` library links the `prometheus_geometry` and
`prometheus_topology` libraries. Each module under `src/` has its own
`CMakeLists.txt`, public headers in `include/`, and private implementation headers,
sources, and internal tests in `src/`. Shared public types live in
`include/prometheus/common.hpp`.

Each module builds one test executable (`geometry_tests` or `topology_tests`).
Add implementation files to the module's library target and all test files to its
single test target. Test targets can access private headers; library consumers
receive only public include directories. The root `tests/` directory contains
tests of the main library, built as `prometheus_modeller_tests`.

Other targets can link to `prometheus_modeller` for both modules, or link to a
module library directly. All tests are registered with CTest.

For your language server, `build/compile_commands.json` is generated during
configuration. Expose it at the project root once on macOS/Linux:

```sh
ln -s build/compile_commands.json compile_commands.json
```

Use `-DCMAKE_BUILD_TYPE=Release` for an optimized build, or `-DBUILD_TESTING=OFF`
to build only the library.

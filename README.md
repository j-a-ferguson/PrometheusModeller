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

Consumers link to `prometheus_modeller` and include `<prometheus/modeller.hpp>`.
That umbrella header re-exports the curve and surface declarations from
`src/geometry/include/` and the node ID declarations from `src/topology/include/`.
It also re-exports shared types from the header-only `prometheus_common` module
under `src/common/include/`. Consumers and integration
tests under `tests/` receive only `src/api/` as a project include directory.
Module unit tests can also include their module's internal headers.

For your language server, `build/compile_commands.json` is generated during
configuration. Expose it at the project root once on macOS/Linux:

```sh
ln -s build/compile_commands.json compile_commands.json
```

Use `-DCMAKE_BUILD_TYPE=Release` for an optimized build, or `-DBUILD_TESTING=OFF`
to build only the library.

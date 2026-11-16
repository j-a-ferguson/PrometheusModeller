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

Put public headers in `include/`, implementations in `src/`, and tests in `tests/`.
Add new source files to the corresponding target in `CMakeLists.txt`.
Other targets can link to `prometheus_modeller`.

For your language server, `build/compile_commands.json` is generated during
configuration. Expose it at the project root once on macOS/Linux:

```sh
ln -s build/compile_commands.json compile_commands.json
```

Use `-DCMAKE_BUILD_TYPE=Release` for an optimized build, or `-DBUILD_TESTING=OFF`
to build only the library.

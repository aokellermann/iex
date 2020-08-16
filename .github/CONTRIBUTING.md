# Contributing
In addition to the dependencies listed [here](../README.md#Dependencies), you will also need:
* clang-tidy 10.0.0+
* clang-format 10.0.0+

### Code Style
C++ code in this repository follows [Google C++ Style](https://google.github.io/styleguide/cppguide.html) fairly closely. Divergences and further specifications are laid out below:
* Maximum line length is 120 characters.
* Use `#pragma once` instead of header guards.
* Braces go on a new line, unless the entire expression can fit on one line.
* File names never use dashes.
* Doxygen is used for documentation.
* Exceptions may be thrown.
When in doubt, follow conventions in preexisting code.

You can automatically format your code with:
 ```bash
bash scripts/format.sh
```

### Building
The project's `CMakeLists.txt` supports the following options:
* `IEX_BUILD_LIBRARY`: Build `iex` library.
* `IEX_BUILD_WARNINGS`: Turn on extra compiler warnings.
* `IEX_BUILD_TESTING`: Build unit test target.
* `IEX_BUILD_DOCUMENTATION`: Build documentation using `doxygen`.
* `IEX_ENABLE_STRESS_TESTS`: Build stress unit tests (CI does not run these).
* `IEX_TIDY`: Run static analyzer.

To perform a weed whack build, run:
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -IEX_BUILD_WARNINGS:BOOL=ON -IEX_BUILD_TESTING:BOOL=ON -IEX_BUILD_DOCUMENTATION:BOOL=ON -IEX_TIDY:BOOL=ON ..
cd ..
cmake --build build/
```
After, you can run unit tests:
```bash
././build/tests/unit_test
```
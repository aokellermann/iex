# Contributing

## Issues

### Weed report (something is not working)

For the meaning of "weed", see [here](https://github.com/maxitg/SetReplace/blob/master/.github/CONTRIBUTING.md#weed-reports-something-is-not-working).

First, check if there are any issues open regarding the issue you're currently having. If there is one open, you may comment on it. If there isn't one open, you may open a new weed report with a description and code excerpt of the problem you're having.

### Features

If there is functionality you would like to see in the project, you may open a feature request. Since this is a hobby project with sporadic updates, it may be most efficient if you implement the feature yourself. 

## Code

Our CI performs several checks before passing a build:
1. Formatting (clang-format)
2. Static analysis (clang-tidy)
3. Compiler warnings
4. Unit Tests (GTest)

In addition to the dependencies listed [here](../README.md#Dependencies), you will also need:
* clang-tidy 10.0.0+
* clang-format 10.0.0+
* gtest 1.10+

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
* `IEX_BUILD_LIBRARY`: Build `iex` library (default on).
* `IEX_BUILD_WARNINGS`: Build with compiler warnings.
* `IEX_BUILD_TESTS`: Build unit tests.
* `IEX_BUILD_STRESS_TESTS`: Build stress unit tests (CI does not run these).
* `IEX_BUILD_DOCUMENTATION`: Build Doxygen documentation.
* `IEX_TIDY`: Run static analyzer.
* `BUILD_SHARED_LIBS`: Build shared library rather than static (default on).

There are two types of weed whack builds that you can run:

To perform a regular weed whack build, run:
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DIEX_BUILD_TESTS=ON ..
cmake --build .
```

This is for your regular testing.

To perform a release weed whack build, run:
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DIEX_BUILD_TESTS=ON -DIEX_BUILD_WARNINGS=ON -DIEX_TIDY=ON ..
cmake --build .
```

This is for your testing right before you push. The inclusion of warnings and static analysis will cause the build to take much longer, but the CI will not pass the build unless there are no warnings.

After, you can run unit tests:
```bash
././build/tests/unit_test
```
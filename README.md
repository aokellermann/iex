# iex

### Description

`iex` is a WIP C++17 library for querying [IEX Cloud](https://iexcloud.io/), a financial data REST API.

### Install

Please note that this library is only designed to be built on Linux, and has only been tested with GCC 10.1 and Clang 10.0.

#### Arch Linux

An [AUR package](https://aur.archlinux.org/packages/iex-git/) is available:
```bash
yay -S iex-git
```

#### Other Linux

##### Dependencies
Building from source requires the installation of all necessary dependencies:
* [cmake](https://github.com/Kitware/CMake) (build only)
* [curl](https://github.com/curl/curl)
* [json](https://github.com/nlohmann/json)
* [doxygen](https://github.com/doxygen/doxygen) (optional documentation)

##### Clone and Build
After dependencies are installed, run from a shell:
```bash
git clone https://github.com/aokellermann/iex.git
mkdir iex/build && cd iex/build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
sudo cmake --build . --target install
```
Substitute `/usr` with your desired install location.

### Usage

#### Build

Simply include the header `iex/iex.h`:
```c++
#include <iex/iex.h>
```

You can easily link using CMake:
```cmake
target_link_libraries(example iex::iex)
```

#### Initialization

You must call `iex::Init` before any other threads have been created before using any `iex::Get` functions. This lack of thread-safety is not because of this library (see [here](https://curl.haxx.se/libcurl/c/curl_global_init.html)).

Here is an example of using environment variables to initialize `iex`:

See the [examples](examples) directory for a fully working annotated example usage of `iex`, along with an accompanying example `CMakeLists.txt`.

### Contributing

See [contributing guidelines](.github/CONTRIBUTING.md).

### License

Copyright 2020 Antony Kellermann

[Data provided by IEX Cloud](https://iexcloud.io)

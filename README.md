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

#### Code

See the [examples](examples) directory for a fully working annotated example usage of `iex`, along with an accompanying example `CMakeLists.txt`.

Currently this library supports the following endpoints:
* System Status
* Quote
* Company
* Symbols

### Contributing

See [contributing guidelines](.github/CONTRIBUTING.md).

### License

[MIT](LICENSE)

[Data provided by IEX Cloud](https://iexcloud.io)

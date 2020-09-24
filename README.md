# iex

### Description
iex is a WIP C++17 library for querying [IEX Cloud](https://iexcloud.io/), a financial data REST API.

### Install
Please note that this library is only designed to be built on Linux, and has only been tested with GCC 10.1 and Clang 10.0.

#### Arch Linux Package

Arch Linux users may install from the AUR:
```bash
git clone https://aur.archlinux.org/iex-git.git && cd iex-git
makepkg -si
```
Or by using an AUR helper:
```bash
yay -S iex-git
```

#### Manual Build

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

Simply include the public header `iex/iex.h`.

If you are building with CMake, you can link using `iex::iex`:
```cmake
target_link_libraries(example iex::iex)
```

See the [examples](examples) directory for a fully working annotated example usage of `iex`, along with an accompanying example `CMakeLists.txt`.

### Contributing

See [contributing guidelines](.github/CONTRIBUTING.md).

### Projects

##### API Goals
* Support calls to all [stock price](https://iexcloud.io/docs/api/#stock-prices) endpoints.
* Support calls to all [stock fundamental](https://iexcloud.io/docs/api/#stock-fundamentals) endpoints.
* Support calls to some [reference](https://iexcloud.io/docs/api/#reference-data) endpoints:
  * Symbols
  * OTC Symbols
  * Mutual Fund Symbols
  * Cryptocurrency Symbols
* Provide data caching:
  * Smart querying (will not make API call if relevant data is already cached)
  * Cache management
  * Cache-only querying

[Data provided by IEX Cloud](https://iexcloud.io)

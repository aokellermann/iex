# iex

### Description
iex is a WIP C++17 library for querying [IEX Cloud](https://iexcloud.io/), a financial data REST API.

### Install

#### Manual Build

##### Dependencies
Building from source requires the installation of all necessary dependencies:
* [cmake](https://github.com/Kitware/CMake) (build only)
* [curl](https://github.com/curl/curl)
* [json](https://github.com/nlohmann/json)

##### Clone and Build
After dependencies are installed, run from a shell:
```bash
git clone https://github.com/aokellermann/iex.git
mkdir iex/build && cd iex/build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
sudo make install
```
Substitute `/usr` with your desired install location.

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

##### Intended Usage
* Public header `iex.h`.
* Link with `iex::iex` using CMake.

[Data provided by IEX Cloud](https://iexcloud.io)

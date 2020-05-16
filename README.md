# iex

### Description
iex is a WIP C++17 library for querying [IEX Cloud's](https://iexcloud.io/) financial data REST API.

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

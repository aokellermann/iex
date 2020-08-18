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
target_link_libraries(target_name iex::iex)
```

#### Initialization

You must call `iex::Init` before any other threads have been created before using any `iex::Get` functions. This lack of thread-safety is not because of this library (see [here](https://curl.haxx.se/libcurl/c/curl_global_init.html)).

Here is an example of using environment variables to initialize `iex`:

```c++
iex::Keys keys;
keys.public_key = getenv("IEX_PUBLIC_KEY");
keys.secret_key = getenv("IEX_SECRET_KEY");
keys.public_sandbox_key = getenv("IEX_SANDBOX_PUBLIC_KEY");
keys.secret_sandbox_key = getenv("IEX_SANDBOX_SECRET_KEY");
const auto ec = iex::Init(std::move(keys));
if (ec.Failure())
{
  std::cerr << ec << std::endl;
  exit(EXIT_FAILURE);
}
```

#### Fetching Data

There are several overloads of `iex::Get` that you can use to fetch API data.

##### Single Endpoint/Symbol

The following is an example call to fetch $TSLA realtime quote, and printing out the latest price:
```c++
const auto quote = iex::Get<iex::Endpoint::Type::QUOTE>(iex::Symbol("tsla");
if (response.second.Success())
{
    auto price = quote.first->Get<iex::Quote::MemberType::LATEST_PRICE>();
    if (price.has_value())
      std::cout << "TSLA realtime price: $" << price.value() << std::endl;
}
```

##### Single Endpoint Multiple Symbol

The following is an example call to fetch $TSLA, $AMD, and $MSFT realtime quotes, and printing out the latest prices:
```c++
auto response = iex::Get<iex::Endpoint::Type::QUOTE>(
      iex::SymbolSet{iex::Symbol("tsla"), iex::Symbol("amd"), iex::Symbol("msft")});
if (response.second.Success())
{
  for (const auto& [symbol, quote] : response.first)
  {
    if (quote)
    {
      auto price = quote->Get<iex::Quote::MemberType::LATEST_PRICE>();
      if (price.has_value())
        std::cout << symbol.Get() << " realtime price: $" << price.value() << std::endl;
    }
  }
}
```

##### Multiple Endpoint Multiple Symbol

The following is an example call to fetch $TSLA, $AMD, and $MSFT realtime quotes and company information, and printing out the latest prices and company names:
```c++
auto response = iex::Get<iex::Endpoint::Type::QUOTE, iex::Endpoint::Type::COMPANY>(
      iex::SymbolSet{iex::Symbol("tsla"), iex::Symbol("amd"), iex::Symbol("msft")});if (response.second.Success())
{
  for (const auto& [symbol, endpoints] : response.first)
  {
    auto& [quote, company] = endpoints;
    if (quote && company)
    {
      auto price = quote->Get<iex::Quote::MemberType::LATEST_PRICE>();
      auto name = company->Get<iex::Company::MemberType::COMPANY_NAME>();
      if (price.has_value() && name.has_value())
        std::cout << name.value() << " realtime price: $" << price.value() << std::endl;
    }
  }
}
```

Notice in the `iex::Get` calls, there is an optional parameter:
* A collection of endpoint-specific `iex::Option`s
* API Version (defaults to `STABLE`)
* DataType (defaults to `AUTHENTIC`)
    * `AUTHENTIC`: genuine data that counts towards your used credits
    * `SANDBOX`: ingenuine data that doesn't count towards your used credits (usually used for testing)
    
The above examples all use default options. Below is a call to the Beta version of IEX's Sandbox API, with the Quote DisplayPercentOption:
```c++
iex::Endpoint::OptionsObject options{
      {iex::Quote::DisplayPercentOption()}, iex::Version::BETA, iex::DataType::SANDBOX};
  auto response = iex::Get<iex::Endpoint::Type::QUOTE>(iex::Symbol("tsla"), options);
```

### Contributing

See [contributing guidelines](.github/CONTRIBUTING.md).

### License

Copyright 2020 Antony Kellermann

[Data provided by IEX Cloud](https://iexcloud.io)

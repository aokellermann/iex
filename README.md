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

or you can use the `PKGBUILD` in this repository:

```bash
mkdir build && cd build
wget https://raw.githubusercontent.com/aokellermann/iex/master/PKGBUILD
makepkg -si
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
sudo make install
```
Substitute `/usr` with your desired install location.

### Usage

#### Build

Include any necessary headers:
```c++
#include <iex/iex.h>
#include <iex/api/quote.h>
```

This library is designed to be easily linkable using CMake with `iex::iex`:
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

The following is an example templated call to fetch $TSLA's realtime quote, and printing out the latest price:

```c++
// Make sure to include the Quote header
#include <iex/iex.h>  // for iex::Get()
#include <iex/api/quote.h>  // for iex::Quote

...

// Declare types
constexpr const iex::Endpoint::Type kQuoteType = iex::Endpoint::Type::QUOTE;
using QuoteTypename = iex::EndpointTypename<kQuoteType>;
using QuoteMemberType = QuoteTypename::MemberType;

// GET from API
auto quote_response = iex::Get<kQuoteType>(
    iex::Symbol("tsla"),
    iex::RequestOptions{{QuoteTypename::DisplayPercentOption()}, iex::Version::BETA, iex::DataType::SANDBOX});

if (quote_response.second.Success())
{
  // GET successful
  const auto& quote_endpoint = quote_response.first;
    
  // Read latest price data member
  const auto quote_latest_price = quote_endpoint->Get<QuoteMemberType::LATEST_PRICE>();
    
  if (quote_latest_price.has_value())
  {
    // Latest price is available in data
    std::cout << "Latest price: " << quote_latest_price.value() << std::endl;
  }
  else
  {
    // Latest price is not available in data
    std::cout << "Latest price not available" << std::endl;
  }
}
else
{
  // Failed to get data from API
  std::cerr << quote_response.second << std::endl;
}
```

Notice in the `iex::Get` call, there are some options passed:
* A collection of endpoint-specific `iex::Option`s (in this case `iex::Quote::DisplayPercentOption`)
* API Version (defaults to `STABLE`)
* DataType (defaults to `AUTHENTIC`)
    * `AUTHENTIC`: genuine data that counts towards your used credits
    * `SANDBOX`: ingenuine data that doesn't count towards your used credits (usually used for testing)
    
If you want to use default options, you can do this instead:
```c++
auto quote_response = iex::Get<kQuoteType>(iex::Symbol("tsla"));
```

##### Multiple Endpoints/Symbols

Fetching data from multiple endpoints and symbols can be done efficiently, internally performing only one HTTP GET (as long as they are using the same `Version` and `DataType`).

The following is an example single-GET call to fetch $TSLA and $AAPL's realtime quotes and company information.

```c++
// Make sure to include the Quote/Company headers
#include <iex/iex.h>  // for iex::Get()
#include <iex/api/quote.h>  // for iex::Quote
#include <iex/api/company.h>  // for iex::Company

...

// Target Symbols, Endpoints, and options 
iex::Symbol tesla_symbol("tsla");
iex::Symbol apple_symbol("aapl");
constexpr const auto kQuoteType = iex::Endpoint::Type::QUOTE;
constexpr const auto kCompanyType = iex::Endpoint::Type::COMPANY;
const auto tesla_opts =
    iex::RequestOptions{iex::Endpoint::Options{iex::Quote::DisplayPercentOption()}, {}, iex::DataType::SANDBOX};
const auto apple_opts = iex::RequestOptions{iex::Endpoint::Options{}, {}, iex::DataType::SANDBOX};

// Build request object
iex::SymbolRequests sreqs;
iex::Requests reqs = iex::Requests{{kQuoteType, tesla_opts}, {kCompanyType, apple_opts}};
sreqs.emplace(tesla_symbol, reqs);
sreqs.emplace(apple_symbol, reqs);

// Perform GET
const auto response = iex::Get(sreqs);
if (response.second.Failure())
{
  std::cout << response.second << std::endl;
}

for (const auto& symbol : {tesla_symbol, apple_symbol})
{
  const auto& symbol_response = response.first.Get(symbol);
  if (symbol_response)
  {
    const auto& quote = symbol_response->Get<kQuoteType>(tesla_opts);
    const auto& company = symbol_response->Get<kCompanyType>(apple_opts);
    if (quote && company)
    {
      const auto latest_price = quote->Get<iex::EndpointTypename<kQuoteType>::MemberType::LATEST_PRICE>();
      const auto company_name = company->Get<iex::EndpointTypename<kCompanyType>::MemberType::COMPANY_NAME>();
      if (latest_price && company_name)
      {
        std::cout << *company_name << " real-time price: " << *latest_price << std::endl;
      }
    }
  }
}
```

### Contributing

See [contributing guidelines](.github/CONTRIBUTING.md).

### License

Copyright 2020 Antony Kellermann

[Data provided by IEX Cloud](https://iexcloud.io)

/**
 * @file example.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <iex/iex.h>

#include <iostream>

int main(int, char**)
{
  // Initialize iex before any other threads have been created.
  // Note: Init() must be called before any iex::Symbol instances are created.
  // If Init() fails, you won't be able to use the library.

  iex::Keys keys;
  keys.public_key = getenv("IEX_PUBLIC_KEY");
  keys.secret_key = getenv("IEX_SECRET_KEY");
  keys.public_sandbox_key = getenv("IEX_SANDBOX_PUBLIC_KEY");
  keys.secret_sandbox_key = getenv("IEX_SANDBOX_SECRET_KEY");
  const auto ec = iex::Init(std::move(keys));
  if (ec.Failure())
  {
    std::cerr << ec.what() << std::endl;
    exit(EXIT_FAILURE);
  }

  // Single Endpoint Single Symbol Query
  {
    const auto quote = iex::Get<iex::Endpoint::Type::QUOTE>(iex::Symbol("tsla"));

    // Check for nullptr
    if (quote)
    {
      auto price = quote->Get<iex::Quote::MemberType::LATEST_PRICE>();

      // check if std::optional contains value
      if (price.has_value()) std::cout << "TSLA realtime price: $" << price.value() << std::endl;
    }
  }

  // Single Endpoint Multiple Symbol Query
  {
    auto map = iex::Get<iex::Endpoint::Type::QUOTE>(
        iex::SymbolSet{iex::Symbol("tsla"), iex::Symbol("amd"), iex::Symbol("msft")});

    // Iterate over std::unordered_map
    for (const auto& [symbol, quote] : map)
    {
      if (quote)
      {
        auto price = quote->Get<iex::Quote::MemberType::LATEST_PRICE>();
        if (price.has_value()) std::cout << symbol.Get() << " realtime price: $" << price.value() << std::endl;
      }
    }
  }

  // Multiple Endpoint Single Symbol Query
  {
    // Bind endpoints from std::tuple
    const auto [quote, company] =
    iex::Get<iex::Endpoint::Type::QUOTE, iex::Endpoint::Type::COMPANY>(iex::Symbol("tsla"));
    if (quote && company)
    {
      auto price = quote->Get<iex::Quote::MemberType::LATEST_PRICE>();
      auto name = company->Get<iex::Company::MemberType::COMPANY_NAME>();
      if (price.has_value() && name.has_value())
        std::cout << name.value() << " realtime price: $" << price.value() << std::endl;
    }
  }
  // Multiple Endpoint Multiple Symbol Query
  {
    auto map = iex::Get<iex::Endpoint::Type::QUOTE, iex::Endpoint::Type::COMPANY>(
        iex::SymbolSet{iex::Symbol("tsla"), iex::Symbol("amd"), iex::Symbol("msft")});
    for (const auto& [symbol, endpoints] : map)
    {
      // Bind endpoints from std::tuple
      const auto& [quote, company] = endpoints;
      if (quote && company)
      {
        auto price = quote->Get<iex::Quote::MemberType::LATEST_PRICE>();
        auto name = company->Get<iex::Company::MemberType::COMPANY_NAME>();
        if (price.has_value() && name.has_value())
          std::cout << name.value() << " realtime price: $" << price.value() << std::endl;
      }
    }
  }

  // Endpoint Options
  {
    // Multiplies all percentages by 100
    {
      auto opts = iex::Endpoint::OptionsObject{{iex::Quote::DisplayPercentOption()}};
      const auto quote = iex::Get<iex::Endpoint::Type::QUOTE>(iex::Symbol("tsla"), opts);
    }

    // Uses the Beta Sandbox API version
    {
      auto opts = iex::Endpoint::OptionsObject{{}, iex::Version::BETA, iex::DataType::SANDBOX};
      const auto quote = iex::Get<iex::Endpoint::Type::QUOTE>(iex::Symbol("tsla"), opts);
    }
  }
}
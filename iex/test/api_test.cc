/**
 * @file api_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include <mutex>
#include <thread>
#include <vector>

#include "iex/api/company.h"
#include "iex/api/quote.h"
#include "iex/api/system_status.h"
#include "iex/detail/curl_wrapper.h"
#include "iex/iex.h"

static const iex::Endpoint::OptionsObject kOptions{{}, {}, iex::DataType::SANDBOX};

static const bool kCI = std::getenv("CI") != nullptr;

void Sleep()
{
  if (kCI)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  else
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

#ifdef IEX_ENABLE_STRESS_TESTS

template <iex::Endpoint::Type Type>
const auto kBasicGetFunc = [](const iex::Endpoint::OptionsObject& options,
                              std::mutex& mutex,
                              std::vector<iex::ValueWithErrorCode<bool>>& vec) {
  auto resp = iex::Get<Type>(options);
  std::lock_guard lock(mutex);
  vec.emplace_back(iex::ValueWithErrorCode<bool>{resp.first != nullptr, std::move(resp.second)});
};

template <iex::Endpoint::Type Type>
const auto kSymbolGetFunc = [](const iex::Symbol& symbol, const iex::Endpoint::OptionsObject& options,
                               std::mutex& mutex, std::vector<iex::ValueWithErrorCode<bool>>& vec) {
  auto resp = iex::Get<Type>(symbol, options);
  std::lock_guard lock(mutex);
  vec.emplace_back(iex::ValueWithErrorCode<bool>{resp.first != nullptr, std::move(resp.second)});
};

TEST(Api, Multithread)
{
  constexpr const iex::Endpoint::Type kEStatus = iex::Endpoint::Type::SYSTEM_STATUS;
  constexpr const iex::Endpoint::Type kEQuote = iex::Endpoint::Type::QUOTE;
  constexpr const iex::Endpoint::Type kECompany = iex::Endpoint::Type::COMPANY;

  std::array<iex::Symbol, 5> symbols = {iex::Symbol("tsla"), iex::Symbol("aapl"), iex::Symbol("msft"),
                                        iex::Symbol("amd"), iex::Symbol("intc")};
  std::array<iex::Version, 3> versions = {iex::Version::STABLE, iex::Version::V1, iex::Version::BETA};
  std::array<iex::DataType, 2> data_types = {iex::DataType::AUTHENTIC, iex::DataType::SANDBOX};

  std::mutex return_data_mutex;
  std::vector<iex::ValueWithErrorCode<bool>> return_data;

  std::vector<std::thread> threads;
  for (const auto& version : versions)
  {
    for (const auto& sym : symbols)
    {
      const auto options = iex::Endpoint::OptionsObject{{}, version, iex::DataType::SANDBOX};
      threads.emplace_back(std::thread(kSymbolGetFunc<kEQuote>, sym, iex::Endpoint::OptionsObject{options},
                                       std::ref(return_data_mutex), std::ref(return_data)));
      threads.emplace_back(std::thread(kSymbolGetFunc<kECompany>, sym, iex::Endpoint::OptionsObject{options},
                                       std::ref(return_data_mutex), std::ref(return_data)));
    }

    for (const auto& data_type : data_types)
    {
      const auto options = iex::Endpoint::OptionsObject{{}, version, data_type};
      threads.emplace_back(std::thread(kBasicGetFunc<kEStatus>, iex::Endpoint::OptionsObject{options},
                                       std::ref(return_data_mutex), std::ref(return_data)));
    }
  }

  for (auto& thread : threads)
  {
    thread.join();
  }

  for (const auto& data : return_data)
  {
    EXPECT_TRUE(data.first) << "iex::Get() returned nullptr";
    EXPECT_EQ(data.second, iex::ErrorCode());
  }
}

TEST(Api, IexManualTimeoutStress)
{
  namespace curl = iex::curl;

  const char* tk = getenv("IEX_SANDBOX_SECRET_KEY");
  ASSERT_NE(tk, nullptr);

  const curl::UrlSet urls = {curl::Url("https://sandbox.iexapis.com/stable/stock/aapl/quote?token=" + std::string(tk)),
                             curl::Url("https://sandbox.iexapis.com/stable/stock/tsla/quote?token=" + std::string(tk)),
                             curl::Url("https://sandbox.iexapis.com/stable/stock/amd/quote?token=" + std::string(tk)),
                             curl::Url("https://sandbox.iexapis.com/stable/stock/intc/quote?token=" + std::string(tk)),
                             curl::Url("https://sandbox.iexapis.com/stable/stock/twtr/quote?token=" + std::string(tk))};

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  int failures = 0;
  for (int i = 0; i < 5; ++i)
  {
    for (const auto& url : urls)
    {
      if (curl::Get(url).second.Failure())
      {
        ++failures;
      }
    }
  }

  // Expect at least one to fail due to API rate limiting and not using a timeout.
  ASSERT_GT(failures, 0);

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // Now, do a prolonged stress test to make sure the constant timeout

  const curl::RetryBehavior retry_behavior{3, {iex::kIexHttpTooManyRequests}, true, iex::kIexRequestLimitTimeout};
  std::vector<iex::ErrorCode> ecs;

  for (int i = 0; i < 50; ++i)
  {
    for (const auto& url : urls)
    {
      ecs.emplace_back(curl::Get(url, 1, retry_behavior).second);
      std::this_thread::sleep_for(iex::kIexRequestLimitTimeout);
    }
  }
  for (const auto& ec : ecs)
  {
    EXPECT_EQ(ec, iex::ErrorCode());
  }
}
#endif

TEST(Api, SingleSymbolSingleEndpoint)
{
  Sleep();

  const auto res = iex::Get<iex::Endpoint::QUOTE>(iex::Symbol("tsla"), kOptions);
  ASSERT_EQ(res.second, iex::ErrorCode());

  const auto& quote = res.first;
  EXPECT_NE(quote, nullptr);
}

TEST(Api, SingleSymbolSingleEndpointInvalidSymbol)
{
  Sleep();

  const auto res = iex::Get<iex::Endpoint::QUOTE>(iex::Symbol("aaaaa"), kOptions);
  EXPECT_NE(res.second, iex::ErrorCode());

  const auto& quote = res.first;
  EXPECT_EQ(quote, nullptr);
}

TEST(Api, SingleSymbolMultipleEndpoint)
{
  Sleep();

  const auto res = iex::Get<iex::Endpoint::QUOTE, iex::Endpoint::COMPANY>(iex::Symbol("tsla"), kOptions);
  ASSERT_EQ(res.second, iex::ErrorCode());

  const auto& [quote, company] = res.first;
  EXPECT_NE(quote, nullptr);
  EXPECT_NE(company, nullptr);
}

TEST(Api, SingleSymbolMultipleEndpointInvalidSymbol)
{
  Sleep();

  const auto res = iex::Get<iex::Endpoint::QUOTE, iex::Endpoint::COMPANY>(iex::Symbol("aaaaa"), kOptions);
  EXPECT_NE(res.second, iex::ErrorCode());

  const auto& [quote, company] = res.first;
  EXPECT_EQ(quote, nullptr);
  EXPECT_EQ(company, nullptr);
}

TEST(Api, MultipleSymbolSingleEndpoint)
{
  Sleep();

  const auto res = iex::Get<iex::Endpoint::QUOTE>(iex::SymbolSet{iex::Symbol("tsla"), iex::Symbol("aapl")}, kOptions);
  ASSERT_EQ(res.second, iex::ErrorCode());

  for (const auto& [_, quote] : res.first)
  {
    EXPECT_NE(quote, nullptr);
  }
}

TEST(Api, MultipleSymbolSingleEndpointOneInvalidSymbol)
{
  Sleep();

  const auto success_symbol = iex::Symbol("tsla");
  const auto failure_symbol = iex::Symbol("aaaaa");

  const auto res = iex::Get<iex::Endpoint::QUOTE>(iex::SymbolSet{success_symbol, failure_symbol}, kOptions);
  EXPECT_NE(res.second, iex::ErrorCode());

  for (const auto& [symbol, quote] : res.first)
  {
    if (symbol == success_symbol)
      EXPECT_NE(quote, nullptr);
    else if (symbol == failure_symbol)
      EXPECT_EQ(quote, nullptr);
  }
}

TEST(Api, MultipleSymbolMultipleEndpoint)
{
  Sleep();

  const auto res = iex::Get<iex::Endpoint::QUOTE, iex::Endpoint::COMPANY>(
      iex::SymbolSet{iex::Symbol("tsla"), iex::Symbol("aapl")}, kOptions);
  ASSERT_EQ(res.second, iex::ErrorCode());

  for (const auto& [_, tuple] : res.first)
  {
    const auto& [quote, company] = tuple;
    EXPECT_NE(quote, nullptr);
    EXPECT_NE(company, nullptr);
  }
}

TEST(Api, MultipleSymbolMultipleEndpointOneInvalidSymbol)
{
  Sleep();

  const auto success_symbol = iex::Symbol("tsla");
  const auto failure_symbol = iex::Symbol("aaaaa");

  const auto res =
      iex::Get<iex::Endpoint::QUOTE, iex::Endpoint::COMPANY>(iex::SymbolSet{success_symbol, failure_symbol}, kOptions);
  EXPECT_NE(res.second, iex::ErrorCode());

  for (const auto& [symbol, tuple] : res.first)
  {
    const auto& [quote, company] = tuple;
    if (symbol == success_symbol)
    {
      EXPECT_NE(quote, nullptr);
      EXPECT_NE(company, nullptr);
    }
    else if (symbol == failure_symbol)
    {
      EXPECT_EQ(quote, nullptr);
      EXPECT_EQ(company, nullptr);
    }
  }
}

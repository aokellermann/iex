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

#ifdef IEX_ENABLE_STRESS_TESTS
TEST(Api, Batch)
{
  iex::Symbol sym1("tsla");
  iex::Symbol sym2("aapl");
  constexpr const auto kEp1 = iex::Endpoint::Type::QUOTE;
  constexpr const auto kEp2 = iex::Endpoint::Type::COMPANY;
  const auto opts1 =
      iex::RequestOptions{iex::Endpoint::Options{iex::Quote::DisplayPercentOption()}, {}, iex::DataType::SANDBOX};
  const auto opts2 = iex::RequestOptions{iex::Endpoint::Options{}, {}, iex::DataType::SANDBOX};
  iex::SymbolRequests sreqs;
  iex::Requests reqs = iex::Requests{{kEp1, opts1}, {kEp2, opts2}};
  sreqs.emplace(sym1, reqs);
  sreqs.emplace(sym2, reqs);

  const auto response = iex::Get(sreqs);
  ASSERT_EQ(response.second, iex::ErrorCode());

  const auto sym_ptr1 = response.first.Get(sym1)->Get<kEp1>(opts1);
  const auto sym_ptr2 = response.first.Get(sym2)->Get<kEp2>(opts2);
  EXPECT_NE(sym_ptr1, nullptr);
  EXPECT_NE(sym_ptr2, nullptr);
}

TEST(Api, AggregatedAndBatch)
{
  iex::AggregatedRequests areqs;
  iex::Symbol sym1("tsla");
  iex::Symbol sym2("aapl");
  constexpr const auto kEp1 = iex::Endpoint::Type::QUOTE;
  constexpr const auto kEp2 = iex::Endpoint::Type::COMPANY;
  const auto opts1 =
      iex::RequestOptions{iex::Endpoint::Options{iex::Quote::DisplayPercentOption()}, {}, iex::DataType::SANDBOX};
  const auto opts2 = iex::RequestOptions{iex::Endpoint::Options{}, {}, iex::DataType::SANDBOX};
  iex::Requests reqs = iex::Requests{{kEp1, opts1}, {kEp2, opts2}};
  areqs.symbol_requests.emplace(sym1, reqs);
  areqs.symbol_requests.emplace(sym2, reqs);

  constexpr const auto kEpStatus = iex::Endpoint::Type::SYSTEM_STATUS;
  areqs.requests.emplace(kEpStatus, opts2);

  const auto response = iex::Get(areqs);
  ASSERT_EQ(response.second, iex::ErrorCode());

  const auto sym_ptr1 = response.first.symbol_responses.Get(sym1)->Get<kEp1>(opts1);
  const auto sym_ptr2 = response.first.symbol_responses.Get(sym2)->Get<kEp2>(opts2);
  EXPECT_NE(sym_ptr1, nullptr);
  EXPECT_NE(sym_ptr2, nullptr);

  const auto status_ptr = response.first.responses.Get<kEpStatus>(opts2);
  EXPECT_NE(status_ptr, nullptr);
}

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
  std::vector<iex::ValueWithErrorCode<iex::Responses>> return_data;

  std::mutex return_data_mutex_sym;
  std::vector<iex::ValueWithErrorCode<iex::SymbolResponses>> return_data_sym;

  const auto get_func = [&](const iex::Request& request) {
    auto resp = iex::Get(request);
    std::lock_guard lock(return_data_mutex);
    return_data.emplace_back(std::move(resp));
  };
  const auto get_func_sym = [&](const iex::SymbolRequest& request) {
    auto resp = iex::Get(request);
    std::lock_guard lock(return_data_mutex_sym);
    return_data_sym.emplace_back(std::move(resp));
  };

  std::vector<std::thread> threads;
  for (const auto& version : versions)
  {
    for (const auto& sym : symbols)
    {
      threads.emplace_back(std::thread(
          get_func_sym, iex::SymbolRequest(sym, {kEQuote, iex::RequestOptions{{}, version, iex::DataType::SANDBOX}})));
      threads.emplace_back(
          std::thread(get_func_sym,
                      iex::SymbolRequest(sym, {kECompany, iex::RequestOptions{{}, version, iex::DataType::SANDBOX}})));
    }

    for (const auto& data_type : data_types)
    {
      threads.emplace_back(std::thread(get_func, iex::Request{kEStatus, iex::RequestOptions{{}, version, data_type}}));
    }
  }

  for (auto& thread : threads)
  {
    thread.join();
  }

  for (const auto& data : return_data)
  {
    EXPECT_EQ(data.second, iex::ErrorCode());

    bool found = false;
    for (const auto& version : versions)
    {
      for (const auto& data_type : data_types)
      {
        if (data.first.Get<kEStatus>(iex::RequestOptions{{}, version, data_type}) != nullptr)
        {
          found = true;
          break;
        }
      }
    }
    EXPECT_TRUE(found);
  }

  for (const auto& data : return_data_sym)
  {
    EXPECT_EQ(data.second, iex::ErrorCode());

    bool found = false;
    for (const auto& sym : symbols)
    {
      for (const auto& version : versions)
      {
        for (const auto& data_type : data_types)
        {
          const auto* const sym_ptr = data.first.Get(sym);
          if (sym_ptr != nullptr && (sym_ptr->Get<kEQuote>(iex::RequestOptions{{}, version, data_type}) != nullptr ||
                                     sym_ptr->Get<kECompany>(iex::RequestOptions{{}, version, data_type}) != nullptr))
          {
            found = true;
            break;
          }
        }
      }
    }
    EXPECT_TRUE(found);
  }
}

TEST(Curl, IexManualTimeoutStress)
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

TEST(Api, Tuple)
{
  auto res = iex::Get<iex::Endpoint::QUOTE, iex::Endpoint::COMPANY>(iex::Symbol("tsla"), iex::RequestOptions{});
  ASSERT_EQ(res.second, iex::ErrorCode());

  auto& [quote, company] = res.first;
  EXPECT_NE(quote, nullptr);
  EXPECT_NE(company, nullptr);
}

TEST(Api, TupleMap)
{
  auto res = iex::Get<iex::Endpoint::QUOTE, iex::Endpoint::COMPANY>(iex::SymbolSet{iex::Symbol("tsla"), iex::Symbol("aapl")}, iex::RequestOptions{});
  ASSERT_EQ(res.second, iex::ErrorCode());

  for (const auto& [symbol, tuple] : res.first)
  {
    const auto& [quote, company] = tuple;
    EXPECT_NE(quote, nullptr);
    EXPECT_NE(company, nullptr);
  }
}

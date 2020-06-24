/**
 * @file api_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex/api.h"

#include <gtest/gtest.h>

#include <mutex>
#include <thread>
#include <vector>

#include "iex/api/company.h"
#include "iex/api/quote.h"
#include "iex/api/system_status.h"

namespace api = iex::api;

TEST(Api, AggregatedAndBatch)
{
  api::AggregatedRequests areqs;
  api::Symbol sym1("tsla");
  api::Symbol sym2("aapl");
  constexpr const auto kEp1 = api::Endpoint::Type::QUOTE;
  constexpr const auto kEp2 = api::Endpoint::Type::COMPANY;
  const auto opts1 =
      api::RequestOptions{api::Endpoint::Options{api::Quote::DisplayPercentOption()}, {}, api::DataType::SANDBOX};
  const auto opts2 = api::RequestOptions{api::Endpoint::Options{}, {}, api::DataType::SANDBOX};
  api::Requests reqs = api::Requests{{kEp1, opts1}, {kEp2, opts2}};
  areqs.symbol_requests.emplace(sym1, reqs);
  areqs.symbol_requests.emplace(sym2, reqs);

  constexpr const auto kEpStatus = api::Endpoint::Type::SYSTEM_STATUS;
  areqs.requests.emplace(kEpStatus, opts2);

  const auto response = api::Get(areqs);
  ASSERT_EQ(response.second, iex::ErrorCode());

  const auto sym_ptr1 = response.first.symbol_responses.Get(sym1)->Get<kEp1>(opts1);
  const auto sym_ptr2 = response.first.symbol_responses.Get(sym2)->Get<kEp2>(opts2);
  EXPECT_NE(sym_ptr1, nullptr);
  EXPECT_NE(sym_ptr2, nullptr);

  const auto status_ptr = response.first.responses.Get<kEpStatus>(opts2);
  EXPECT_NE(status_ptr, nullptr);
}

TEST(Api, Batch)
{
  api::Symbol sym1("tsla");
  api::Symbol sym2("aapl");
  constexpr const auto kEp1 = api::Endpoint::Type::QUOTE;
  constexpr const auto kEp2 = api::Endpoint::Type::COMPANY;
  const auto opts1 =
      api::RequestOptions{api::Endpoint::Options{api::Quote::DisplayPercentOption()}, {}, api::DataType::SANDBOX};
  const auto opts2 = api::RequestOptions{api::Endpoint::Options{}, {}, api::DataType::SANDBOX};
  api::SymbolRequests sreqs;
  api::Requests reqs = api::Requests{{kEp1, opts1}, {kEp2, opts2}};
  sreqs.emplace(sym1, reqs);
  sreqs.emplace(sym2, reqs);

  const auto response = api::Get(sreqs);
  ASSERT_EQ(response.second, iex::ErrorCode());

  const auto sym_ptr1 = response.first.Get(sym1)->Get<kEp1>(opts1);
  const auto sym_ptr2 = response.first.Get(sym2)->Get<kEp2>(opts2);
  EXPECT_NE(sym_ptr1, nullptr);
  EXPECT_NE(sym_ptr2, nullptr);
}

TEST(Api, Multithread)
{
  constexpr const api::Endpoint::Type kEStatus = api::Endpoint::Type::SYSTEM_STATUS;
  constexpr const api::Endpoint::Type kEQuote = api::Endpoint::Type::QUOTE;
  constexpr const api::Endpoint::Type kECompany = api::Endpoint::Type::COMPANY;

  std::array<api::Symbol, 5> symbols = {api::Symbol("tsla"), api::Symbol("aapl"), api::Symbol("msft"),
                                        api::Symbol("amd"), api::Symbol("intc")};
  std::array<api::Version, 3> versions = {api::Version::STABLE, api::Version::V1, api::Version::BETA};
  std::array<api::DataType, 2> data_types = {api::DataType::AUTHENTIC, api::DataType::SANDBOX};

  std::mutex return_data_mutex;
  std::vector<iex::ValueWithErrorCode<api::Responses>> return_data;

  std::mutex return_data_mutex_sym;
  std::vector<iex::ValueWithErrorCode<api::SymbolResponses>> return_data_sym;

  const auto get_func = [&](const api::Request& request) {
    auto resp = api::Get(request);
    std::lock_guard lock(return_data_mutex);
    return_data.emplace_back(std::move(resp));
  };
  const auto get_func_sym = [&](const api::SymbolRequest& request) {
    auto resp = api::Get(request);
    std::lock_guard lock(return_data_mutex_sym);
    return_data_sym.emplace_back(std::move(resp));
  };

  std::vector<std::thread> threads;
  for (const auto& version : versions)
  {
    for (const auto& sym : symbols)
    {
      threads.emplace_back(std::thread(
          get_func_sym, api::SymbolRequest(sym, {kEQuote, api::RequestOptions{{}, version, api::DataType::SANDBOX}})));
      threads.emplace_back(
          std::thread(get_func_sym,
                      api::SymbolRequest(sym, {kECompany, api::RequestOptions{{}, version, api::DataType::SANDBOX}})));
    }

    for (const auto& data_type : data_types)
    {
      threads.emplace_back(std::thread(get_func, api::Request{kEStatus, api::RequestOptions{{}, version, data_type}}));
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
        if (data.first.Get<kEStatus>(api::RequestOptions{{}, version, data_type}) != nullptr)
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
          const auto *const sym_ptr = data.first.Get(sym);
          if (sym_ptr != nullptr &&
              (sym_ptr->Get<kEQuote>(api::RequestOptions{{}, version, data_type}) != nullptr ||
              sym_ptr->Get<kECompany>(api::RequestOptions{{}, version, data_type}) != nullptr))
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

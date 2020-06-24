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
#include "iex/iex.h"

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
          const auto *const sym_ptr = data.first.Get(sym);
          if (sym_ptr != nullptr &&
              (sym_ptr->Get<kEQuote>(iex::RequestOptions{{}, version, data_type}) != nullptr ||
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

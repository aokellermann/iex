/**
 * @file quote_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex/api/quote.h"

#include <gtest/gtest.h>

#include "iex/api.h"

namespace api = iex::api;

TEST(Quote, GetWithoutTemplate)
{
  api::Symbol sym("tsla");
  const auto opts =
      api::RequestOptions{api::Endpoint::Options{api::Quote::DisplayPercentOption()}, {}, api::DataType::SANDBOX};
  api::SymbolRequest req(sym, api::Request{api::Endpoint::Type::QUOTE, opts});
  auto response = api::Get(req);
  EXPECT_EQ(response.second, iex::ErrorCode());
  ASSERT_TRUE(response.second.Success());

  const auto* const sym_ptr = response.first.Get(sym);
  ASSERT_NE(sym_ptr, nullptr);

  const auto end_ptr = sym_ptr->Get<api::Endpoint::Type::QUOTE>(opts);
  EXPECT_NE(end_ptr, nullptr);
}

TEST(Quote, GetWithTemplate)
{
  auto response =
      api::Get<api::Endpoint::Type::QUOTE>(api::Symbol("tsla"), api::RequestOptions{{}, {}, api::DataType::SANDBOX});
  EXPECT_EQ(response.second, iex::ErrorCode());
  ASSERT_TRUE(response.second.Success());

  const auto a = response.first;
  EXPECT_NE(a, nullptr);
}

TEST(Quote, AllFields)
{
  const char* json_s =
      "{\n"
      "  \"symbol\": \"AAPL\",\n"
      "  \"companyName\": \"Apple, Inc.\",\n"
      "  \"primaryExchange\": \"NASDAQ\",\n"
      "  \"calculationPrice\": \"tops\",\n"
      "  \"open\": 350.25,\n"
      "  \"openTime\": 1592320822921,\n"
      "  \"openSource\": \"official\",\n"
      "  \"close\": 350.25,\n"
      "  \"closeTime\": 1592320822921,\n"
      "  \"closeSource\": \"official\",\n"
      "  \"high\": 350.25,\n"
      "  \"highTime\": 1592320822921,\n"
      "  \"highSource\": \"15 minute delayed price\",\n"
      "  \"low\": 350.25,\n"
      "  \"lowTime\": 1592319692468,\n"
      "  \"lowSource\": \"15 minute delayed price\",\n"
      "  \"latestPrice\": 350.25,\n"
      "  \"latestSource\": \"IEX real time price\",\n"
      "  \"latestTime\": \"11:35:05 AM\",\n"
      "  \"latestUpdate\": 1592321705202,\n"
      "  \"latestVolume\": 20567140,\n"
      "  \"iexRealtimePrice\": 350.25,\n"
      "  \"iexRealtimeSize\": 27,\n"
      "  \"iexLastUpdated\": 1592321705202,\n"
      "  \"delayedPrice\": 350.25,\n"
      "  \"delayedPriceTime\": 1592321705202,\n"
      "  \"oddLotDelayedPrice\": 350.25,\n"
      "  \"oddLotDelayedPriceTime\": 1592321705202,\n"
      "  \"extendedPrice\": 350.25,\n"
      "  \"extendedChange\": 350.25,\n"
      "  \"extendedChangePercent\": 0.02117,\n"
      "  \"extendedPriceTime\": 1592321705202,\n"
      "  \"previousClose\": 342.99,\n"
      "  \"previousVolume\": 34702230,\n"
      "  \"change\": 7.26,\n"
      "  \"changePercent\": 0.02117,\n"
      "  \"volume\": 110533,\n"
      "  \"iexMarketPercent\": 0.006061888374230089,\n"
      "  \"iexVolume\": 110533,\n"
      "  \"avgTotalVolume\": 34010007,\n"
      "  \"iexBidPrice\": 333,\n"
      "  \"iexBidSize\": 100,\n"
      "  \"iexAskPrice\": 356,\n"
      "  \"iexAskSize\": 100,\n"
      "  \"iexOpen\": 350.25,\n"
      "  \"iexOpenTime\": 1592321705202,\n"
      "  \"iexClose\": 350.25,\n"
      "  \"iexCloseTime\": 1592321705202,\n"
      "  \"marketCap\": 1518102585000,\n"
      "  \"peRatio\": 27.23,\n"
      "  \"week52High\": 354.77,\n"
      "  \"week52Low\": 190.3,\n"
      "  \"ytdChange\": 0.17897,\n"
      "  \"lastTradeTime\": 1592321705202,\n"
      "  \"isUSMarketOpen\": true\n"
      "}";

  iex::json::Json json = iex::json::Json::parse(json_s);
  api::Quote quote(iex::json::JsonStorage{json});

  using MemberType = api::Quote::MemberType;
  EXPECT_TRUE(quote.Get<MemberType::COMPANY_NAME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::PRIMARY_EXCHANGE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::CALCULATION_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::OPEN_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::OPEN_TIME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::OPEN_SOURCE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::CLOSE_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::CLOSE_TIME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::CLOSE_SOURCE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::HIGH_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::HIGH_TIME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::HIGH_SOURCE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::LOW_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::LOW_TIME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::LOW_SOURCE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::LATEST_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::LATEST_UPDATE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::LATEST_SOURCE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::LATEST_VOLUME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_REALTIME_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_REALTIME_SIZE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_LAST_UPDATED>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::DELAYED_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::DELAYED_TIME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::ODD_LOT_DELAYED_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::ODD_LOT_DELAYED_TIME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::EXTENDED_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::EXTENDED_TIME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::EXTENDED_CHANGE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::EXTENDED_CHANGE_PERCENT>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::PREVIOUS_CLOSE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::PREVIOUS_VOLUME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::CHANGE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::CHANGE_PERCENT>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::VOLUME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_MARKET_PERCENT>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_VOLUME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::AVERAGE_TOTAL_VOLUME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_BID_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_BID_SIZE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_ASK_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_ASK_SIZE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_OPEN_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_OPEN_TIME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_CLOSE_PRICE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IEX_CLOSE_TIME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::MARKET_CAP>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::PE_RATIO>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::WEEK_52_HIGH>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::WEEK_52_LOW>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::YTD_CHANGE>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::LAST_TRADE_TIME>().has_value());
  EXPECT_TRUE(quote.Get<MemberType::IS_US_MARKET_OPEN>().has_value());
}

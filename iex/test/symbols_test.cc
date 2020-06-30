/**
 * @file symbols_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex/api/symbols.h"

#include <gtest/gtest.h>

#include <vector>

#include "iex/iex.h"

const std::vector<const char*> kSymbols = {
    "aapl", "msft", "tsla", "intc", "amd", "bynd", "aig+", "brk.a", "ver-f", "mj", "qqq", "arkq", "robo",
};

TEST(Symbols, GetWithoutTemplate)
{
  iex::RequestOptions opts = iex::RequestOptions{{}, {}, iex::SANDBOX};
  iex::Request req{iex::Endpoint::Type::SYMBOLS, opts};
  auto response = iex::Get(req);
  EXPECT_EQ(response.second, iex::ErrorCode());
  ASSERT_TRUE(response.second.Success());

  const auto a = response.first.Get<iex::Endpoint::Type::SYMBOLS>(opts);
  ASSERT_NE(a, nullptr);

  for (const auto& sym : kSymbols)
  {
    const auto member = a->Get(iex::Symbol(sym));
    ASSERT_TRUE(member.has_value());
    EXPECT_TRUE(member->Get<iex::Symbols::MemberType::NAME>().has_value());
  }
}

TEST(Symbols, GetWithTemplate)
{
  iex::RequestOptions opts = iex::RequestOptions{{}, {}, iex::SANDBOX};
  auto response = iex::Get<iex::Endpoint::Type::SYMBOLS>(opts);
  EXPECT_EQ(response.second, iex::ErrorCode());
  ASSERT_TRUE(response.second.Success());

  const auto a = response.first;
  ASSERT_NE(a, nullptr);

  for (const auto& sym : kSymbols)
  {
    const auto member = a->Get(iex::Symbol(sym));
    ASSERT_TRUE(member.has_value());
    EXPECT_TRUE(member->Get<iex::Symbols::MemberType::NAME>().has_value());
  }
}

TEST(Symbols, AllFields)
{
  const char* json_s =
      "[\n"
      "  {\n"
      "    \"symbol\": \"A\",\n"
      "    \"exchange\": \"NYS\",\n"
      "    \"name\": \"Agilent Technologies Inc.\",\n"
      "    \"date\": \"2020-06-24\",\n"
      "    \"type\": \"cs\",\n"
      "    \"iexId\": \"IEX_46574843354B2D52\",\n"
      "    \"region\": \"US\",\n"
      "    \"currency\": \"USD\",\n"
      "    \"isEnabled\": true,\n"
      "    \"figi\": \"BBG000C2V3D6\",\n"
      "    \"cik\": \"1090872\"\n"
      "  }\n"
      "]";

  iex::json::Json json = iex::json::Json::parse(json_s);
  iex::Symbols symbols(iex::json::JsonStorage{json});
  const auto symbol = symbols.Get(iex::Symbol("A"));
  ASSERT_TRUE(symbol.has_value());

  using MemberType = iex::Symbols::MemberType;
  EXPECT_TRUE(symbol->Get<MemberType::EXCHANGE>().has_value());
  EXPECT_TRUE(symbol->Get<MemberType::NAME>().has_value());
  EXPECT_TRUE(symbol->Get<MemberType::DATE>().has_value());
  EXPECT_TRUE(symbol->Get<MemberType::TYPE>().has_value());
  EXPECT_TRUE(symbol->Get<MemberType::IEX_ID>().has_value());
  EXPECT_TRUE(symbol->Get<MemberType::REGION>().has_value());
  EXPECT_TRUE(symbol->Get<MemberType::CURRENCY>().has_value());
  EXPECT_TRUE(symbol->Get<MemberType::IS_ENABLED>().has_value());
  EXPECT_TRUE(symbol->Get<MemberType::FIGI>().has_value());
  EXPECT_TRUE(symbol->Get<MemberType::CIK>().has_value());
}
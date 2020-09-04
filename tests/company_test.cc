/**
 * @file company_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include "iex/iex.h"

static const iex::Endpoint::OptionsObject kOptions{{}, {}, iex::DataType::SANDBOX};

TEST(Company, Get)
{
  const auto company = iex::Get<iex::Endpoint::Type::COMPANY>(iex::Symbol("tsla"), kOptions);
  EXPECT_NE(company, nullptr);
}

TEST(Company, AllFields)
{
  const char* json_s =
      "{\n"
      "  \"symbol\": \"AAPL\",\n"
      "  \"companyName\": \"Apple, Inc.\",\n"
      "  \"exchange\": \"NASDAQ\",\n"
      "  \"industry\": \"Telecommunications Equipment\",\n"
      "  \"website\": \"http://www.apple.com\",\n"
      "  \"description\": \"Apple, Inc. engages in the design, manufacture, and sale of smartphones, personal "
      "computers, tablets, wearables and accessories, and other variety of related services. It operates through the "
      "following geographical segments: Americas, Europe, Greater China, Japan, and Rest of Asia Pacific. The Americas "
      "segment includes North and South America. The Europe segment consists of European countries, as well as India, "
      "the Middle East, and Africa. The Greater China segment comprises of China, Hong Kong, and Taiwan. The Rest of "
      "Asia Pacific segment includes Australia and Asian countries. Its products and services include iPhone, Mac, "
      "iPad, AirPods, Apple TV, Apple Watch, Beats products, Apple Care, iCloud, digital content stores, streaming, "
      "and licensing services. The company was founded by Steven Paul Jobs, Ronald Gerald Wayne, and Stephen G. "
      "Wozniak on April 1, 1976 and is headquartered in Cupertino, CA.\",\n"
      "  \"CEO\": \"Timothy Donald Cook\",\n"
      "  \"securityName\": \"Apple Inc.\",\n"
      "  \"issueType\": \"cs\",\n"
      "  \"sector\": \"Electronic Technology\",\n"
      "  \"primarySicCode\": 3663,\n"
      "  \"employees\": 137000,\n"
      "  \"tags\": [\n"
      "    \"Electronic Technology\",\n"
      "    \"Telecommunications Equipment\"\n"
      "  ],\n"
      "  \"address\": \"One Apple Park Way\",\n"
      "  \"address2\": \"One Apple Park Way2\",\n"
      "  \"state\": \"CA\",\n"
      "  \"city\": \"Cupertino\",\n"
      "  \"zip\": \"95014-2083\",\n"
      "  \"country\": \"US\",\n"
      "  \"phone\": \"1.408.996.1010\"\n"
      "}";

  iex::json::Json json = iex::json::Json::parse(json_s);
  iex::Company company(iex::json::JsonStorage{json});

  using MemberType = iex::Company::MemberType;
  EXPECT_TRUE(company.Get<MemberType::COMPANY_NAME>().has_value());
  EXPECT_TRUE(company.Get<MemberType::EXCHANGE>().has_value());
  EXPECT_TRUE(company.Get<MemberType::INDUSTRY>().has_value());
  EXPECT_TRUE(company.Get<MemberType::WEBSITE>().has_value());
  EXPECT_TRUE(company.Get<MemberType::DESCRIPTION>().has_value());
  EXPECT_TRUE(company.Get<MemberType::CEO>().has_value());
  EXPECT_TRUE(company.Get<MemberType::SECURITY_NAME>().has_value());
  EXPECT_TRUE(company.Get<MemberType::ISSUE_TYPE>().has_value());
  EXPECT_TRUE(company.Get<MemberType::SECTOR>().has_value());
  EXPECT_TRUE(company.Get<MemberType::PRIMARY_SIC_CODE>().has_value());
  EXPECT_TRUE(company.Get<MemberType::EMPLOYEES>().has_value());
  EXPECT_TRUE(company.Get<MemberType::TAGS>().has_value() && !company.Get<MemberType::TAGS>().value().empty());
  EXPECT_TRUE(company.Get<MemberType::ADDRESS>().has_value());
  EXPECT_TRUE(company.Get<MemberType::ADDRESS_2>().has_value());
  EXPECT_TRUE(company.Get<MemberType::STATE>().has_value());
  EXPECT_TRUE(company.Get<MemberType::CITY>().has_value());
  EXPECT_TRUE(company.Get<MemberType::ZIP>().has_value());
  EXPECT_TRUE(company.Get<MemberType::COUNTRY>().has_value());
  EXPECT_TRUE(company.Get<MemberType::PHONE>().has_value());
}
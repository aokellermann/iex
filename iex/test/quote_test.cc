/**
 * @file quote_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include "iex/api.h"

namespace api = iex::api;
/*+

TEST(Quote, GetWithoutTemplate)
{
  api::Request req{api::Endpoint::Type::QUOTE};
  auto response = api::Get(req);
  EXPECT_EQ(response.second, iex::ErrorCode());
  ASSERT_TRUE(response.second.Success());

  const auto a = response.first.Get<api::Endpoint::Type::QUOTE>();
  ASSERT_NE(a, nullptr);
}

TEST(Quote, GetWithTemplate)
{
  auto response = api::Get<api::Endpoint::Type::QUOTE>();
  EXPECT_EQ(response.second, iex::ErrorCode());
  ASSERT_TRUE(response.second.Success());

  const auto a = response.first;
  ASSERT_NE(a, nullptr);
}
 */

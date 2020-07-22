/**
 * @file system_status_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include "iex/iex.h"

TEST(SystemStatus, GetWithoutTemplate)
{
  iex::Request req{iex::Endpoint::Type::SYSTEM_STATUS};
  auto response = iex::Get(req);
  EXPECT_EQ(response.second, iex::ErrorCode());
  ASSERT_TRUE(response.second.Success());

  const auto a = response.first.Get<iex::Endpoint::Type::SYSTEM_STATUS>();
  ASSERT_NE(a, nullptr);

  EXPECT_EQ(a->Get<iex::SystemStatus::MemberType::STATUS>().value(), "up") << "iex must be up in order to run testing";
  EXPECT_FALSE(a->Get<iex::SystemStatus::MemberType::VERSION>().value().empty());
  EXPECT_GT(a->Get<iex::SystemStatus::MemberType::TIMESTAMP>().value().count(), 0);
  EXPECT_GT(a->Get<iex::SystemStatus::MemberType::CURRENT_MONTH_API_CALLS>().value(), 0);
}

TEST(SystemStatus, GetWithTemplate)
{
  auto response = iex::Get<iex::Endpoint::Type::SYSTEM_STATUS>();
  EXPECT_EQ(response.second, iex::ErrorCode());
  ASSERT_TRUE(response.second.Success());

  const auto a = response.first;
  ASSERT_NE(a, nullptr);

  EXPECT_EQ(a->Get<iex::SystemStatus::MemberType::STATUS>().value(), "up") << "iex must be up in order to run testing";
  EXPECT_FALSE(a->Get<iex::SystemStatus::MemberType::VERSION>().value().empty());
  EXPECT_GT(a->Get<iex::SystemStatus::MemberType::TIMESTAMP>().value().count(), 0);
  EXPECT_GT(a->Get<iex::SystemStatus::MemberType::CURRENT_MONTH_API_CALLS>().value(), 0);
}

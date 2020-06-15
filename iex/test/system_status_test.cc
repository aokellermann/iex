/**
 * @file system_status_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex/api/system_status.h"

#include <gtest/gtest.h>

#include "iex/api.h"

namespace api = iex::api;

TEST(SystemStatus, GetWithoutTemplate)
{
  api::Request req{api::Endpoint::Type::SYSTEM_STATUS};
  auto response = api::Get(req);
  EXPECT_EQ(response.second, iex::ErrorCode());
  ASSERT_TRUE(response.second.Success());

  const auto a = response.first.Get<api::Endpoint::Type::SYSTEM_STATUS>();
  ASSERT_NE(a, nullptr);

  EXPECT_EQ(a->Get<api::SystemStatus::MemberType::STATUS>().value(), "up") << "API must be up in order to run testing";
  EXPECT_FALSE(a->Get<api::SystemStatus::MemberType::VERSION>().value().empty());
  EXPECT_GT(a->Get<api::SystemStatus::MemberType::TIMESTAMP>().value().count(), 0);
  EXPECT_GT(a->Get<api::SystemStatus::MemberType::CURRENT_MONTH_API_CALLS>().value(), 0);
}

TEST(SystemStatus, GetWithTemplate)
{
  auto response = api::Get<api::Endpoint::Type::SYSTEM_STATUS>();
  EXPECT_EQ(response.second, iex::ErrorCode());
  ASSERT_TRUE(response.second.Success());

  const auto a = response.first;
  ASSERT_NE(a, nullptr);

  EXPECT_EQ(a->Get<api::SystemStatus::MemberType::STATUS>().value(), "up") << "API must be up in order to run testing";
  EXPECT_FALSE(a->Get<api::SystemStatus::MemberType::VERSION>().value().empty());
  EXPECT_GT(a->Get<api::SystemStatus::MemberType::TIMESTAMP>().value().count(), 0);
  EXPECT_GT(a->Get<api::SystemStatus::MemberType::CURRENT_MONTH_API_CALLS>().value(), 0);
}

/**
 * @file system_status_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include "iex/iex.h"

TEST(SystemStatus, Get)
{
  const auto status = iex::Get<iex::Endpoint::Type::SYSTEM_STATUS>();
  ASSERT_NE(status, nullptr);

  EXPECT_EQ(status->Get<iex::SystemStatus::MemberType::STATUS>().value(), "up")
      << "iex must be up in order to run testing";
  EXPECT_FALSE(status->Get<iex::SystemStatus::MemberType::VERSION>().value().empty());
  EXPECT_GT(status->Get<iex::SystemStatus::MemberType::TIMESTAMP>().value().count(), 0);
  EXPECT_GT(status->Get<iex::SystemStatus::MemberType::CURRENT_MONTH_API_CALLS>().value(), 0);
}

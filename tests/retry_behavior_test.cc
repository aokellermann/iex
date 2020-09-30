/**
 * @file retry_behavior_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include <unordered_set>

#include "iex/iex.h"

TEST(RetryBehavior, GetSet)
{
  auto default_behavior = iex::GetRetryBehavior();
  auto retries = 7832;
  auto resps = std::unordered_set<iex::curl::HttpResponseCode>{404, 123, 429};
  auto empty = true;
  auto timeout = iex::curl::TimeoutDuration(72);
  iex::SetRetryBehavior(iex::curl::RetryBehavior{retries, resps, empty, timeout});

  const auto& new_behavior = iex::GetRetryBehavior();
  EXPECT_EQ(new_behavior.max_retries, retries);
  EXPECT_EQ(new_behavior.responses_to_retry, resps);
  EXPECT_EQ(new_behavior.retry_if_empty_response_data, empty);
  EXPECT_EQ(new_behavior.timeout, timeout);

  // Put default back
  iex::SetRetryBehavior(std::move(default_behavior));
}

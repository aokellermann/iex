/**
 * @file unit_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include "iex/iex.h"

int main(int argc, char** argv)
{
  iex::Keys keys;
  keys.public_key = getenv("IEX_PUBLIC_KEY");
  keys.secret_key = getenv("IEX_SECRET_KEY");
  keys.public_sandbox_key = getenv("IEX_SANDBOX_PUBLIC_KEY");
  keys.secret_sandbox_key = getenv("IEX_SANDBOX_SECRET_KEY");
  const auto ec = iex::Init(std::move(keys));
  if (ec.Failure())
  {
    return 1;
  }

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

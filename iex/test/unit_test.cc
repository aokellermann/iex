/**
 * Copyright 2020 Antony Kellermann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <gtest/gtest.h>

#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include "iex/env.h"
#include "iex/iex.h"
#include "iex/singleton.h"

namespace env = iex::env;

TEST(iex, init_test)
{
  const auto ec = iex::Init();
  EXPECT_TRUE(ec.Success());
}

/**
 * Test whether singleton works properly.
 * Creates a bunch of the same class using singleton::GetInstance(), but each with a different identifier.
 * Then, puts them into a set, using their id as a hash.
 * If singleton works properly, the set will be of size 1.
 */
TEST(Singleton, SingletonUnique)
{
  struct SingletonImpl
  {
    int i;
    bool operator==(const SingletonImpl& other) const { return i == other.i; }
  };

  struct SingletonHasher
  {
    std::size_t operator()(const SingletonImpl& s) const { return std::hash<int>()(s.i); }
  };

  std::unordered_set<SingletonImpl, SingletonHasher> set;
  std::mutex mutex;

  const auto thread_func = [&set, &mutex](int i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Try to induce pileup
    const auto& instance = iex::singleton::GetInstance<SingletonImpl>();
    std::lock_guard<std::mutex> lock(mutex);
    set.insert(instance);
  };

  const std::size_t num_threads = 1000;
  std::vector<std::thread> threads(num_threads);
  for (int i = 0; i < num_threads; ++i)
  {
    threads[i] = std::thread(thread_func, i);
  }

  for (int i = 0; i < num_threads; ++i)
  {
    threads[i].join();
  }

  EXPECT_EQ(set.size(), 1);
}

TEST(Env, GetHome)
{
  const auto get_res_success = env::GetEnv("HOME");
  EXPECT_NE(get_res_success.first, "");
  EXPECT_EQ(get_res_success.second, iex::ErrorCode());
}

TEST(Env, SetGetUnset)
{
  EXPECT_EQ(env::SetEnv("IEX_TEST", "TEST_VAR_VALUE"), "");

  const auto get_res_success = env::GetEnv("IEX_TEST");
  EXPECT_EQ(get_res_success.first, "TEST_VAR_VALUE");
  EXPECT_EQ(get_res_success.second, iex::ErrorCode());

  const auto unset_ec = env::UnsetEnv("IEX_TEST");
  EXPECT_EQ(unset_ec, iex::ErrorCode());

  const auto get_res_failure = env::GetEnv("IEX_TEST");
  EXPECT_EQ(get_res_failure.first, "");
  EXPECT_NE(get_res_failure.second, iex::ErrorCode());
}

TEST(Env, IllegalNamesAndValues)
{
  const auto get_res_empty = env::GetEnv("");
  EXPECT_EQ(get_res_empty.first, "");
  EXPECT_NE(get_res_empty.second, iex::ErrorCode());

  const auto get_res_equal_char = env::GetEnv("test_=_test");
  EXPECT_EQ(get_res_equal_char.first, "");
  EXPECT_NE(get_res_equal_char.second, iex::ErrorCode());

  EXPECT_NE(env::SetEnv("", "TEST_VAR_VALUE"), iex::ErrorCode());
  EXPECT_NE(env::SetEnv("TEST_VAR_NAME", ""), iex::ErrorCode());
  EXPECT_NE(env::SetEnv("", ""), iex::ErrorCode());

  EXPECT_NE(env::SetEnv("test_=_test", "TEST_VAR_VALUE"), iex::ErrorCode());
  EXPECT_NE(env::SetEnv("TEST_VAR_NAME", "test_=_test"), iex::ErrorCode());
  EXPECT_NE(env::SetEnv("test_=_test", "test_=_test"), iex::ErrorCode());

  EXPECT_NE(env::UnsetEnv(""), iex::ErrorCode());
  EXPECT_NE(env::UnsetEnv("test_=_test"), iex::ErrorCode());
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

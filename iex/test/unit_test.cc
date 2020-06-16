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

#include "iex/api.h"
#include "iex/keychain.h"
#include "iex/common.h"
#include "iex/env.h"
#include "iex/singleton.h"

namespace env = iex::env;

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
    explicit SingletonImpl(int num) : i(num) {}

    explicit SingletonImpl(int num, double) : i(num) {}

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
    const auto& instance =
        i % 2 == 0 ? iex::singleton::GetInstance<SingletonImpl>(i) : iex::singleton::GetInstance<SingletonImpl>(i, 1.5);
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

TEST(Singleton, Variadic)
{
  struct A
  {
    A() = default;
    A(int) {}
    A(double, int) {}
  };

  const A& x = iex::singleton::GetInstance<A>();
  const A& xx = iex::singleton::GetInstance<A>();
  const A& y = iex::singleton::GetInstance<A>(0);
  const A& z = iex::singleton::GetInstance<A>(0.5, 1);
  ASSERT_EQ(std::addressof(x), std::addressof(xx));
  EXPECT_EQ(std::addressof(x), std::addressof(y));
  EXPECT_EQ(std::addressof(x), std::addressof(z));
  EXPECT_EQ(std::addressof(y), std::addressof(z));
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
  const auto ec = iex::api::Init(iex::api::key::Keychain::EnvironmentFlag());
  if (ec.Failure())
  {
    return 1;
  }

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

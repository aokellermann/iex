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

#include "iex/detail/common.h"
#include "iex/detail/singleton.h"
#include "iex/iex.h"

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

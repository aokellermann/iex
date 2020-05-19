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

#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>  // NOLINT Why does linter think this is a C library?
#include "iex/iex.h"
#include "iex/singleton.h"
#include "iex/curl_wrapper.h"

namespace curl = iex::curl;

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
TEST(singleton, singleton_unique)
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

  const std::size_t num_threads = 10;
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

TEST(url, correct_encoding)
{
  curl::Url url("https://google.com/path?foo=1&bar=2&plus=+");
  EXPECT_EQ("https%3A%2F%2Fgoogle.com%2Fpath%3Ffoo%3D1%26bar%3D2%26plus%3D%2B", url.GetAsString());
}

TEST(url, invalid_url)
{
  curl::Url url("");
  EXPECT_TRUE(url.Valid().Failure());
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

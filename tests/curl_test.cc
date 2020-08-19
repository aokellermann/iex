/**
 * @file curl_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include <initializer_list>
#include <thread>

#include "iex/detail/common.h"
#include "iex/detail/curl_wrapper.h"

namespace curl = iex::curl;

using ParamInit = std::vector<iex::KVP<std::string>>;
const ParamInit kEmptyParams = {};
const ParamInit kInvalidName = {{"foo1", "bar1"}, {"", "bar2"}};
const ParamInit kInvalidValue = {{"foo1", "bar1"}, {"foo2", ""}};
const ParamInit kValidParams = {{"foo1", "bar1"}, {"foo2", "bar2"}};
const ParamInit kValidParams2 = {{"foo3", "bar3"}, {"foo4", "bar4"}};
const ParamInit kEncodeParams = {{"foo1", "bar1"}, {"foo2", "bar+"}};

curl::Url::Params GetUrlParams(const ParamInit& params_init)
{
  curl::Url::Params params;
  for (const auto& kvp : params_init) params.emplace(kvp);
  return params;
}

const char* postman_echo_get_base = "https://postman-echo.com/get";

struct UrlInitParams
{
  const char* base;
  const ParamInit* params;
  bool valid;
  const char* encoded_url;
};

class Url : public testing::TestWithParam<UrlInitParams>
{
 protected:
  ~Url() override { delete url_; }

  void Construct()
  {
    url_ = GetParam().params ? new curl::Url(GetParam().base, GetUrlParams(*GetParam().params))
                             : new curl::Url(GetParam().base);
  }

  curl::Url* url_{nullptr};
};

TEST_P(Url, CorrectValidity)
{
  if (GetParam().valid)
    EXPECT_NO_THROW(Construct());
  else
    EXPECT_THROW(Construct(), curl::InvalidUrlError);
}

TEST_P(Url, Encoding)
{
  if (GetParam().valid)
  {
    EXPECT_NO_THROW({
      Construct();
      EXPECT_EQ(std::string(*url_), GetParam().encoded_url);
    });
  }
}

INSTANTIATE_TEST_CASE_P(Correctness,
                        Url,
                        testing::Values(UrlInitParams{"", nullptr, false},
                                        UrlInitParams{"", &kEmptyParams, false},
                                        UrlInitParams{"", &kValidParams, false},
                                        UrlInitParams{"base", &kInvalidName, false},
                                        UrlInitParams{"base", &kInvalidValue, false},
                                        UrlInitParams{"base", nullptr, true, "base"},
                                        UrlInitParams{"base", &kEmptyParams, true, "base"},
                                        UrlInitParams{"base", &kValidParams, true, "base?foo1=bar1&foo2=bar2"},
                                        UrlInitParams{"base", &kEncodeParams, true, "base?foo1=bar1&foo2=bar%2B"}));

TEST(Curl, Single)
{
  curl::Url url(postman_echo_get_base, GetUrlParams(kValidParams));
  curl::Json expected_response;
  expected_response["foo1"] = "bar1";
  expected_response["foo2"] = "bar2";

  const auto data = curl::Get(url);
  EXPECT_EQ(data["args"].dump(4), expected_response.dump(4));
}

TEST(Curl, Double)
{
  curl::Url url(postman_echo_get_base, GetUrlParams(kValidParams));
  curl::Url url2(postman_echo_get_base, GetUrlParams(kValidParams2));

  const auto urls = {url, url2};

  curl::Json expected_response_first;
  curl::Json expected_response_second;
  expected_response_first["foo1"] = "bar1";
  expected_response_first["foo2"] = "bar2";
  expected_response_second["foo3"] = "bar3";
  expected_response_second["foo4"] = "bar4";

  const auto data = curl::Get(urls.begin(), urls.end());

  ASSERT_TRUE(data.find(url) != data.end());
  ASSERT_TRUE(data.find(url2) != data.end());

  const auto json1 = data.find(url)->second;
  const auto json2 = data.find(url2)->second;

  EXPECT_EQ(json1["args"].dump(4), expected_response_first.dump(4));
  EXPECT_EQ(json2["args"].dump(4), expected_response_second.dump(4));
}

TEST(Curl, GarbageUrl)
{
  curl::Url garbage("garbage_url");
  const auto data = curl::Get(garbage);
  EXPECT_TRUE(data.is_null());
}

TEST(Curl, Multithread)
{
  curl::Url url(postman_echo_get_base, GetUrlParams(kValidParams));
  curl::Url url2(postman_echo_get_base, GetUrlParams(kValidParams2));

  const auto urls = {url, url2};

  curl::Json expected_response_first;
  curl::Json expected_response_second;
  expected_response_first["foo1"] = "bar1";
  expected_response_first["foo2"] = "bar2";
  expected_response_second["foo3"] = "bar3";
  expected_response_second["foo4"] = "bar4";

  curl::Json responses[6]{};
  const auto thread_func = [&urls, &responses](int i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Try to induce pileup
    auto map = curl::Get(urls);
    const auto* begin = urls.begin();
    const auto& first = *begin;
    ++begin;
    const auto& second = *begin;
    auto data1 = map[first];
    auto data2 = map[second];
    if (!data1.is_null())
    {
      responses[i * 2] = data1["args"];
    }
    if (!data2.is_null())
    {
      responses[i * 2 + 1] = data2["args"];
    }
  };

  const std::size_t num_threads = 3;
  std::vector<std::thread> threads(num_threads);
  for (int i = 0; i < num_threads; ++i)
  {
    threads[i] = std::thread(thread_func, i);
  }

  for (int i = 0; i < num_threads; ++i)
  {
    threads[i].join();
  }

  EXPECT_EQ(responses[0].dump(4), expected_response_first.dump(4));
  EXPECT_EQ(responses[2].dump(4), expected_response_first.dump(4));
  EXPECT_EQ(responses[4].dump(4), expected_response_first.dump(4));
  EXPECT_EQ(responses[1].dump(4), expected_response_second.dump(4));
  EXPECT_EQ(responses[3].dump(4), expected_response_second.dump(4));
  EXPECT_EQ(responses[5].dump(4), expected_response_second.dump(4));
}

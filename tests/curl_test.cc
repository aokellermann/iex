/**
 * @file curl_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include <initializer_list>
#include <thread>

#include "iex/detail/curl_wrapper.h"

namespace curl = iex::curl;

const std::initializer_list<curl::Url::Param> empty_params = {};
const std::initializer_list<curl::Url::Param> invalid_name = {{"foo1", "bar1"}, {"", "bar2"}};
const std::initializer_list<curl::Url::Param> invalid_value = {{"foo1", "bar1"}, {"foo2", ""}};
const std::initializer_list<curl::Url::Param> valid_params = {{"foo1", "bar1"}, {"foo2", "bar2"}};
const std::initializer_list<curl::Url::Param> valid_params_2 = {{"foo3", "bar3"}, {"foo4", "bar4"}};
const std::initializer_list<curl::Url::Param> encode_params = {{"foo1", "bar1"}, {"foo2", "bar+"}};

const char* postman_echo_get_base = "https://postman-echo.com/get";

struct UrlInitParams
{
  const char* base;
  const std::initializer_list<curl::Url::Param>* params;
  bool valid;
  const char* encoded_url;
};

class Url : public testing::TestWithParam<UrlInitParams>
{
 protected:
  Url()
  {
    const auto& param = GetParam();
    url_ = param.params == nullptr
               ? new curl::Url(param.base)
               : new curl::Url(GetParam().base, GetParam().params->begin(), GetParam().params->end());
  }

  ~Url() override { delete url_; }

  curl::Url* url_;
};

TEST_P(Url, CorrectValidity) { EXPECT_EQ(url_->Validity().Success(), GetParam().valid); }

TEST_P(Url, Encoding) { EXPECT_EQ(url_->GetAsString(), GetParam().valid ? GetParam().encoded_url : ""); }

INSTANTIATE_TEST_CASE_P(Correctness,
                        Url,
                        testing::Values(UrlInitParams{"", nullptr, false},
                                        UrlInitParams{"", &empty_params, false},
                                        UrlInitParams{"", &valid_params, false},
                                        UrlInitParams{"base", &invalid_name, false},
                                        UrlInitParams{"base", &invalid_value, false},
                                        UrlInitParams{"base", nullptr, true, "base"},
                                        UrlInitParams{"base", &empty_params, true, "base"},
                                        UrlInitParams{"base", &valid_params, true, "base?foo1=bar1&foo2=bar2"},
                                        UrlInitParams{"base", &encode_params, true, "base?foo1=bar1&foo2=bar%2B"}));

TEST(Curl, Single)
{
  curl::Url url(postman_echo_get_base, valid_params.begin(), valid_params.end());
  curl::Json expected_response;
  expected_response["foo1"] = "bar1";
  expected_response["foo2"] = "bar2";

  const auto data = curl::Get(url);

  ASSERT_TRUE(data.second.Success());
  EXPECT_EQ(data.first["args"].dump(4), expected_response.dump(4));
}

TEST(Curl, Double)
{
  curl::Url url(postman_echo_get_base, valid_params.begin(), valid_params.end());
  curl::Url url2(postman_echo_get_base, valid_params_2.begin(), valid_params_2.end());

  const auto urls = {url, url2};

  curl::Json expected_response_first, expected_response_second;
  expected_response_first["foo1"] = "bar1";
  expected_response_first["foo2"] = "bar2";
  expected_response_second["foo3"] = "bar3";
  expected_response_second["foo4"] = "bar4";

  const auto data = curl::Get(urls.begin(), urls.end());

  ASSERT_TRUE(data.second.Success());
  ASSERT_TRUE(data.first.find(url) != data.first.end());
  ASSERT_TRUE(data.first.find(url2) != data.first.end());

  const auto json1 = data.first.find(url)->second.first, json2 = data.first.find(url2)->second.first;

  EXPECT_EQ(json1["args"].dump(4), expected_response_first.dump(4));
  EXPECT_EQ(json2["args"].dump(4), expected_response_second.dump(4));
}

TEST(Curl, GarbageUrl)
{
  curl::Url garbage("garbage_url");
  const auto data = curl::Get(garbage);
  EXPECT_TRUE(data.second.Failure());
}

TEST(Curl, Multithread)
{
  curl::Url url(postman_echo_get_base, valid_params.begin(), valid_params.end());
  curl::Url url2(postman_echo_get_base, valid_params_2.begin(), valid_params_2.end());

  const auto urls = {url, url2};

  curl::Json expected_response_first, expected_response_second;
  expected_response_first["foo1"] = "bar1";
  expected_response_first["foo2"] = "bar2";
  expected_response_second["foo3"] = "bar3";
  expected_response_second["foo4"] = "bar4";

  curl::Json responses[6];
  const auto thread_func = [&urls, &responses](int i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Try to induce pileup
    auto map = curl::Get(urls);
    auto begin = urls.begin();
    auto& first = *begin;
    ++begin;
    auto& second = *begin;
    auto data1 = map.first[first], data2 = map.first[second];
    if (data1.second.Success())
    {
      responses[i * 2] = data1.first["args"];
    }
    if (data2.second.Success())
    {
      responses[i * 2 + 1] = data2.first["args"];
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

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

#include <initializer_list>

#include "iex/curl_wrapper.h"
#include "iex/iex.h"

namespace curl = iex::curl;

const std::initializer_list<std::pair<const char*, const char*>> empty_params = {};
const std::initializer_list<std::pair<const char*, const char*>> invalid_name = {{"foo1", "bar1"}, {"", "bar2"}};
const std::initializer_list<std::pair<const char*, const char*>> invalid_value = {{"foo1", "bar1"}, {"foo2", ""}};
const std::initializer_list<std::pair<const char*, const char*>> valid_params = {{"foo1", "bar1"}, {"foo2", "bar2"}};
const std::initializer_list<std::pair<const char*, const char*>> valid_params_2 = {{"foo3", "bar3"}, {"foo4", "bar4"}};
const std::initializer_list<std::pair<const char*, const char*>> encode_params = {{"foo1", "bar1"}, {"foo2", "bar+"}};

const char* postman_echo_get_base = "https://postman-echo.com/get";

TEST(Url, Validity)
{
  EXPECT_TRUE(curl::Url("").Validity().Failure());
  EXPECT_TRUE(curl::Url("", empty_params.begin(), empty_params.end()).Validity().Failure());
  EXPECT_TRUE(curl::Url("", valid_params.begin(), valid_params.end()).Validity().Failure());
  EXPECT_TRUE(curl::Url("base", invalid_name.begin(), invalid_name.end()).Validity().Failure());
  EXPECT_TRUE(curl::Url("base", invalid_value.begin(), invalid_value.end()).Validity().Failure());

  EXPECT_TRUE(curl::Url("base").Validity().Success());
  EXPECT_TRUE(curl::Url("base", empty_params.begin(), empty_params.end()).Validity().Success());
  EXPECT_TRUE(curl::Url("base", valid_params.begin(), valid_params.end()).Validity().Success());

  EXPECT_TRUE(curl::Url("").GetAsString().empty());
  EXPECT_TRUE(curl::Url("base", invalid_value.begin(), invalid_value.end()).GetAsString().empty());
}

TEST(Url, EqualityWithoutEncoding)
{
  EXPECT_EQ(curl::Url("base").GetAsString(), "base");
  EXPECT_EQ(curl::Url("base", empty_params.begin(), empty_params.end()).GetAsString(), "base");
  EXPECT_EQ(curl::Url("base", valid_params.begin(), valid_params.end()).GetAsString(), "base?foo1=bar1&foo2=bar2");
  EXPECT_EQ(curl::Url("base", encode_params.begin(), encode_params.end()).GetAsString(), "base?foo1=bar1&foo2=bar%2B");
}

TEST(Curl, Single)
{
  curl::Url url(postman_echo_get_base, valid_params.begin(), valid_params.end());
  curl::Json expected_response;
  expected_response["foo1"] = "bar1";
  expected_response["foo2"] = "bar2";

  const auto data = curl::Get(url);

  ASSERT_TRUE(data.second.Success());
  EXPECT_EQ(data.first["args"], expected_response);
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

  EXPECT_EQ(json1["args"], expected_response_first);
  EXPECT_EQ(json2["args"], expected_response_second);
}

TEST(Curl, GarbageUrl)
{
  curl::Url garbage("garbage_url");
  const auto data = curl::Get(garbage);
  EXPECT_TRUE(data.second.Failure());
}

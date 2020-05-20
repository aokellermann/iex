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

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

#include "iex/iex.h"

namespace iex::curl
{
// region Url

class Url
{
 public:
  Url() = delete;

  explicit Url(const char* base_url) : impl_(base_url), ec_(impl_.empty() ? "Empty URL" : "") {}

  template <class InputIt>
  Url(const char* base_url, InputIt params_begin, InputIt params_end) : Url(base_url)
  {
    if (ec_.Failure())
    {
      impl_.clear();
      return;
    }

    for (InputIt head = params_begin; head != params_end; ++head)
    {
      AppendParam(head->first, head->second, head == params_begin);
      if (ec_.Failure())
      {
        impl_.clear();
        return;
      }
    }
  }

  [[nodiscard]] const std::string& GetAsString() const noexcept { return impl_; }

  [[nodiscard]] const ErrorCode& Validity() const noexcept { return ec_; }

  bool operator==(const Url& other) const { return impl_ == other.impl_; }

 private:
  static ValueWithErrorCode<std::string> UrlEncode(const std::string& plaintext_str);

  void AppendParam(const std::string& name, const std::string& raw_value, bool first);

  std::string impl_;
  ErrorCode ec_;
};

struct UrlHasher
{
  std::size_t operator()(const Url& s) const { return std::hash<std::string>()(s.GetAsString()); }
};

struct UrlEquality
{
  size_t operator()(const Url& a, const Url& b) const noexcept { return a.GetAsString() == b.GetAsString(); }
};

// endregion

// region Interface

using Json = nlohmann::json;

template <class InputIt>
ValueWithErrorCode<std::unordered_map<Url, ValueWithErrorCode<Json>, UrlHasher, UrlEquality>> Get(
    InputIt urls_begin, InputIt urls_end, int max_connections = 0);

ValueWithErrorCode<Json> Get(const Url& url, int max_connections = 0)
{
  const auto url_list = {url};
  auto data_map = Get(url_list.begin(), url_list.end(), max_connections);
  return {data_map.first[url].first, data_map.second};
}

// endregion Interface
}  // namespace iex::curl

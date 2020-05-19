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

#include <string>

#include "iex/iex.h"

namespace iex::curl
{
// region Url

class Url
{
 public:
  explicit Url(const char* base_url) : impl_(base_url) {}

  template <class InputIt>
  Url(const char* base_url, InputIt params_begin, InputIt params_end) : Url(base_url)
  {
    if (params_begin != params_end)
    {
      impl_.append("?" + params_begin->first + '=' + params_begin->second);
      while (++params_begin != params_end)
      {
        impl_.append("&" + params_begin->first + '=' + params_begin->second);
      }
    }
  }

  [[nodiscard]] const std::string& GetAsString() const noexcept { return impl_; }

  bool operator==(const Url& other) const { return impl_ == other.impl_; }

 private:
  std::string impl_;
};

struct UrlHasher
{
  std::size_t operator()(const Url& s) const { return std::hash<std::string>()(s.GetAsString()); }
};

// endregion
}  // namespace iex::curl

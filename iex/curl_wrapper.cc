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

#include "iex/curl_wrapper.h"

#include <curl/curl.h>

#include <memory>
#include <mutex>

namespace iex::curl
{
namespace
{
namespace detail
{
// region Handle Wrappers

struct EasyHandle
{
  EasyHandle() : handle_(curl_easy_init()) {}

  ~EasyHandle() { curl_easy_cleanup(handle_); }

  CURL* handle_;
};

struct MultiHandle
{
  MultiHandle() : handle_(curl_multi_init()) {}

  ~MultiHandle() { curl_multi_cleanup(handle_); }

  CURLM* handle_;
};

// endregion Handle Wrappers

// region Initialization

/**
 * This class should only be instantiated once as static.
 */
class CurlInitImpl
{
 public:
  CurlInitImpl()
  {
    const CURLcode init_res = curl_global_init(CURL_GLOBAL_ALL);
    if (init_res != CURLcode::CURLE_OK)
    {
      curl_init_ec_ = ErrorCode("curl_global_init(CURL_GLOBAL_ALL) failed", ErrorCode(curl_easy_strerror(init_res)));
    }
  }

  ~CurlInitImpl() { curl_global_cleanup(); }

  [[nodiscard]] const ErrorCode& GetErrorCode() const noexcept { return curl_init_ec_; }

 private:
  /// Stores the `ErrorCode` returned by `curl_global_init`.
  ErrorCode curl_init_ec_;
};

std::unique_ptr<CurlInitImpl> curl_init_impl;

// endregion Initialization

// region Curl Escape

std::unique_ptr<EasyHandle> curl_escape_handle;

// endregionCurl Escape

// region Interface

std::once_flag init_once_flag;

const ErrorCode& InitIfNeeded()
{
  std::call_once(init_once_flag, [] {
    curl_init_impl = std::make_unique<CurlInitImpl>();
    curl_escape_handle = std::make_unique<EasyHandle>();
  });
  return curl_init_impl->GetErrorCode();
}

// endregion Interface
}  // namespace detail

// region Interface

const ErrorCode& InitIfNeeded() { return detail::InitIfNeeded(); }

ValueWithErrorCode<std::string> GetEscapedUrlStringFromPlaintextString(const std::string& plaintext_url_string)
{
  // Curl must be initialized before using escape function!
  const ErrorCode& init_ec = InitIfNeeded();
  if (init_ec.Failure())
  {
    return {std::string(), init_ec};
  }

  char* escaped_c_string =
      curl_easy_escape(detail::curl_escape_handle->handle_, plaintext_url_string.c_str(), plaintext_url_string.size());
  std::string escaped_string(escaped_c_string);
  curl_free(escaped_c_string);
  if (escaped_string.empty())
  {
    return {std::string(), ErrorCode("curl_easy_escape() failed")};
  }

  return {escaped_string, ErrorCode()};
}

// endregion Interface
}  // namespace

// region Url

ValueWithErrorCode<std::string> Url::UrlEncode(const std::string& plaintext_str)
{
  return GetEscapedUrlStringFromPlaintextString(plaintext_str);
}

void Url::AppendParam(const std::string& name, const std::string& raw_value, bool first)
{
  if (name.empty() || raw_value.empty())
  {
    ec_ = ErrorCode(name.empty() ? "name is empty" : "value is empty");
    return;
  }

  const auto pair = UrlEncode(raw_value);
  if (pair.second.Failure())
  {
    ec_ = pair.second;
    return;
  }

  impl_ += (first ? "?" : "&") + name + "=" + pair.first;
}

// endregion Url
}  // namespace iex::curl

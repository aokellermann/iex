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
std::once_flag init_impl;

// endregion Initialization
}  // namespace detail

const ErrorCode& InitIfNeeded()
{
  std::call_once(detail::init_impl, [] { detail::curl_init_impl = std::make_unique<detail::CurlInitImpl>(); });
  return detail::curl_init_impl->GetErrorCode();
}

}  // namespace

}  // namespace iex::curl

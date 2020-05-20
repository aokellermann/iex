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

#include <initializer_list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iex::curl
{
namespace
{
namespace detail
{
// region Helpers

/**
 * Appends string to ptr and returns size * nmemb
 * @param ptr char* to append to
 * @param size size of string
 * @param nmemb size per element of string
 * @param string char* to append to ptr
 * @return number of bytes appended
 */
inline size_t WriteFunc(void* ptr, size_t size, size_t nmemb, void* string)
{
  (static_cast<std::string*>(string))->append(reinterpret_cast<char*>(ptr), size * nmemb);
  return size * nmemb;
}

/**
 * RAII wrapper for CURL easy handle.
 */
struct EasyHandle
{
  EasyHandle() : handle_(curl_easy_init()) {}

  ~EasyHandle() { curl_easy_cleanup(handle_); }

  CURL* handle_;
};

struct EasyHandleDataPair : std::pair<EasyHandle, std::string>
{
  EasyHandleDataPair() = default;

  explicit EasyHandleDataPair(const Url& url)
  {
    AssignUrl(url);
    curl_easy_setopt(first.handle_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(first.handle_, CURLOPT_WRITEFUNCTION, &WriteFunc);
    curl_easy_setopt(first.handle_, CURLOPT_WRITEDATA, &second);
    curl_easy_setopt(first.handle_, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(first.handle_, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(first.handle_, CURLOPT_SSL_VERIFYHOST, 0L);
  }

  void AssignUrl(const Url& url)
  {
    curl_easy_setopt(first.handle_, CURLOPT_URL, url.GetAsString().c_str());
    curl_easy_setopt(first.handle_, CURLOPT_PRIVATE, &url);
  }

  Json ExtractData()
  {
    if (second.empty())
    {
      return Json();
    }

    Json json = Json::parse(second);
    second.clear();
    return json;
  }
};

/**
 * RAII wrapper for CURL multi handle.
 */
struct MultiHandle
{
  MultiHandle() : handle_(curl_multi_init()) {}

  ~MultiHandle() { curl_multi_cleanup(handle_); }

  CURLM* handle_;
};

class MultiHandleImpl
{
 public:
  // Adapted from https://curl.haxx.se/libcurl/c/10-at-a-time.html
  std::unordered_map<Url, ValueWithErrorCode<Json>, UrlHasher, UrlEquality> Get(
      const std::unordered_set<Url, UrlHasher, UrlEquality>& url_set, int max_connections)
  {
    // Populate handles_in_use_.
    GetAvailableHandles(url_set);

    // Set max parallel connections (useful for clients wanting high throughput).
    curl_multi_setopt(multi_handle_.handle_, CURLMOPT_MAX_TOTAL_CONNECTIONS, max_connections);

    CURLMsg* msg;
    int msgs_left = -1;
    int still_alive = 1;

    // Contains information about failed GETs.
    std::unordered_map<Url, ErrorCode, UrlHasher, UrlEquality> ecs;

    while (still_alive)
    {
      curl_multi_perform(multi_handle_.handle_, &still_alive);

      while ((msg = curl_multi_info_read(multi_handle_.handle_, &msgs_left)))
      {
        if (msg->msg == CURLMSG_DONE)
        {
          Url* url;
          CURL* done_handle = msg->easy_handle;
          curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &url);
          const CURLcode result = msg->data.result;
          if (result != CURLE_OK)
          {
            ecs.emplace(*url, ErrorCode(curl_easy_strerror(result)));
          }
          curl_multi_remove_handle(multi_handle_.handle_, done_handle);
        }
      }

      if (still_alive)
      {
        curl_multi_wait(multi_handle_.handle_, nullptr, 0, 1000, nullptr);
      }
    }

    std::unordered_map<Url, ValueWithErrorCode<Json>, UrlHasher, UrlEquality> return_data;
    for (auto& handle : handles_in_use_)
    {
      auto ec_iter = ecs.find(handle.first);
      ErrorCode ec = ec_iter == ecs.end() ? ErrorCode() : ec_iter->second;
      return_data.insert(std::make_pair(handle.first, std::make_pair(handle.second.ExtractData(), std::move(ec))));
    }

    ClearUsedHandles();

    return return_data;
  }

 private:
  void GetAvailableHandles(const std::unordered_set<Url, UrlHasher, UrlEquality>& url_set)
  {
    // Try to find handles with the same Url first.
    for (const auto& url : url_set)
    {
      if (available_handles.empty())
      {
        break;
      }

      auto found_url = available_handles.extract(url);
      if (!found_url.empty())
      {
        handles_in_use_.insert(std::move(found_url));
      }
    }

    // Reuse handles that have already been created.
    std::size_t num_handles_still_needed = handles_in_use_.size() - url_set.size();
    std::size_t i = 0;
    auto current_url_iter = url_set.begin();
    for (; i < num_handles_still_needed && current_url_iter != url_set.end(); ++i, ++current_url_iter)
    {
      if (!available_handles.empty())
      {
        // Reuse handles here
        // Extract, and change Url.
        auto extracted_node = available_handles.extract(available_handles.begin());
        extracted_node.key() = *current_url_iter;
        extracted_node.mapped().AssignUrl(*current_url_iter);
        handles_in_use_.insert(std::move(extracted_node));
      }
      else
      {
        // Create new handles if no more are available.
        handles_in_use_.emplace(*current_url_iter, *current_url_iter);
      }
    }

    for (const auto& [url, pair] : handles_in_use_)
    {
      curl_multi_add_handle(multi_handle_.handle_, pair.first.handle_);
    }
  }

  void ClearUsedHandles()
  {
    // Move all to available.
    available_handles.merge(handles_in_use_);
  }

  MultiHandle multi_handle_;
  std::unordered_map<Url, EasyHandleDataPair, UrlHasher, UrlEquality> handles_in_use_;
  std::unordered_map<Url, EasyHandleDataPair, UrlHasher, UrlEquality> available_handles;
};

thread_local std::unique_ptr<MultiHandleImpl> local_multi_handle;

// endregion Helpers

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

std::unordered_map<Url, ValueWithErrorCode<Json>, UrlHasher, UrlEquality> Get(
    const std::unordered_set<Url, UrlHasher, UrlEquality>& url_set, int max_connections)
{
  if (!detail::local_multi_handle)
  {
    detail::local_multi_handle = std::make_unique<detail::MultiHandleImpl>();
  }

  return detail::local_multi_handle->Get(url_set, max_connections);
}

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

// region Interface

template <class InputIt>
ValueWithErrorCode<std::unordered_map<Url, ValueWithErrorCode<Json>, UrlHasher, UrlEquality>> Get(InputIt urls_begin,
                                                                                                  InputIt urls_end,
                                                                                                  int max_connections)
{
  const auto& init_ec = InitIfNeeded();
  if (init_ec.Failure())
  {
    return {std::unordered_map<Url, ValueWithErrorCode<Json>, UrlHasher, UrlEquality>(),
            ErrorCode("curl::Get failed", init_ec)};
  }

  const std::unordered_set<Url, UrlHasher, UrlEquality> url_set(urls_begin, urls_end);
  for (const auto& url : url_set)
  {
    if (url.Validity().Failure())
    {
      return {std::unordered_map<Url, ValueWithErrorCode<Json>, UrlHasher, UrlEquality>(),
              ErrorCode(std::string("curl::Get failed"),
                        ErrorCode(std::string("at least one Url is invalid"), url.Validity()))};
    }
  }

  auto data_map = Get(url_set, max_connections);
  std::vector<ErrorCode> named_errors;
  for (const auto& [url, pair] : data_map)
  {
    if (pair.second.Failure())
    {
      named_errors.emplace_back(
          ErrorCode("HTTP GET failed", {{"url", ErrorCode(url.GetAsString())}, {"ec", pair.second}}));
    }
  }

  ErrorCode ec =
      named_errors.empty() ? ErrorCode() : ErrorCode("curl::Get failed", named_errors.begin(), named_errors.end());
  return {data_map, ec};
}

// endregion Interface
}  // namespace iex::curl

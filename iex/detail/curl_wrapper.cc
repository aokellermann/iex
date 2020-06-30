/**
 * @file curl_wrapper.cc
 * @author Antony Kellermann
 * @brief Contains all definitions necessary for performing a HTTP GET by a client.
 * @copyright 2020 Antony Kellermann
 */

#include "curl_wrapper.h"

#include <curl/curl.h>

#include <initializer_list>
#include <memory>
#include <mutex>
#include <thread>
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

/**
 * RAII wrapper for CURL easy handle and its corresponding data.
 *
 * This wrapper is reusable. To reuse with the same Url, nothing special needs to be done. If you would like to reuse
 * with a new Url, call AssignUrl, passing the Url you would like to use.
 *
 * After use, ExtractData should be called (so that the data buffer is cleared).
 */
class EasyHandleDataPair : public std::pair<EasyHandle, std::string>
{
 public:
  EasyHandleDataPair() = delete;

  explicit EasyHandleDataPair(Url target_url) : url_(std::move(target_url))
  {
    AssignHandleUrlOptions();

    // Follow any redirections
    curl_easy_setopt(first.handle_, CURLOPT_FOLLOWLOCATION, 1L);
    // Set output data location
    curl_easy_setopt(first.handle_, CURLOPT_WRITEFUNCTION, &WriteFunc);
    // Set write callback for appending data
    curl_easy_setopt(first.handle_, CURLOPT_WRITEDATA, &second);
    // Accept all types of encoding
    curl_easy_setopt(first.handle_, CURLOPT_ACCEPT_ENCODING, "");
    // Turns off internal signalling to ensure thread-safety. See here: https://curl.haxx.se/libcurl/c/threadsafe.html
    curl_easy_setopt(first.handle_, CURLOPT_NOSIGNAL, 1L);
    // Treats HTTP codes greater than 400 as error. See https://iexcloud.io/docs/api/#error-codes
    curl_easy_setopt(first.handle_, CURLOPT_FAILONERROR, 1L);
  }

  void AssignUrl(Url target_url)
  {
    url_ = std::move(target_url);
    AssignHandleUrlOptions();
  }

  ValueWithErrorCode<Json> ExtractData()
  {
    if (second.empty())
    {
      return {{}, ErrorCode{"Empty return data"}};
    }

    Json json;
    try
    {
      json = Json::parse(second);
    }
    catch (const std::exception& e)
    {
      return {{}, ErrorCode("Failed to parse return data", {{"ex", ErrorCode(e.what())}, {"data", ErrorCode(second)}})};
    }

    second.clear();
    return {std::move(json), {}};
  }

 private:
  void AssignHandleUrlOptions()
  {
    // Set Url
    curl_easy_setopt(first.handle_, CURLOPT_URL, url_.GetAsString().c_str());
    // Store pointer to associated Url.
    curl_easy_setopt(first.handle_, CURLOPT_PRIVATE, &url_);
  }

  Url url_;
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

class MultiHandleWrapper
{
 public:
  /**
   * Performs HTTP GETs on the input Urls, allowing at most max_connection parallel connections.
   *
   * This function is adapted from https://curl.haxx.se/libcurl/c/10-at-a-time.html
   * @param url_set the Urls to perform HTTP GET on
   * @param max_connections maximum number of parallel connections
   * @return map of Url to returned Json data (with ErrorCode if encountered)
   */
  GetMap Get(const UrlSet& url_set, int max_connections, const RetryBehavior& retry_behavior)
  {
    // Populate handles_in_use_.
    GetAvailableHandles(url_set);

    // Set max parallel connections (useful for clients wanting high throughput).
    curl_multi_setopt(multi_handle_.handle_, CURLMOPT_MAX_TOTAL_CONNECTIONS, max_connections);

    CURLMsg* msg;
    int msgs_left = -1;
    int still_alive = 1;

    // Contains information about failed GETs.
    UrlMap<ErrorCode> ecs;

    // Contains number of retries per url.
    UrlMap<int> retries;

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

          bool do_retry = false;
          const bool retry_allowed = retry_behavior.max_retries > retries[*url];

          if (result != CURLE_OK)
          {
            HttpResponseCode http_code = 0;
            if (result == CURLE_HTTP_RETURNED_ERROR)
            {
              curl_easy_getinfo(done_handle, CURLINFO_RESPONSE_CODE, &http_code);
            }

            // Check if request should be retried. Otherwise, emplace ec.
            if (retry_allowed && retry_behavior.responses_to_retry.count(http_code))
            {
              do_retry = true;
            }
            else
            {
              ErrorCode ec = http_code == 0 ? ErrorCode(curl_easy_strerror(result))
                                            : ErrorCode(curl_easy_strerror(result),
                                                        {{"response code", ErrorCode(std::to_string(http_code))},
                                                         {"retries", ErrorCode(std::to_string(retries[*url]))}});
              ecs.emplace(*url, std::move(ec));
            }
          }
          else if (handles_in_use_.find(*url)->second.second.empty())
          {
            // If successful curl, empty response data, and retry behavior is specified, retry.
            if (retry_allowed && retry_behavior.retry_if_empty_response_data)
            {
              do_retry = true;
            }
            else
            {
              ErrorCode ec = ErrorCode("Empty return data", {"retries", ErrorCode(std::to_string(retries[*url]))});
              ecs.emplace(*url, std::move(ec));
            }
          }

          // Always remove handle after complete request
          curl_multi_remove_handle(multi_handle_.handle_, done_handle);

          if (retry_allowed && do_retry)
          {
            // Clear data and increment retries
            handles_in_use_.find(*url)->second.second.clear();
            ++retries[*url];

            // Sleep before retrying
            std::this_thread::sleep_for(retry_behavior.timeout);

            // Re-add handle
            curl_multi_add_handle(multi_handle_.handle_, done_handle);
          }
        }
      }

      if (still_alive)
      {
        curl_multi_wait(multi_handle_.handle_, nullptr, 0, 1000, nullptr);
      }
    }

    GetMap return_data;
    for (auto& handle : handles_in_use_)
    {
      auto ec_iter = ecs.find(handle.first);
      ErrorCode ec = ec_iter == ecs.end() ? ErrorCode() : ec_iter->second;
      auto data = handle.second.ExtractData();
      if (ec.Success() && data.second.Failure())
      {
        ec = std::move(data.second);
      }

      return_data.insert(std::make_pair(handle.first, std::make_pair(std::move(data.first), std::move(ec))));
    }

    ClearUsedHandles();

    return return_data;
  }

 private:
  /**
   * Populates handles_in_use_ with valid Url-assigned handles.
   * @param url_set the Urls for which handles are requirded
   */
  void GetAvailableHandles(const std::unordered_set<Url, UrlHasher, UrlEquality>& url_set)
  {
    // Try to find handles with the same Url first.
    for (const auto& url : url_set)
    {
      if (available_handles_.empty())
      {
        break;
      }

      auto found_url = available_handles_.extract(url);
      if (!found_url.empty())
      {
        handles_in_use_.insert(std::move(found_url));
      }
    }

    // Reuse handles that have already been created.
    for (const auto& url : url_set)
    {
      if (!handles_in_use_.count(url))
      {
        if (!available_handles_.empty())
        {
          // Reuse handles here
          // Extract, and change Url.
          auto extracted_node = available_handles_.extract(available_handles_.begin());
          extracted_node.key() = url;
          extracted_node.mapped().AssignUrl(url);
          handles_in_use_.insert(std::move(extracted_node));
        }
        else
        {
          // Create new handles if no more are available.
          handles_in_use_.emplace(url, url);
        }
      }
    }

    for (const auto& [url, pair] : handles_in_use_)
    {
      curl_multi_add_handle(multi_handle_.handle_, pair.first.handle_);
    }
  }

  /**
   * Moves all handles from handles_in_use_ to available_handles.
   */
  void ClearUsedHandles()
  {
    // Move all to available.
    available_handles_.merge(handles_in_use_);
  }

  MultiHandle multi_handle_;
  UrlMap<EasyHandleDataPair> handles_in_use_;
  UrlMap<EasyHandleDataPair> available_handles_;
};

/**
 * Thread local multi handle, used for performing multiple GETs in parallel. This variable is lazily initialized.
 *
 * By using a thread local multi handle, our CURL wrapper can be completely lock free!
 */
thread_local std::unique_ptr<MultiHandleWrapper> local_multi_handle;

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

/**
 * This handle is maintained
 */
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

GetMap PerformGet(const std::unordered_set<Url, UrlHasher, UrlEquality>& url_set,
                  int max_connections,
                  const RetryBehavior& retry_behavior)
{
  if (!detail::local_multi_handle)
  {
    detail::local_multi_handle = std::make_unique<detail::MultiHandleWrapper>();
  }

  return detail::local_multi_handle->Get(url_set, max_connections, retry_behavior);
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

  // CURL documentation states that a null or empty return value from curl_easy_escape indicates failure.
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

void Url::AppendParam(const Param& param, bool first)
{
  if (param.name.empty() || param.value.empty())
  {
    ec_ = ErrorCode(param.name.empty() ? "name is empty" : "value is empty");
    return;
  }

  const auto pair = UrlEncode(param.value);
  if (pair.second.Failure())
  {
    ec_ = pair.second;
    return;
  }

  impl_ += (first ? "?" : "&") + param.name + "=" + pair.first;
}

// endregion Url

// region Interface

ErrorCode Init() { return InitIfNeeded(); }

ValueWithErrorCode<GetMap> Get(const std::unordered_set<Url, UrlHasher, UrlEquality>& url_set,
                               int max_connections,
                               const RetryBehavior& retry_behavior)
{
  // Check if CURL initialization previously succeeded (it is typically initialized on Url construction).
  const auto& init_ec = InitIfNeeded();
  if (init_ec.Failure())
  {
    return {GetMap(), ErrorCode("curl::Get failed", init_ec)};
  }

  // Check for invalid Urls.
  for (const auto& url : url_set)
  {
    if (url.Validity().Failure())
    {
      return {GetMap(), ErrorCode(std::string("curl::Get failed"),
                                  ErrorCode(std::string("at least one Url is invalid"), url.Validity()))};
    }
  }

  // Perform GET.
  auto data_map = PerformGet(url_set, max_connections, retry_behavior);

  // Generate ErrorCode if any GETs failed.
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

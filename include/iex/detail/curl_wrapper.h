/**
 * @file curl_wrapper.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <chrono>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "iex/detail/common.h"
#include "iex/detail/json_serializer.h"
#include "iex/detail/utils.h"

/**
 * Contains all declarations necessary for performing a HTTP GET by a client.
 */
namespace iex::curl
{
// region Url

/// Thrown when an invalid Url or Param is being constructed.
struct InvalidUrlError : public ErrorCode
{
  enum ErrorID
  {
    EMPTY_URL,
    EMPTY_PARAM_KEY,
    EMPTY_PARAM_VALUE,
    EMPTY_PARAM_LIST_VALUE,
  };

  explicit InvalidUrlError(ErrorID error_id);

  template <typename T>
  static auto Validator(ErrorID error_id)
  {
    static const auto kFunc = [error_id](const T& t) { return t.empty() ? throw InvalidUrlError(error_id) : t; };
    return kFunc;
  }

  ErrorID id;
};

/**
 * Represents a Url that can be the subject of an HTTP GET.
 */
class Url
{
 public:
  using ValueType = std::string;

  /**
   * Hasher struct to use for Urls.
   */
  struct UrlHasher
  {
    std::size_t operator()(const Url& url) const noexcept { return std::hash<ValueType>()(ValueType(url)); }
  };

  /**
   * Represents a named Url parameter, such as ?foo=bar or &foo=bar1,bar2.
   */
  class Param : public KVP<std::string>
  {
   public:
    using KeyType = KVP::KeyType;
    using ValueType = KVP::ValueType;

    /// An empty param is always invalid.
    Param() = delete;

    explicit Param(const KVP<ValueType>& kvp) : Param(kvp.key, kvp.value) {}

    /// Constructs a Param with the given key and value. Throws if empty key or value.
    Param(const KeyType& k, const ValueType& v)
        : KVP(InvalidUrlError::Validator<KeyType>(InvalidUrlError::EMPTY_PARAM_KEY)(k),
              Escape(InvalidUrlError::Validator<ValueType>(InvalidUrlError::EMPTY_PARAM_VALUE)(v)))
    {
    }

    /// Constructs a param with the given key and values, which are comma delimited and escaped.
    template <typename InputIt>
    Param(const KeyType& k, InputIt v_begin, InputIt v_end)
        : KVP(InvalidUrlError::Validator<KeyType>(InvalidUrlError::EMPTY_PARAM_KEY)(k),
              utils::Join(v_begin, v_end, ',', [](const auto& v) {
                return Escape(InvalidUrlError::Validator<ValueType>(InvalidUrlError::EMPTY_PARAM_LIST_VALUE)(v));
              }))
    {
    }

    /// Constructs a param with the given key and values, which are comma delimited and escaped.
    Param(const KeyType& k, std::initializer_list<ValueType> vs) : Param(k, vs.begin(), vs.end()) {}

    explicit operator std::string() const { return key + '=' + value; }

   private:
    static Param::ValueType Escape(const Param::ValueType& v);
  };

  /// A unique (by key) set of params.
  using Params = std::set<Param, Param::KeyCompare>;

  /// An empty Url is always invalid.
  Url() = delete;

  /**
   * Constructs a Url from the given string.
   * @param url the Url as a string
   * @throws InvalidUrlError if empty
   */
  explicit Url(const ValueType& base) : impl_(InvalidUrlError::Validator<ValueType>(InvalidUrlError::EMPTY_URL)(base))
  {
  }

  template <class InputIt>
  Url(const ValueType& base, InputIt params_begin, InputIt params_end) : Url(base)
  {
    if (params_begin != params_end)
      impl_ += '?' + utils::Join(params_begin, params_end, '&', [](const auto& p) { return std::string(p); });
  }

  Url(const std::string& base, const Params& params) : Url(base, params.begin(), params.end()) {}

  explicit operator const ValueType&() const { return impl_; }

  bool operator==(const Url& other) const noexcept { return impl_ == other.impl_; }

 private:
  ValueType impl_;
};

/**
 * Unordered map of Url objects using custom hasher UrlHasher.
 */
template <typename T>
using UrlMap = std::unordered_map<Url, T, Url::UrlHasher>;

/**
 * Unordered set of Url objects using custom hasher UrlHasher.
 */
using UrlSet = std::unordered_set<Url, Url::UrlHasher>;

// endregion Url

// region Retry

/**
 * The HTTP response number, such as 404 (Not Found).
 */
using HttpResponseCode = int64_t;

/**
 * Time to wait in ms.
 */
using TimeoutDuration = std::chrono::milliseconds;

/**
 * This struct determines if HTTP requests are retried on error.
 */
struct RetryBehavior
{
  /**
   * The Url will be requested at maximum this number of times + 1. Ignored if responses_to_try is empty.
   */
  int max_retries = 0;

  /**
   * The Url will be retried if this set contains the response code. Ignored if max_retries is zero.
   */
  std::unordered_set<HttpResponseCode> responses_to_retry;

  /**
   * Retry if the HTTP request succeeded, but contains no data.
   */
  bool retry_if_empty_response_data = false;

  /**
   * Sleep period before retrying request.
   */
  TimeoutDuration timeout = decltype(timeout)::zero();
};

// endregion Retry

// region Interface

/**
 * @brief Initializes all CURL components.
 * @details This function must be called once at the beginning of the program before ANY other threads have been
 * created. This behavior is a dependency of cURL, not this library.
 * @see https://curl.haxx.se/libcurl/c/curl_global_init.html
 * @return ErrorCode denoting success or failure.
 */
ErrorCode Init();

using Json = json::Json;

/**
 * An unordered map of Url to their corresponding GetResponse objects. This is what is returned when querying multiple
 * Urls at the same time.
 */
using UrlJsonMap = UrlMap<Json>;

/**
 * See templated Get function.
 */
UrlJsonMap Get(const UrlSet& url_set, int max_connections = 0, const RetryBehavior& retry_behavior = {});

/**
 * Performs HTTP GET on target Urls, with max_connection maximum parallel HTTP connections.
 * @tparam InputIt Iterator such that it can be dereferenced to a Url.
 * @param urls_begin beginning of Url container
 * @param urls_end end of Url container
 * @param max_connections number of maximum HTTP parellel connections allowed (0 means no limit)
 * @param retry_behavior The number and response codes to retry HTTP requests if encountered.
 * @return map of Url to corresponding returned data (with error code)
 */
template <class InputIt>
inline UrlJsonMap Get(InputIt urls_begin, InputIt urls_end, int max_connections = 0,
                      const RetryBehavior& retry_behavior = {})
{
  const UrlSet url_set(urls_begin, urls_end);
  return Get(url_set, max_connections, retry_behavior);
}

/**
 * See templated Get function.
 */
inline Json Get(const Url& url, int max_connections = 0, const RetryBehavior& retry_behavior = {})
{
  const auto url_list = {url};
  auto data_map = Get(url_list.begin(), url_list.end(), max_connections, retry_behavior);
  const auto iter = data_map.find(url);
  return iter != data_map.end() ? iter->second : Json();
}

// endregion Interface
}  // namespace iex::curl

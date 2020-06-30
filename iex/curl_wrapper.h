/**
 * @file curl_wrapper.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <chrono>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "iex/common.h"
#include "iex/json_serializer.h"

/**
 * Contains all declarations necessary for performing a HTTP GET by a client.
 */
namespace iex::curl
{
// region Url

/**
 * Represents a Url that can be the subject of an HTTP GET.
 */
class Url
{
 public:
  /**
   * Represents a named Url parameter.
   */
  struct Param
  {
    using Name = std::string;
    using Value = std::string;

    struct Hasher
    {
      std::size_t operator()(const Param& param) const { return std::hash<std::string>()(param.name); }
    };

    template <typename InputIt>
    Param(Name param_name, InputIt comma_separated_params_begin, InputIt comma_separated_params_end)
        : name(std::move(param_name))
    {
      for (auto iterator = comma_separated_params_begin; iterator != comma_separated_params_end; ++iterator)
      {
        value += *iterator + ',';
      }
      if (comma_separated_params_begin != comma_separated_params_end)
      {
        value.pop_back();
      }
    }

    Param(Name param_name, std::initializer_list<Value> values)
        : Param(std::move(param_name), values.begin(), values.end())
    {
    }

    Param(Name param_name, Value param_value) : name(std::move(param_name)), value(std::move(param_value)) {}

    bool operator==(const Param& other) const { return name == other.name && value == other.value; }

    Name name;
    Value value;
  };

  using Params = std::unordered_set<Param, Param::Hasher>;

  Url() = delete;

  explicit Url(std::string base_url) : impl_(std::move(base_url)), ec_(impl_.empty() ? "Empty URL" : "") {}

  Url(const std::string& base_url, Params params) : Url(base_url, params.begin(), params.end()) {}

  template <class InputIt>
  Url(const std::string& base_url, InputIt params_begin, InputIt params_end) : Url(base_url)
  {
    if (ec_.Failure())
    {
      impl_.clear();
      return;
    }

    for (InputIt head = params_begin; head != params_end; ++head)
    {
      AppendParam(*head, head == params_begin);
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

  void AppendParam(const Param& param, bool first);

  std::string impl_;
  ErrorCode ec_;
};

/**
 * Hasher struct to use for Urls.
 *
 * This is useful for associative containers, such as UrlMap and UrlSet.
 */
struct UrlHasher
{
  std::size_t operator()(const Url& s) const { return std::hash<std::string>()(s.GetAsString()); }
};

/**
 * Equality struct to use for Urls.
 *
 * This is useful for associative containers, such as UrlMap and UrlSet.
 */
struct UrlEquality
{
  bool operator()(const Url& a, const Url& b) const noexcept { return a.GetAsString() == b.GetAsString(); }
};

/**
 * Unordered map of Url objects using custom hasher UrlHasher and custom equality UrlEquality.
 */
template <typename T>
using UrlMap = std::unordered_map<Url, T, UrlHasher, UrlEquality>;

/**
 * Unordered set of Url objects using custom hasher UrlHasher and custom equality UrlEquality.
 */
using UrlSet = std::unordered_set<Url, UrlHasher, UrlEquality>;

// endregion

// region Retry

/**
 * The HTTP response number, such as 404 (Not Found).
 */
using HttpResponseCode = int64_t;

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
  std::chrono::milliseconds timeout = decltype(timeout)::zero();
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

using json::Json;

/**
 * Represents an HTTP GET response: Json object containing response data, and ErrorCode denoting success or failure.
 */
using GetResponse = ValueWithErrorCode<Json>;

/**
 * An unordered map of Url to their corresponding GetResponse objects. This is what is returned when querying multiple
 * Urls at the same time.
 */
using GetMap = UrlMap<GetResponse>;

/**
 * See templated Get function.
 */
ValueWithErrorCode<GetMap> Get(const UrlSet& url_set, int max_connections = 0,
                               const RetryBehavior& retry_behavior = {});

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
inline ValueWithErrorCode<GetMap> Get(InputIt urls_begin, InputIt urls_end, int max_connections = 0,
                                      const RetryBehavior& retry_behavior = {})
{
  const UrlSet url_set(urls_begin, urls_end);
  return Get(url_set, max_connections, retry_behavior);
}

/**
 * See templated Get function.
 */
inline GetResponse Get(const Url& url, int max_connections = 0, const RetryBehavior& retry_behavior = {})
{
  const auto url_list = {url};
  auto data_map = Get(url_list.begin(), url_list.end(), max_connections, retry_behavior);
  const auto iter = data_map.first.find(url);
  const auto& json = iter != data_map.first.end() ? iter->second.first : Json();
  return {json, data_map.second};
}

// endregion Interface
}  // namespace iex::curl

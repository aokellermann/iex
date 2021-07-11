/**
 * @file api.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex/api/api.h"

#include <mutex>
#include <thread>

#include "iex/detail/common.h"
#include "iex/detail/curl_wrapper.h"
#include "iex/detail/json_serializer.h"
#include "iex/detail/utils.h"

namespace iex
{
namespace
{
// region Request Limiting

/**
 * Arbitrary number of retries.
 */
curl::RetryBehavior retry_behavior{3, {kIexHttpTooManyRequests}, true, kDefaultContinuousIexRequestLimitTimeout};

/**
 * Used to ensure only one request at a time.
 */
std::mutex api_call_mutex;

std::chrono::high_resolution_clock::time_point last_call_ts;

// endregion Request Limiting

// region Url Helpers

using Url = curl::Url;
using Param = curl::Url::Param;
using Params = curl::Url::Params;
using UrlEndpointMap = curl::UrlMap<Endpoint::TypeSet>;

Keys api_keys;

const std::string kBaseUrlMap[]{"https://cloud.iexapis.com/", "https://sandbox.iexapis.com/"};

const std::string kVersionUrlMap[]{"stable", /*"latest", */ "v1", "beta"};

Key GetKey(const DataType type)
{
  return type == DataType::AUTHENTIC ? api_keys.secret_key : api_keys.secret_sandbox_key;
}

void AppendParams(Params& params, const Endpoint::Options& options)
{
  for (const auto& opt : options)
  {
    params.insert(Param(opt.key, opt.value));
  }
}

// endregion Url Helpers

// region Init

ErrorCode InnerInit()
{
  auto ec = curl::Init();
  if (ec.Failure())
  {
    return ErrorCode("iex::Init failed", std::move(ec));
  }

  return ErrorCode::kSuccess;
}

// endregion Init
}  // namespace

// region Symbol

Symbol::Symbol(std::string sym) : impl_(std::move(utils::ToUpper(sym))) {}

// endregion Symbol

// region Curl

namespace detail
{
Url GetUrl(const std::string& endpoint_name, const Endpoint::OptionsObject& options)
{
  std::string url_string = kBaseUrlMap[options.data_type] + kVersionUrlMap[options.version] + '/' + endpoint_name;

  Params params{{"token", GetKey(options.data_type)}};
  AppendParams(params, options.options);

  return Url(std::move(url_string), std::move(params));
}

Url GetUrl(const std::unordered_set<std::string>& endpoint_names, const SymbolSet& symbols,
           const Endpoint::OptionsObject& options)
{
  std::string url_string = kBaseUrlMap[options.data_type] + kVersionUrlMap[options.version] + "/stock/market/batch";

  std::unordered_set<std::string> syms;
  syms.reserve(symbols.size());
  for (const auto& sym : symbols)
  {
    syms.insert(sym.Get());
  }

  Params params = {{"symbols", syms.begin(), syms.end()},
                   {"types", endpoint_names.begin(), endpoint_names.end()},
                   {"token", GetKey(options.data_type)}};
  AppendParams(params, options.options);

  return Url(std::move(url_string), std::move(params));
}

/**
 * Curls the given Urls.
 *
 * For now, this is a workaround for avoiding hitting IEX Cloud request limits.
 * Ideally, this would be done through libcurl in a manner such as was
 * suggested here: https://github.com/curl/curl/issues/3920, so we wouldn't need to loop over urls.
 * to introduce a timeout. This approach is quite slow unfortunately...
 * @see https://iexcloud.io/docs/api/#request-limits
 * @param urls UrlSet
 * @return GetMap
 */
json::Json PerformCurl(const curl::Url& url)
{
  std::lock_guard lock(api_call_mutex);
  auto now = std::chrono::high_resolution_clock::now();
  const auto time_since_last_call = now - last_call_ts;
  std::this_thread::sleep_for(retry_behavior.timeout - time_since_last_call);
  auto response = curl::Get(url, 0, retry_behavior);
  last_call_ts = std::move(now);
  return response;
}
}  // namespace detail
// endregion Curl

// region Interface

ErrorCode Init(Keys keys)
{
  api_keys = std::move(keys);
  return InnerInit();
}

void SetRetryBehavior(curl::RetryBehavior new_retry_behavior)
{
  std::lock_guard lock(api_call_mutex);
  retry_behavior = std::move(new_retry_behavior);
}

const curl::RetryBehavior& GetRetryBehavior()
{
  std::lock_guard lock(api_call_mutex);
  return retry_behavior;
}

// endregion Interface

}  // namespace iex

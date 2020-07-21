/**
 * @file api.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex.h"

#include <mutex>
#include <thread>

#include "iex/detail/curl_wrapper.h"
#include "iex/detail/json_serializer.h"
#include "iex/detail/utils.h"

namespace iex
{
namespace
{
// region Perform Curl

/**
 * Arbitrary number of retries.
 */
const curl::RetryBehavior kDefaultRetryBehavior{3, {kIexHttpTooManyRequests}, true, kIexRequestLimitTimeout};

/**
 * Used to ensure only one request at a time.
 */
std::mutex api_call_mutex;

std::chrono::high_resolution_clock::time_point last_call_ts;

// endregion Perform Curl

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
  params.reserve(params.size() + options.size());
  for (const auto& opt : options)
  {
    params.insert(Param(opt.GetName(), {opt.GetValue()}));
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

  return {};
}

// endregion Init
}  // namespace

// region Symbol

Symbol::Symbol(std::string sym) : impl_(std::move(utils::ToUpper(sym))) {}

void Symbol::Set(std::string sym) { impl_ = std::move(utils::ToUpper(sym)); }

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
ValueWithErrorCode<curl::GetMap> PerformCurl(const curl::Url& url)
{
  std::lock_guard lock(api_call_mutex);
  auto now = std::chrono::high_resolution_clock::now();
  const auto time_since_last_call = now - last_call_ts;
  std::this_thread::sleep_for(kIexRequestLimitTimeout - time_since_last_call);
  auto response = curl::Get(curl::UrlSet{url}, 0, kDefaultRetryBehavior);
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

// endregion Interface

}  // namespace iex

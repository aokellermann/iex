/**
 * @file api.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "api.h"

#include <unordered_map>
#include <functional>

#include "iex/api/system_status.h"
#include "iex/curl_wrapper.h"
#include "iex/json_serializer.h"
#include "iex/singleton.h"

namespace iex::api
{
namespace
{
// region Url Helpers

const std::string kBaseUrlMap[]{"https://cloud.iexapis.com/", "https://sandbox.iexapis.com"};

const std::string kVersionUrlMap[]{"stable", /*"latest", */ "v1", "beta"};

const std::vector<const Endpoint*> kEndpoints{&singleton::GetInstance<SystemStatus>(),
                                              /*&singleton::GetInstance<Quote>()*/};

curl::Url GetUrl(const Endpoint::Type& type, const RequestOptions& options)
{
  std::string url_string =
      kBaseUrlMap[options.data_type] + kVersionUrlMap[options.version] + '/' + kEndpoints[type]->GetName();
  curl::Url::Params params;
  params.reserve(options.options.size());
  for (const auto& opt : options.options)
  {
    params.insert(curl::Url::Param(opt->GetName(), {opt->GetValueAsString()}));
  }

  return curl::Url(std::move(url_string), std::move(params));
}

curl::Url GetUrl(const Symbol& symbol, const Endpoint::Type& type, const RequestOptions& options)
{
  std::string url_string = kBaseUrlMap[options.data_type] + kVersionUrlMap[options.version] + "/stock/market/batch";

  curl::Url::Params params = {{"symbols", symbol}, {"types", kEndpoints[type]->GetName()}};
  params.reserve(params.size() + options.options.size());
  for (const auto& opt : options.options)
  {
    params.insert(curl::Url::Param(opt->GetName(), {opt->GetValueAsString()}));
  }

  return curl::Url(std::move(url_string), std::move(params));
}

curl::UrlMap<Endpoint::Type> GetUrls(const AggregatedRequests& requests)
{
  // In the future, this function may call another file/class to optimize calls.
  // For now, it performs no optimization.

  // Only stock endpoint may be batch called according to documentation. https://iexcloud.io/docs/api/#batch-requests

  curl::UrlMap<Endpoint::Type> url_map;
  url_map.reserve(requests.requests.size() + requests.symbol_requests.size());

  // First, create non-symbol-related Urls.
  for (const auto& [type, opts] : requests.requests)
  {
    curl::Url url = GetUrl(type, opts);
    if (url.Validity().Success())
    {
      url_map.emplace(std::move(url), type);
    }
  }

  for (const auto& [symbol, reqs] : requests.symbol_requests)
  {
    for (const auto& [type, opts] : reqs)
    {
      curl::Url url = GetUrl(symbol, type, opts);
      if (url.Validity().Success())
      {
        url_map.emplace(std::move(url), type);
      }
    }
  }

  return url_map;
}

// endregion Url Helpers

// region Endpoint Factories

template <typename E = Endpoint>
ValueWithErrorCode<EndpointPtr<E>> EndpointFactory(const json::Json& input_json)
{
  static_assert(std::is_base_of<Endpoint, E>::value, "E must derive from Endpoint");
  try
  {
    return {std::make_shared<const E>(json::JsonStorage{input_json}), {}};
  }
  catch (const std::exception& e)
  {
    return {nullptr, ErrorCode("Endpoint::Deserialize() failed", {"exception", ErrorCode(e.what())})};
  }
}

template <typename E = SymbolEndpoint>
ValueWithErrorCode<EndpointPtr<E>> EndpointFactory(Symbol symbol, const json::Json& input_json)
{
  static_assert(std::is_base_of<SymbolEndpoint, E>::value, "E must derive from SymbolEndpoint");

  try
  {
    return {std::make_shared<const E>(json::JsonStorage{input_json}, std::move(symbol)), {}};
  }
  catch (const std::exception& e)
  {
    return {nullptr, ErrorCode("SymbolEndpoint::Deserialize() failed", {"exception", ErrorCode(e.what())})};
  }
}

// endregion Endpoint Factories

}  // namespace

// region Interface

ValueWithErrorCode<AggregatedResponses> Get(const AggregatedRequests& requests)
{
  if (requests.requests.empty() && requests.symbol_requests.empty())
  {
    return {{}, {"api::Get() failed", ErrorCode("AggregatedRequests is empty")}};
  }

  const auto type_url_map = GetUrls(requests);
  curl::UrlSet url_set;
  url_set.reserve(type_url_map.size());
  for (const auto& [url, type] : type_url_map)
  {
    url_set.emplace(url);
  }

  auto response = curl::Get(url_set);
  if (response.second.Failure())
  {
    return {{}, {"api::Get() failed", std::move(response.second)}};
  }

  AggregatedResponses aggregated_responses;
  auto get_map = response.first;
  for (auto& [url, json] : get_map)
  {
    const auto iter = type_url_map.find(url);
    if (iter == type_url_map.end())
    {
      return {{}, {"api::Get() failed", ErrorCode("url unexpectedly not found in map")}};
    }

    switch (iter->second)
    {
      default:
        break;

      case Endpoint::Type::SYSTEM_STATUS:
      {
        auto new_endpoint_ptr = EndpointFactory<SystemStatus>(json.first);
        if (new_endpoint_ptr.second.Failure())
        {
          return {{}, {"api::Get() failed", std::move(new_endpoint_ptr.second)}};
        }

        aggregated_responses.responses.Put<Endpoint::Type::SYSTEM_STATUS>(new_endpoint_ptr.first);
        break;
      }

        /*
      case Endpoint::Type::QUOTE:
      {
        auto new_endpoint_ptr = EndpointFactory<Quote>(json.first.items().begin().key(),
                                                       *json.first.items().begin().value().items().begin());
        if (new_endpoint_ptr.second.Failure())
        {
          return {{}, {"api::Get() failed", std::move(new_endpoint_ptr.second)}};
        }

        aggregated_responses.responses.Put<Endpoint::Type::SYSTEM_STATUS>(new_endpoint_ptr.first);
        break;
      }
         */
    }
  }

  return {aggregated_responses, {}};
}

// endregion Interface

}  // namespace iex::api

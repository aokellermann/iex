/**
 * @file api.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "api.h"

#include <functional>
#include <unordered_map>

#include "iex/api/quote.h"
#include "iex/api/system_status.h"
#include "iex/curl_wrapper.h"
#include "iex/json_serializer.h"
#include "iex/keychain.h"
#include "iex/singleton.h"
#include "iex/utils.h"

namespace iex::api
{
namespace
{
// region Url Helpers

using Url = curl::Url;
using Param = curl::Url::Param;
using Params = curl::Url::Params;
using UrlEndpointMap = curl::UrlMap<Endpoint::Type>;

/**
 * This is a pointer to a static singleton.
 */
key::Keychain* keychain = nullptr;

const std::string kBaseUrlMap[]{"https://cloud.iexapis.com/", "https://sandbox.iexapis.com/"};

const std::string kVersionUrlMap[]{"stable", /*"latest", */ "v1", "beta"};

const std::vector<const Endpoint*> kEndpoints{&singleton::GetInstance<SystemStatus>(),
                                              &singleton::GetInstance<Quote>()};

ValueWithErrorCode<key::Keychain::Key> GetKey(const DataType type)
{
  if (keychain == nullptr)
  {
    return {{}, ErrorCode{"Keychain was never created"}};
  }

  return keychain->Get(type == DataType::AUTHENTIC ? key::Keychain::KeyType::SECRET
                                                   : key::Keychain::KeyType::SANDBOX_SECRET);
}

void AppendParams(Params& params, const Endpoint::Options& options)
{
  params.reserve(params.size() + options.size());
  for (const auto& opt : options)
  {
    params.insert(Param(opt.GetName(), {opt.GetValue()}));
  }
}

Url GetUrl(const Endpoint::Type& type, const RequestOptions& options)
{
  std::string url_string =
      kBaseUrlMap[options.data_type] + kVersionUrlMap[options.version] + '/' + kEndpoints[type]->GetName();

  const auto key = GetKey(options.data_type);
  if (key.second.Failure())
  {
    return Url{""};  // Invalid
  }

  Params params{{"token", key.first}};
  AppendParams(params, options.options);

  return Url(std::move(url_string), std::move(params));
}

Url GetUrl(const Symbol& symbol, const Endpoint::Type& type, const RequestOptions& options)
{
  std::string url_string = kBaseUrlMap[options.data_type] + kVersionUrlMap[options.version] + "/stock/market/batch";

  const auto key = GetKey(options.data_type);
  if (key.second.Failure())
  {
    return Url{""};  // Invalid
  }

  Params params = {{"symbols", symbol.Get()}, {"types", kEndpoints[type]->GetName()}, {"token", key.first}};
  AppendParams(params, options.options);

  return Url(std::move(url_string), std::move(params));
}

UrlEndpointMap GetUrls(const AggregatedRequests& requests)
{
  // In the future, this function may call another file/class to optimize calls.
  // For now, it performs no optimization.

  // Only stock endpoint may be batch called according to documentation. https://iexcloud.io/docs/api/#batch-requests

  UrlEndpointMap url_map;
  url_map.reserve(requests.requests.size() + requests.symbol_requests.size());

  // First, create non-symbol-related Urls.
  for (const auto& [type, opts] : requests.requests)
  {
    Url url = GetUrl(type, opts);
    if (url.Validity().Success())
    {
      url_map.emplace(std::move(url), type);
    }
  }

  for (const auto& [symbol, reqs] : requests.symbol_requests)
  {
    for (const auto& [type, opts] : reqs)
    {
      Url url = GetUrl(symbol, type, opts);
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
ValueWithErrorCode<EndpointPtr<E>> EndpointFactory(const json::Json& input_json, Symbol symbol)
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

// region Interface

ErrorCode Init(const key::Keychain::EnvironmentFlag&)
{
  auto ec = InnerInit();
  if (ec.Failure())
  {
    return ec;
  }

  keychain = &singleton::GetInstance<key::Keychain>(key::Keychain::EnvironmentFlag());
  return keychain->KeychainValidity();
}

ErrorCode Init(file::Directory keychain_directory)
{
  auto ec = InnerInit();
  if (ec.Failure())
  {
    return ec;
  }

  keychain = &singleton::GetInstance<key::Keychain>(keychain_directory);
  return keychain->KeychainValidity();
}

ErrorCode SetKey(key::Keychain::KeyType type, const key::Keychain::Key& key)
{
  if (keychain == nullptr)
  {
    return ErrorCode{"Keychain not initialized"};
  }

  return keychain->Set(type, key);
}

bool IsReadyForUse() { return keychain != nullptr && keychain->KeychainValidity().Success() && keychain->Populated(); }

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

      case Endpoint::Type::QUOTE:
      {
        const auto symbol = Symbol(json.first.items().begin().key());
        auto new_endpoint_ptr = EndpointFactory<Quote>(*json.first.items().begin().value().items().begin(), symbol);
        if (new_endpoint_ptr.second.Failure())
        {
          return {{}, {"api::Get() failed", std::move(new_endpoint_ptr.second)}};
        }

        aggregated_responses.symbol_responses.Put<Endpoint::Type::QUOTE>(new_endpoint_ptr.first, symbol);
        break;
      }
    }
  }

  return {aggregated_responses, {}};
}

// endregion Interface

}  // namespace iex::api

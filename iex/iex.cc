/**
 * @file api.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex.h"

#include <functional>
#include <thread>
#include <unordered_map>

#include "iex/api/company.h"
#include "iex/api/quote.h"
#include "iex/api/symbols.h"
#include "iex/api/system_status.h"
#include "iex/curl_wrapper.h"
#include "iex/json_serializer.h"
#include "iex/keychain.h"
#include "iex/singleton.h"
#include "iex/utils.h"

namespace iex
{
namespace
{
// region Perform Curl

constexpr const curl::HttpResponseCode kIexHttpTooManyRequests = 429;

const curl::RetryBehavior kDefaultRetryBehavior{3, {kIexHttpTooManyRequests}};

constexpr const std::chrono::milliseconds kIexTimeout(10);

std::mutex last_api_call_mutex;

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
  std::lock_guard lock(last_api_call_mutex);
  auto response = curl::Get(curl::UrlSet{url}, 0, kDefaultRetryBehavior);
  std::this_thread::sleep_for(kIexTimeout);
  return response;
}

// endregion Perform Curl

// region Url Helpers

using Url = curl::Url;
using Param = curl::Url::Param;
using Params = curl::Url::Params;
using UrlEndpointMap = curl::UrlMap<Endpoint::TypeSet>;

/**
 * This is a pointer to a static singleton.
 */
key::Keychain* keychain = nullptr;

const std::string kBaseUrlMap[]{"https://cloud.iexapis.com/", "https://sandbox.iexapis.com/"};

const std::string kVersionUrlMap[]{"stable", /*"latest", */ "v1", "beta"};

const std::vector<const Endpoint*> kEndpoints{&singleton::GetInstance<Symbols>(),
                                              &singleton::GetInstance<SystemStatus>(), &singleton::GetInstance<Quote>(),
                                              &singleton::GetInstance<Company>()};

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

Url GetUrl(const SymbolSet& symbols, const Endpoint::TypeSet& types, const RequestOptions& options)
{
  std::string url_string = kBaseUrlMap[options.data_type] + kVersionUrlMap[options.version] + "/stock/market/batch";

  const auto key = GetKey(options.data_type);
  if (key.second.Failure())
  {
    return Url{""};  // Invalid
  }

  std::unordered_set<std::string> syms;
  syms.reserve(symbols.size());
  for (const auto& sym : symbols)
  {
    syms.insert(sym.Get());
  }

  std::unordered_set<std::string> ts;
  ts.reserve(types.size());
  for (const auto& t : types)
  {
    ts.insert(kEndpoints[t]->GetName());
  }

  Params params = {{"symbols", syms.begin(), syms.end()}, {"types", ts.begin(), ts.end()}, {"token", key.first}};
  AppendParams(params, options.options);

  return Url(std::move(url_string), std::move(params));
}

EndpointDataMap<UrlEndpointMap> GetUrls(const Requests& requests)
{
  EndpointDataMap<UrlEndpointMap> url_map;

  for (const auto& [type, opts] : requests)
  {
    Url url = GetUrl(type, opts);
    if (url.Validity().Success())
    {
      url_map[opts.version][opts.data_type][url].insert(type);
    }
  }

  return url_map;
}

SymbolMap<EndpointDataMap<UrlEndpointMap>> GetUrls(const SymbolRequests& requests)
{
  // In the future, this function may call another file/class to optimize calls.
  // For now, it performs no optimization.

  // Only stock endpoint may be batch called according to documentation. https://iexcloud.io/docs/api/#batch-requests

  struct BatchMember
  {
    const Symbol& symbol;
    const Endpoint::Type& type;
    const Endpoint::Options& opts;
  };

  EndpointDataMap<std::vector<BatchMember>> batch_map;
  for (const auto& [symbol, reqs] : requests)
  {
    for (const auto& [type, opts] : reqs)
    {
      batch_map[opts.version][opts.data_type].emplace_back(BatchMember{symbol, type, opts.options});
    }
  }

  SymbolMap<EndpointDataMap<UrlEndpointMap>> symbol_endpoint_map;

  SymbolSet symbols;
  Endpoint::TypeSet types;
  Endpoint::Options options;
  for (const auto& [version, data_map] : batch_map)
  {
    for (const auto& [data_type, members] : data_map)
    {
      symbols.clear();
      types.clear();
      options.clear();
      symbols.reserve(members.size());
      types.reserve(members.size());

      for (const auto& member : members)
      {
        symbols.insert(member.symbol);
        types.insert(member.type);
        options.insert(options.end(), member.opts.begin(), member.opts.end());
      }

      Url url = GetUrl(symbols, types, RequestOptions{options, version, data_type});
      if (url.Validity().Success())
      {
        for (const auto& symbol : symbols)
        {
          symbol_endpoint_map[symbol][version][data_type][url].insert(types.begin(), types.end());
        }
      }
    }
  }

  return symbol_endpoint_map;
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

ErrorCode PutEndpoint(AggregatedResponses& aggregated_responses, const curl::GetMap& get_map, const Version& version,
                      const DataType& data_type, const Url& url, const Endpoint::TypeSet& types,
                      const Symbol& symbol = {})
{
  const auto url_json = get_map.find(url);
  if (url_json == get_map.end())
  {
    return ErrorCode{"Url not found in response"};
  }

  if (url_json->second.second.Failure())
  {
    return url_json->second.second;
  }

  const auto& json = url_json->second.first;
  const json::Json* jsym = symbol.Get().empty() ? nullptr : &(*json.find(symbol.Get()));

  for (const auto& type : types)
  {
    const json::Json* jendpoint = jsym == nullptr ? nullptr : &(*jsym->find(kEndpoints[type]->GetName()));

    switch (type)
    {
      case Endpoint::Type::QUOTE:
      case Endpoint::Type::COMPANY:
        if (jendpoint == nullptr)
        {
          return ErrorCode("Endpoint not found in response");
        }
        break;

      default:
        break;
    }

    ValueWithErrorCode<EndpointPtr<>> new_endpoint_ptr;
    switch (type)
    {
      case Endpoint::Type::SYMBOLS:
        new_endpoint_ptr = EndpointFactory<Symbols>(json);
        break;
      case Endpoint::Type::SYSTEM_STATUS:
        new_endpoint_ptr = EndpointFactory<SystemStatus>(json);
        break;
      case Endpoint::Type::QUOTE:
        new_endpoint_ptr = EndpointFactory<Quote>(*jendpoint);
        break;
      case Endpoint::Type::COMPANY:
        new_endpoint_ptr = EndpointFactory<Company>(*jendpoint);
        break;

      default:
        break;
    }

    if (new_endpoint_ptr.second.Failure())
    {
      return ErrorCode("Failed to create endpoint", std::move(new_endpoint_ptr.second));
    }

    std::pair<EndpointPtr<>, RequestOptions> put_pair{new_endpoint_ptr.first,
                                                      RequestOptions{Endpoint::Options{}, version, data_type}};
    switch (type)
    {
      case Endpoint::Type::SYMBOLS:
        aggregated_responses.responses.Put<Endpoint::Type::SYMBOLS>(put_pair.first, put_pair.second);
        break;
      case Endpoint::Type::SYSTEM_STATUS:
        aggregated_responses.responses.Put<Endpoint::Type::SYSTEM_STATUS>(put_pair.first, put_pair.second);
        break;
      case Endpoint::Type::QUOTE:
        aggregated_responses.symbol_responses.Put<Endpoint::Type::QUOTE>(put_pair.first, put_pair.second, symbol);
        break;
      case Endpoint::Type::COMPANY:
        aggregated_responses.symbol_responses.Put<Endpoint::Type::COMPANY>(put_pair.first, put_pair.second, symbol);
        break;

      default:
        break;
    }
  }

  return {};
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
    return {{}, {"iex::Get() failed", ErrorCode("AggregatedRequests is empty")}};
  }

  const auto non_batch_urls = GetUrls(requests.requests);
  const auto batch_urls = GetUrls(requests.symbol_requests);

  curl::UrlSet url_set;
  const auto append_urls = [&url_set](const EndpointDataMap<UrlEndpointMap>& edm) {
    for (const auto& [version, dmap] : edm)
    {
      for (const auto& [data_type, urlmap] : dmap)
      {
        url_set.reserve(urlmap.size());
        for (const auto& [url, _] : urlmap)
        {
          url_set.emplace(url);
        }
      }
    }
  };

  append_urls(non_batch_urls);
  for (const auto& [symbol, vmap] : batch_urls)
  {
    append_urls(vmap);
  }

  AggregatedResponses aggregated_responses;

  for (const auto& current_url : url_set)
  {
    auto response = PerformCurl(current_url);
    if (response.second.Failure())
    {
      return {{}, {"iex::Get() failed", std::move(response.second)}};
    }

    auto get_map = response.first;

    const auto put_endpoints = [&](const EndpointDataMap<UrlEndpointMap>& edm, const Symbol& sym = {}) {
      for (const auto& [version, dmap] : edm)
      {
        for (const auto& [data_type, urlmap] : dmap)
        {
          for (const auto& [url, types] : urlmap)
          {
            PutEndpoint(aggregated_responses, get_map, version, data_type, url, types, sym);
          }
        }
      }
    };

    put_endpoints(non_batch_urls);
    for (const auto& [symbol, vmap] : batch_urls)
    {
      put_endpoints(vmap, symbol);
    }
  }

  return {aggregated_responses, {}};
}

// endregion Interface

}  // namespace iex

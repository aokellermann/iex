/**
 * @file api.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "iex/curl_wrapper.h"
#include "iex/json_serializer.h"

namespace iex::api
{
// region Common Types

using Symbol = std::string;

using SymbolSet = std::unordered_set<Symbol>;

template <typename T>
using SymbolMap = std::unordered_map<Symbol, T>;

using Json = json::Json;

struct Timestamp : std::chrono::milliseconds
{
  using std::chrono::milliseconds::milliseconds;

  Timestamp(const Json& json, const std::string& var_name)
      : std::chrono::milliseconds(json.at(var_name).get<uint64_t>())
  {
  }
};

enum Version
{
  STABLE = 0,
  LATEST,
  V1,
  BETA
};

// endregion Common Types

// region Endpoint

struct Endpoint;

template <typename E = Endpoint>
struct EndpointPtr : std::shared_ptr<const E>
{
  static_assert(std::is_base_of<Endpoint, E>::value, "E must derive from Endpoint");
};

/**
 * Base class for all endpoints.
 */
struct Endpoint : virtual json::JsonDeserializable
{
  using Name = std::string;

  static constexpr int kInvalid = -1;

  enum Type
  {
    NONE = kInvalid,
    SYSTEM_STATUS
  };
  using TypeSet = std::unordered_set<Type>;

  using Option = int;
  using OptionSet = std::unordered_set<Option>;

  template <Type>
  struct Map;

  template <Type T>
  using Typename = typename Endpoint::Map<T>::type;

  [[nodiscard]] virtual std::string GetName() const noexcept = 0;

  [[nodiscard]] virtual NamedPair<std::string> GetNamedOptions(const OptionSet& options) const = 0;

  template <typename E>
  static ValueWithErrorCode<std::shared_ptr<const E>> Factory(const Json& input_json)
  {
    try
    {
      return {std::make_shared<const E>(input_json), {}};
    }
    catch (const std::exception& e)
    {
      return {nullptr, ErrorCode("SystemStatus::Deserialize() failed", {"exception", ErrorCode(e.what())})};
    }
  }
};

// endregion Endpoint

// region Requests and Responses

struct RequestOptions
{
  std::unordered_set<Endpoint::Option> options;
  Version version = Version::STABLE;
};

struct Request
{
  Endpoint::Type type;
  RequestOptions options;
};
using Requests = std::unordered_map<Endpoint::Type, RequestOptions>;

struct SymbolRequest : Request
{
  SymbolRequest(Symbol symbol, Endpoint::Type type, RequestOptions request_options)
      : Request{type, std::move(request_options)}, symbol(std::move(symbol))
  {
  }

  SymbolRequest(Symbol symbol, Endpoint::Type type, std::unordered_set<Endpoint::Option> opts = {},
                Version version = {})
      : SymbolRequest(std::move(symbol), type, {std::move(opts), version})
  {
  }

  Symbol symbol;
};
using SymbolRequests = std::unordered_map<Symbol, Requests>;

struct AggregatedRequests
{
  Requests requests;
  SymbolRequests symbol_requests;
};

class Response
{
  template <Endpoint::Type T>
  [[nodiscard]] EndpointPtr<Endpoint::Typename<T>> Get() const noexcept
  {
    return std::dynamic_pointer_cast<Endpoint::Typename<T>>(endpoint_);
  }

 private:
  EndpointPtr<> endpoint_;
};

class Responses
{
  template <Endpoint::Type T>
  [[nodiscard]] EndpointPtr<Endpoint::Typename<T>> Get() const
  {
    const auto iter = endpoint_map_.find(T);
    return iter != endpoint_map_.end() ? std::dynamic_pointer_cast<Endpoint::Typename<T>>(iter->second) : nullptr;
  }

 private:
  std::unordered_map<Endpoint::Type, EndpointPtr<>> endpoint_map_;
};
class SymbolResponses
{
 public:
  [[nodiscard]] const Responses* Get(const Symbol& symbol) const
  {
    const auto iter = security_map_.find(symbol);
    return iter != security_map_.end() ? &iter->second : nullptr;
  }

 private:
  SymbolMap<Responses> security_map_;
};

struct AggregatedResponses
{
  Responses responses;
  SymbolResponses symbol_responses;
};

// endregion Requests and Responses

// region Interface

ValueWithErrorCode<AggregatedResponses> Get(const AggregatedRequests& requests);

inline ValueWithErrorCode<Responses> Get(const Requests& requests)
{
  const AggregatedRequests aggregated_requests{requests, {}};
  auto response = Get(aggregated_requests);
  return {std::move(response.first.responses), {"Get(Requests) failed", std::move(response.second)}};
}

inline ValueWithErrorCode<SymbolResponses> Get(const SymbolRequests& requests)
{
  const AggregatedRequests aggregated_requests{{}, requests};
  auto response = Get(aggregated_requests);
  return {std::move(response.first.symbol_responses), {"Get(SymbolRequests) failed", std::move(response.second)}};
}

inline ValueWithErrorCode<Responses> Get(const Request& request)
{
  return Get(Requests{{request.type, request.options}});
}

inline ValueWithErrorCode<SymbolResponses> Get(const SymbolRequest& request)
{
  return Get(SymbolRequests{{request.symbol, {{request.type, request.options}}}});
}

template <Endpoint::Type T>
inline ValueWithErrorCode<EndpointPtr<Endpoint::Typename<T>>> Get(const RequestOptions& request_options = {})
{
  const auto response = Get(Request{T, request_options});
  return {response.first.Get<T>(), std::move(response.second)};
}

template <Endpoint::Type T>
inline ValueWithErrorCode<EndpointPtr<Endpoint::Typename<T>>> Get(const Symbol& symbol,
                                                                  const RequestOptions& request_options = {})
{
  const auto response = Get(SymbolRequest{symbol, T, request_options});
  const auto* const ptr = response.first.Get(symbol);
  if (ptr == nullptr)
  {
    return {{}, {"SymbolResponses::Get return nullptr"}};
  }

  return {ptr->Get<T>(), std::move(response.second)};
}

// endregion Interface

}  // namespace iex::api

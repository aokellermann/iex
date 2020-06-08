/**
 * @file api.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "iex/iex.h"
#include "iex/json_serializer.h"
#include "iex/api/forward.h"

namespace iex::api
{
// region Common Types

using Symbol = std::string;
using SymbolSet = std::unordered_set<Symbol>;

template <typename T>
using SymbolMap = std::unordered_map<Symbol, T>;

using Json = json::Json;

/**
 * Represents a timestamp in milliseconds.
 *
 * IEX timestamps are given in unix time with milliseconds.
 */
using Timestamp = std::chrono::milliseconds;

/**
 * @see https://iexcloud.io/docs/api/#api-versioning
 */
enum Version
{
  STABLE,
  // LATEST,
  V1,
  BETA
};

/**
 * @see https://iexcloud.io/docs/api/#testing-sandbox
 */
enum DataType
{
  AUTHENTIC,
  SANDBOX
};

// endregion Common Types

// region Endpoint

template <typename E = Endpoint>
using EndpointPtr = std::shared_ptr<const E>;

/**
 * Base class for all endpoints.
 */
class Endpoint : virtual json::JsonDeserializable
{
 public:
  // region Types

  /**
   * The name of an endpoint that the API accepts as a valid string.
   */
  using Name = std::string;

  /**
   * Enum representing the endpoint.
   */
  enum Type
  {
    /**
     * @see https://iexcloud.io/docs/api/#api-system-metadata
     */
    SYSTEM_STATUS
  };

  /**
   * A unique collection of types.
   */
  using TypeSet = std::unordered_set<Type>;

  /**
   * A map of Endpoint::Type to an arbitrary type.
   */
  template <typename T>
  using TypeMap = std::unordered_map<Type, T>;

  /**
   * Abstract interface used by Option to generate Url Params.
   */
  struct OptionBase
  {
    virtual ~OptionBase() = 0;

    [[nodiscard]] virtual std::string GetName() const = 0;

    [[nodiscard]] virtual std::string GetValueAsString() const = 0;
  };

  template <typename T>
  class Option : OptionBase
  {
   public:
    explicit Option(NamedPair<T> named_pair) : pair_(std::move(named_pair)) {}

    Option(std::string name, T value) : pair_(std::make_pair(std::move(name), std::move(value))) {}

    ~Option() override = default;

    [[nodiscard]] std::string GetName() const override { return pair_.first; }

    [[nodiscard]] std::string GetValueAsString() const override
    {
      std::stringstream sstr;
      sstr << pair_.second;
      return sstr.str();
    }

   private:
    const NamedPair<T> pair_;
  };

  template <typename... Ts>
  using OptionSet = std::tuple<Option<Ts>...>;

  using Options = std::vector<std::shared_ptr<OptionBase>>;

  template <Type>
  struct Map;

  template <>
  struct Map<Endpoint::Type::SYSTEM_STATUS>
  {
    using type = const SystemStatus;
  };

  template <Type T>
  using Typename = typename Endpoint::Map<T>::type;

  // endregion Types

  inline explicit Endpoint(Name name) : name_(std::move(name)) {}

  virtual ~Endpoint() = default;

  [[nodiscard]] inline std::string GetName() const noexcept { return name_; }

 private:
  const Name name_;
};

// endregion Endpoint

// region Requests and Responses

struct RequestOptions
{
  Endpoint::Options options;
  Version version = Version::STABLE;
  DataType data_type = DataType::AUTHENTIC;
};

struct Request
{
  Endpoint::Type type;
  RequestOptions options;
};
using Requests = Endpoint::TypeMap<RequestOptions>;

struct SymbolRequest : Request
{
  SymbolRequest(Symbol symbol, Endpoint::Type type, RequestOptions request_options)
      : Request{type, std::move(request_options)}, symbol(std::move(symbol))
  {
  }

  SymbolRequest(const Symbol& symbol, Endpoint::Type type, const Endpoint::Options& opts = {}, Version version = {})
      : SymbolRequest(symbol, type, RequestOptions{opts, version})
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
 public:
  template <Endpoint::Type T>
  void Put(EndpointPtr<> ptr)
  {
    endpoint_map_.emplace(T, ptr);
  }

  template <Endpoint::Type T>
  [[nodiscard]] EndpointPtr<Endpoint::Typename<T>> Get() const
  {
    const auto iter = endpoint_map_.find(T);
    return iter != endpoint_map_.end() ? std::dynamic_pointer_cast<Endpoint::Typename<T>>(iter->second) : nullptr;
  }

 private:
  Endpoint::TypeMap<EndpointPtr<>> endpoint_map_;
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
  if (response.second.Failure())
  {
    return {{}, {"Get(Requests) failed", std::move(response.second)}};
  }
  return {std::move(response.first.responses), {}};
}

inline ValueWithErrorCode<SymbolResponses> Get(const SymbolRequests& requests)
{
  const AggregatedRequests aggregated_requests{{}, requests};
  auto response = Get(aggregated_requests);
  if (response.second.Failure())
  {
    return {{}, {"Get(SymbolRequests) failed", std::move(response.second)}};
  }
  return {std::move(response.first.symbol_responses), {}};
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

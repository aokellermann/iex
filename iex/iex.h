/**
 * @file api.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <ios>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "iex/api/forward.h"
#include "iex/detail/common.h"
#include "iex/detail/curl_wrapper.h"
#include "iex/detail/json_serializer.h"

namespace iex
{
// region Key

/**
 * API key type
 */
using Key = std::string;

struct Keys
{
  Key public_key;
  Key secret_key;
  Key public_sandbox_key;
  Key secret_sandbox_key;
};

// endregion Key

// region Request Limiting

/**
 * This magic number is determined by conducting prolonged stress tests. The IEX documentation is not accurate
 * as of 6/28/20.
 * @see IexManualTimeoutStress in api_test.cc
 * @see https://iexcloud.io/docs/api/#request-limits
 */
constexpr const std::chrono::milliseconds kIexRequestLimitTimeout(40);

/**
 * @see https://iexcloud.io/docs/api/#error-codes
 */
constexpr const curl::HttpResponseCode kIexHttpTooManyRequests = 429;

// endregion Request Limiting

// region Common Types

// region Symbol

/**
 * Represents a security's symbol. In the case of most funds, this corresponds to it's ticker.
 * @see https://en.wikipedia.org/wiki/Ticker_symbol
 * @example "TSLA", "BRK.A", "AIG+"
 */
class Symbol
{
 public:
  Symbol() = default;

  explicit Symbol(std::string sym);

  void Set(std::string sym);

  [[nodiscard]] const std::string& Get() const noexcept { return impl_; }

  inline bool operator==(const Symbol& other) const { return Get() == other.Get(); }

  struct Hasher
  {
    std::size_t operator()(const Symbol& s) const noexcept { return std::hash<std::string>()(s.Get()); }
  };

 private:
  std::string impl_;
};

using SymbolSet = std::unordered_set<Symbol, Symbol::Hasher>;

template <typename T>
using SymbolMap = std::unordered_map<Symbol, T, Symbol::Hasher>;

// endregion Symbol

// Some generic types
using Price = double;
using Volume = uint64_t;
using Percent = double;

/**
 * @see https://iexcloud.io/docs/api/#api-versioning
 */
enum Version
{
  STABLE,
  // LATEST,  // This doesn't work as of 6/16/20. See https://github.com/iexg/IEX-API/issues/1189
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

class Endpoint;

template <typename E = Endpoint, typename std::enable_if_t<std::is_base_of_v<Endpoint, E>, int> = 0>
using EndpointPtr = std::shared_ptr<const E>;

template <typename E = Endpoint, typename std::enable_if_t<std::is_base_of_v<Endpoint, E>, int> = 0>
using BasicEndpointPtr = EndpointPtr<E>;

/**
 * Base class for all endpoints.
 */
class Endpoint
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
    ENUM_FIRST,
    /**
     * @see https://iexcloud.io/docs/api/#symbols
     */
    SYMBOLS = ENUM_FIRST,
    /**
     * @see https://iexcloud.io/docs/api/#api-system-metadata
     */
    SYSTEM_STATUS,

    /**
     * @see https://iexcloud.io/docs/api/#quote
     */
    QUOTE,

    /**
     * @see https://iexcloud.io/docs/api/#company
     */
    COMPANY,
    ENUM_LAST,
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
   * Used to generate Url Params.
   */
  class OptionBase
  {
   public:
    OptionBase(std::string name, std::string value) : pair_(std::make_pair(std::move(name), std::move(value))) {}

    virtual ~OptionBase() = default;

    [[nodiscard]] virtual std::string GetName() const noexcept { return pair_.first; }

    [[nodiscard]] virtual std::string GetValue() const noexcept { return pair_.second; }

   private:
    NamedPair<std::string> pair_;
  };

  template <typename T>
  class Option : public OptionBase
  {
   public:
    Option(const std::string& name, const T& value) : OptionBase(name, GetValueAsString(value)) {}

    ~Option() override = default;

   private:
    [[nodiscard]] static std::string GetValueAsString(const T& value)
    {
      std::stringstream sstr;
      sstr << std::boolalpha << value;  // Use std::boolalpha to print bools as true/false rather than 1/0
      return sstr.str();
    }
  };

  using Options = std::vector<OptionBase>;

  // endregion Types

  inline Endpoint(Name name, json::JsonStorage data) : data_(std::move(data)), name_(std::move(name)) {}

  virtual ~Endpoint() = default;

  template <typename E>
  static EndpointPtr<E> Factory(const json::Json& input_json)
  {

  }

  [[nodiscard]] inline std::string GetName() const noexcept { return name_; }

 protected:
  const json::JsonStorage data_;

 private:
  const Name name_;
};

struct SymbolEndpoint : Endpoint
{
  SymbolEndpoint() = delete;

  SymbolEndpoint(Symbol sym, Endpoint::Name name, json::JsonStorage data)
      : Endpoint(std::move(name), std::move(data)), symbol(std::move(sym))
  {
  }

  const Symbol symbol;
};

template <Endpoint::Type>
struct EndpointTypedefMap;

template <>
struct EndpointTypedefMap<Endpoint::Type::SYMBOLS>
{
  using type = const Symbols;
  using is_symbol_endpoint = std::false_type;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::SYSTEM_STATUS>
{
  using type = const SystemStatus;
  using is_symbol_endpoint = std::false_type;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::QUOTE>
{
  using type = const Quote;
  using is_symbol_endpoint = std::true_type;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::COMPANY>
{
  using type = const Company;
  using is_symbol_endpoint = std::true_type;
};

template <Endpoint::Type T>
using EndpointTypename = typename EndpointTypedefMap<T>::type;

// region Detail

namespace detail
{
// region SFINAE

template <Endpoint::Type... Types>
struct IsEmpty : std::false_type
{
};

template <>
struct IsEmpty<> : std::true_type
{
};

template <Endpoint::Type... Types>
struct IsSingleton : std::bool_constant<sizeof...(Types) == 1>
{
};

template <Endpoint::Type... Types>
struct IsPlural
    : std::bool_constant<std::conjunction_v<std::negation<IsEmpty<Types...>>, std::negation<IsSingleton<Types...>>>>
{
};

template <Endpoint::Type Type>
struct IsBasicEndpoint : std::negation<typename EndpointTypedefMap<Type>::is_symbol_endpoint>
{
};

template <Endpoint::Type Type>
struct IsSymbolEndpoint : EndpointTypedefMap<Type>::is_symbol_endpoint
{
};

template <Endpoint::Type... Types>
struct AreBasicEndpoints : std::bool_constant<std::conjunction_v<IsBasicEndpoint<Types>...>>
{
};

template <Endpoint::Type... Types>
struct AreSymbolEndpoints : std::bool_constant<std::conjunction_v<IsSymbolEndpoint<Types>...>>
{
};

// endregion SFINAE

// region Url Helpers

using Url = curl::Url;

//Url GetUrl(const Endpoint::Type& type, const RequestOptions& options);

//Url GetUrl(const SymbolSet& symbols, const Endpoint::TypeSet& types, const RequestOptions& options);

// endregion Url Helpers
}  // namespace detail

// endregion Detail

// endregion Endpoint

// region Requests and Responses

struct RequestOptions
{
  Endpoint::Options options;
  Version version = Version::STABLE;
  DataType data_type = DataType::AUTHENTIC;
};

using Request = Endpoint::Type;
using Requests = Endpoint::TypeSet;

struct SymbolRequest
{
  Symbol symbol;
  Endpoint::Type type;
};

struct SymbolRequests
{
  SymbolSet symbols;
  Requests requests;
};

struct AggregatedRequests
{
  Requests requests;
  SymbolRequests symbol_requests;
};

class Responses
{
 public:
  template <Endpoint::Type T>
  void Put(EndpointPtr<> ptr)
  {
    endpoint_map_[T] = std::move(ptr);
  }

  template <Endpoint::Type T>
  [[nodiscard]] EndpointPtr<EndpointTypename<T>> Get() const
  {
    const auto iter = endpoint_map_.find(T);
    return iter != endpoint_map_.end() ? std::dynamic_pointer_cast<EndpointTypename<T>>(iter->second) : nullptr;
  }

 private:
  Endpoint::TypeMap<EndpointPtr<>> endpoint_map_;
};

class SymbolResponses
{
 public:
  template <Endpoint::Type T>
  void Put(EndpointPtr<> ptr, const Symbol& symbol)
  {
    security_map_[symbol].Put<T>(std::move(ptr));
  }

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

// region Detail

namespace detail
{
// region SFINAE

template <Endpoint::Type... Types>
struct IsEmpty : std::false_type
{
};

template <>
struct IsEmpty<> : std::true_type
{
};

template <Endpoint::Type... Types>
struct IsSingleton : std::bool_constant<sizeof...(Types) == 1>
{
};

template <Endpoint::Type... Types>
struct IsPlural
    : std::bool_constant<std::conjunction_v<std::negation<IsEmpty<Types...>>, std::negation<IsSingleton<Types...>>>>
{
};

template <Endpoint::Type Type>
struct IsBasicEndpoint : std::negation<typename EndpointTypedefMap<Type>::is_symbol_endpoint>
{
};

template <Endpoint::Type Type>
struct IsSymbolEndpoint : EndpointTypedefMap<Type>::is_symbol_endpoint
{
};

template <Endpoint::Type... Types>
struct AreBasicEndpoints : std::bool_constant<std::conjunction_v<IsBasicEndpoint<Types>...>>
{
};

template <Endpoint::Type... Types>
struct AreSymbolEndpoints : std::bool_constant<std::conjunction_v<IsSymbolEndpoint<Types>...>>
{
};

// endregion SFINAE

// region Url Helpers

using Url = curl::Url;

Url GetUrl(const Endpoint::Type& type, const RequestOptions& options);

Url GetUrl(const SymbolSet& symbols, const Endpoint::TypeSet& types, const RequestOptions& options);

// endregion Url Helpers
}  // namespace detail

// endregion Detail

// region Interface

/**
 * This function must be called once at program startup, before any other threads have been created.
 * @return ErrorCode denoting whether initialization is successful. If failure, this library will not be able to
 * function properly and the program should exit.
 */
ErrorCode Init(Keys keys);

ValueWithErrorCode<AggregatedResponses> Get(const AggregatedRequests& requests, const RequestOptions& options = {});

template <Endpoint::Type... Types>
using EndpointTuple =
    std::enable_if_t<std::negation_v<detail::IsEmpty<Types...>>, std::tuple<EndpointPtr<EndpointTypename<Types>>...>>;

// Plural Basic Endpoints
template <Endpoint::Type... Types>
std::enable_if_t<std::conjunction_v<detail::IsPlural<Types...>, detail::AreBasicEndpoints<Types...>>,
                 ValueWithErrorCode<EndpointTuple<Types...>>>
Get(const RequestOptions& options = {})
{
  auto resps = Get(AggregatedRequests{{Types}...}, options);
  return {std::make_tuple(resps.first.responses.Get<Types>()...), std::move(resps.second)};
}

// Plural Symbol Endpoints
template <Endpoint::Type... Types>
std::enable_if_t<std::conjunction_v<detail::IsPlural<Types...>, detail::AreSymbolEndpoints<Types...>>,
                 ValueWithErrorCode<EndpointTuple<Types...>>>
Get(const Symbol& symbol, const RequestOptions& options = {})
{
  auto resp = Get(AggregatedRequests{{}, {symbol, {Types}...}}, options);
  return {std::make_tuple(resp.first.symbol_responses.Get(symbol)->Get<Types>()...), std::move(resp.second)};
}

// Plural Symbol Endpoints and Symbols
template <Endpoint::Type... Types>
std::enable_if_t<std::conjunction_v<detail::IsPlural<Types...>, detail::AreSymbolEndpoints<Types...>>,
                 ValueWithErrorCode<SymbolMap<EndpointTuple<Types...>>>>
Get(const SymbolSet& symbols, const RequestOptions& options = {})
{
  auto resp = Get(AggregatedRequests{{}, {symbols, Types...}}, options);

  SymbolMap<EndpointTuple<Types...>> map;
  if (resp.second.Failure())
  {
    map.reserve(symbols.size());
    for (const auto& symbol : symbols)
    {
      map.emplace(symbol, std::make_tuple(resp.first.symbol_responses.Get(symbol)->Get<Types>()...));
    }
  }

  return {std::move(map), std::move(resp.second)};
}

// Single Basic Endpoint
template <Endpoint::Type Type>
std::enable_if_t<detail::IsBasicEndpoint<Type>::value, ValueWithErrorCode<EndpointPtr<EndpointTypename<Type>>>> Get(
    const RequestOptions& options = {})
{
  auto resps = Get(AggregatedRequests{{Type}}, options);
  return {resps.first.responses.Get<Type>(), std::move(resps.second)};
}

// Single Symbol Endpoint
template <Endpoint::Type Type>
std::enable_if_t<detail::IsSymbolEndpoint<Type>::value, ValueWithErrorCode<EndpointPtr<EndpointTypename<Type>>>> Get(
    const Symbol& symbol, const RequestOptions& options = {})
{
  auto resp = Get(AggregatedRequests{{}, {{symbol}, {Type}}}, options);
  return {resp.first.symbol_responses.Get(symbol)->Get<Type>(), std::move(resp.second)};
}

// endregion Interface

}  // namespace iex

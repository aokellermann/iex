/**
 * @file api.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <ios>
#include <memory>
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

  struct OptionsObject
  {
    Endpoint::Options options;
    Version version = Version::STABLE;
    DataType data_type = DataType::AUTHENTIC;
  };

  // endregion Types

  inline Endpoint(Name name, json::JsonStorage data) : data_(std::move(data)), name_(std::move(name)) {}

  virtual ~Endpoint() = default;

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
  static constexpr const char* const kPath = "ref-data/symbols";
  using is_symbol_endpoint = std::false_type;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::SYSTEM_STATUS>
{
  using type = const SystemStatus;
  static constexpr const char* const kPath = "status";
  using is_symbol_endpoint = std::false_type;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::QUOTE>
{
  using type = const Quote;
  static constexpr const char* const kPath = "quote";
  using is_symbol_endpoint = std::true_type;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::COMPANY>
{
  using type = const Company;
  static constexpr const char* const kPath = "company";
  using is_symbol_endpoint = std::true_type;
};

template <Endpoint::Type T>
using EndpointTypename = typename EndpointTypedefMap<T>::type;

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

Url GetUrl(const Endpoint::Type& type, const Endpoint::OptionsObject& options);

Url GetUrl(const SymbolSet& symbols, const Endpoint::TypeSet& types, const Endpoint::OptionsObject& options);

// endregion Url Helpers
}  // namespace detail

template <Endpoint::Type Type>
using EndpointPtr = std::shared_ptr<const EndpointTypename<Type>>;

template <Endpoint::Type Type, typename std::enable_if_t<detail::IsBasicEndpoint<Type>::value, int> = 0>
using BasicEndpointPtr = EndpointPtr<Type>;

template <Endpoint::Type Type, typename std::enable_if_t<detail::IsSymbolEndpoint<Type>::value, int> = 0>
using SymbolEndpointPtr = EndpointPtr<Type>;

template <Endpoint::Type... Types>
using EndpointTuple = std::enable_if_t<std::negation_v<detail::IsEmpty<Types...>>, std::tuple<EndpointPtr<Types>...>>;

template <Endpoint::Type... Types>
using BasicEndpointTuple =
    std::enable_if_t<std::negation_v<detail::IsEmpty<Types...>>, std::tuple<BasicEndpointPtr<Types>...>>;

template <Endpoint::Type... Types>
using SymbolEndpointTuple =
    std::enable_if_t<std::negation_v<detail::IsEmpty<Types...>>, std::tuple<SymbolEndpointPtr<Types>...>>;

template <Endpoint::Type Type>
auto EndpointFactory(const json::Json& input_json)
{
  return std::make_shared<typename BasicEndpointPtr<Type>::element_type>(json::JsonStorage{input_json});
}

template <Endpoint::Type Type>
auto EndpointFactory(const json::Json& input_json, const Symbol& symbol)
{
  return std::make_shared<typename SymbolEndpointPtr<Type>::element_type>(json::JsonStorage{input_json}, symbol);
}

namespace detail
{
// region Curl

ValueWithErrorCode<curl::GetMap> PerformCurl(const curl::Url& url);

template <Endpoint::Type Type>
ValueWithErrorCode<BasicEndpointPtr<Type>> Get(const Endpoint::OptionsObject& options)
{
  const auto url = GetUrl(Type, options);
  auto vec = PerformCurl(url);
  if (vec.second.Failure())
  {
    return {{}, std::move(vec.second)};
  }

  try
  {
    auto json = vec.first[url].first;
    return {EndpointFactory<Type>(json), {}};
  }
  catch (const std::exception& e)
  {
    return {{}, ErrorCode("Get() failed", ErrorCode(e.what()))};
  }
}

template <Endpoint::Type... Types>
ValueWithErrorCode<SymbolMap<SymbolEndpointTuple<Types...>>> Get(const SymbolSet& symbols,
                                                                 const Endpoint::OptionsObject& options)
{
  const auto url = GetUrl(symbols, Endpoint::TypeSet{Types...}, options);
  auto vec = PerformCurl(url);
  if (vec.second.Failure())
  {
    return {{}, std::move(vec.second)};
  }

  try
  {
    auto json = vec.first[url].first;
    SymbolMap<SymbolEndpointTuple<Types...>> map;
    map.reserve(symbols.size());
    for (const auto& symbol : symbols)
    {
      map.emplace(symbol,
                  std::make_tuple(EndpointFactory<Types>(json[symbol.Get()][EndpointTypedefMap<Types>::kPath])...));
    }

    return {std::move(map), {}};
  }
  catch (const std::exception& e)
  {
    return {{}, ErrorCode("Get() failed", ErrorCode(e.what()))};
  }
}

// endregion Curl
}  // namespace detail

// endregion Endpoint

// region Interface

/**
 * This function must be called once at program startup, before any other threads have been created.
 * @return ErrorCode denoting whether initialization is successful. If failure, this library will not be able to
 * function properly and the program should exit.
 */
ErrorCode Init(Keys keys);

// Symbol Endpoints
template <Endpoint::Type... Types>
auto Get(const Symbol& symbol, const Endpoint::OptionsObject& options = {})
{
  auto resp = detail::Get<Types...>({symbol}, options);
  if constexpr (detail::IsPlural<Types...>::value)
  {
    using return_type = ValueWithErrorCode<SymbolEndpointTuple<Types...>>;
    return resp.second.Success() ? return_type{std::move(resp.first[symbol]), {}}
                                 : return_type{{}, std::move(resp.second)};
  }
  else
  {
    using return_type = ValueWithErrorCode<SymbolEndpointPtr<std::get<0>(std::make_tuple(Types...))>>;
    return resp.second.Success() ? return_type{std::move(std::get<0>(resp.first[symbol])), {}}
                                 : return_type{{}, std::move(resp.second)};
  }
}

// Plural Symbol Endpoints and Symbols
template <Endpoint::Type... Types>
auto Get(const SymbolSet& symbols, const Endpoint::OptionsObject& options = {})
{
  return detail::Get<Types...>(symbols, options);
}

// Single Basic Endpoint
template <Endpoint::Type Type>
ValueWithErrorCode<BasicEndpointPtr<Type>> Get(const Endpoint::OptionsObject& options = {})
{
  return detail::Get<Type>(options);
}

// endregion Interface

}  // namespace iex

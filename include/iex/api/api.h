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

/**
 * Collection of keys used for initialization.
 */
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
 *
 * Note: this number seems to be different if run on CI. Maybe this should be configurable.
 * @see IexManualTimeoutStress in api_test.cc
 * @see https://iexcloud.io/docs/api/#request-limits
 */
constexpr const curl::TimeoutDuration kDefaultContinuousIexRequestLimitTimeout(40);

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
class Endpoint : public json::JsonBidirectionalSerializable
{
 public:
  // region Types

  /**
   * The path of an endpoint that the API accepts as a valid string.
   */
  using Path = const char* const;

  /**
   * Human-readable endpoint name.
   */
  using Name = const char* const;

  /**
   * Enum representing the endpoint.
   */
  enum Type
  {
    /**
     * Class of basic endpoints. Cannot be queried.
     */
    BASIC = -100,
    /**
     * Class of stock endpoints. Cannot be queried.
     */
    STOCK,
    ENUM_FIRST = 0,
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
  struct OptionBase : KVP<std::string>
  {
   public:
    using KVP::KVP;
    ~OptionBase() override = default;
  };

  template <typename T>
  class Option : public OptionBase
  {
   public:
    explicit Option(const KVP<T>& kvp) : OptionBase(kvp.key, ValueString(kvp.value)) {}
    ~Option() override = default;

   private:
    [[nodiscard]] std::string ValueString(const T& value) const
    {
      std::stringstream sstr;
      sstr << std::boolalpha << value;  // Use std::boolalpha to print bools as true/false rather than 1/0
      return sstr.str();
    }
  };

  using Options = std::vector<KVP<std::string>>;

  struct OptionsObject
  {
    Endpoint::Options options;
    Version version = Version::STABLE;
    DataType data_type = DataType::AUTHENTIC;
  };

  // endregion Types

  inline explicit Endpoint(json::JsonStorage data)
      : data_(std::move(data)),
        timestamp_(std::chrono::duration_cast<Timestamp>(std::chrono::system_clock::now().time_since_epoch()))
  {
  }

  ~Endpoint() override = default;

  [[nodiscard]] inline Timestamp GetTimestamp() const noexcept { return timestamp_; }

  // region Json

  [[nodiscard]] json::Json Serialize() const override { return data_.Serialize(); }

  void Deserialize(const json::Json& input_json) override { return data_.Deserialize(input_json); }

  // endregion Json

 protected:
  json::JsonStorage data_;

 private:
  const Timestamp timestamp_;
};

/**
 * Base class for all stock endpoints.
 */
struct StockEndpoint : Endpoint
{
  StockEndpoint() = delete;

  StockEndpoint(Symbol sym, json::JsonStorage data) : Endpoint(std::move(data)), symbol(std::move(sym)) {}

  const Symbol symbol;
};

/**
 * For each endpoint class specialization, the following is defined:
 * type: Corresponding base class type.
 *
 * For each endpoint specialization, the following is defined:
 * type: Corresponding class type.
 * kPath: Url endpoint path.
 * kName: Human-readable label.
 * kClassType: Class of endpoint type. For now there is only basic and stock.
 */
template <Endpoint::Type>
struct EndpointTypedefMap;

template <>
struct EndpointTypedefMap<Endpoint::Type::BASIC>
{
  using type = Endpoint;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::STOCK>
{
  using type = StockEndpoint;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::SYMBOLS>
{
  using type = Symbols;
  static constexpr Endpoint::Path kPath = "ref-data/symbols";
  static constexpr Endpoint::Name kName = "Stock Symbols";
  static constexpr Endpoint::Type kClassType = Endpoint::Type::BASIC;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::SYSTEM_STATUS>
{
  using type = SystemStatus;
  static constexpr Endpoint::Path kPath = "status";
  static constexpr Endpoint::Name kName = "System Status";
  static constexpr Endpoint::Type kClassType = Endpoint::Type::BASIC;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::QUOTE>
{
  using type = Quote;
  static constexpr Endpoint::Path kPath = "quote";
  static constexpr Endpoint::Name kName = "Quote";
  static constexpr Endpoint::Type kClassType = Endpoint::Type::STOCK;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::COMPANY>
{
  using type = Company;
  static constexpr Endpoint::Path kPath = "company";
  static constexpr Endpoint::Name kName = "Company";
  static constexpr Endpoint::Type kClassType = Endpoint::Type::STOCK;
};

/**
 * Helper for getting the corresponding class type for an Endpoint::Type.
 */
template <Endpoint::Type T>
using EndpointTypename = typename EndpointTypedefMap<T>::type;

// region SFINAE

namespace detail
{
template <auto... Ts>
struct ParameterCount : std::integral_constant<std::size_t, sizeof...(Ts)>
{
};
}  // namespace detail

template <auto... Ts>
struct IsEmpty : std::bool_constant<detail::ParameterCount<Ts...>::value == 0>
{
};

template <auto... Ts>
struct IsSingleton : std::bool_constant<detail::ParameterCount<Ts...>::value == 1>
{
};

template <auto... Ts>
struct IsPlural : std::bool_constant<detail::ParameterCount<Ts...>::value >= 2>
{
};

template <Endpoint::Type Type>
struct IsBasicEndpoint : std::bool_constant<EndpointTypedefMap<Type>::kClassType == Endpoint::Type::BASIC>
{
};

template <Endpoint::Type Type>
struct IsStockEndpoint : std::bool_constant<EndpointTypedefMap<Type>::kClassType == Endpoint::Type::STOCK>
{
};

template <Endpoint::Type... Types>
struct AreBasicEndpoints : std::bool_constant<std::conjunction_v<IsBasicEndpoint<Types>...>>
{
};

template <Endpoint::Type... Types>
struct AreStockEndpoints : std::bool_constant<std::conjunction_v<IsStockEndpoint<Types>...>>
{
};

// endregion SFINAE

namespace detail
{
// region Url Helpers

using Url = curl::Url;

Url GetUrl(const std::string& endpoint_name, const Endpoint::OptionsObject& options);

template <Endpoint::Type Type>
Url GetUrl(const Endpoint::OptionsObject& options)
{
  return GetUrl(EndpointTypedefMap<Type>::kPath, options);
}

Url GetUrl(const std::unordered_set<std::string>& endpoint_names,
           const SymbolSet& symbols,
           const Endpoint::OptionsObject& options);

template <Endpoint::Type... Types>
Url GetUrl(const SymbolSet& symbols, const Endpoint::OptionsObject& options)
{
  return GetUrl({EndpointTypedefMap<Types>::kPath...}, symbols, options);
}

// endregion Url Helpers
}  // namespace detail

/**
 * Pointer type for Endpoints.
 */
template <Endpoint::Type Type>
using EndpointPtr = std::shared_ptr<const EndpointTypename<Type>>;

/**
 * Pointer type for non-Stock Endpoints.
 */
template <Endpoint::Type Type>
using BasicEndpointPtr = std::enable_if_t<IsBasicEndpoint<Type>::value, EndpointPtr<Type>>;

/**
 * Pointer type for Stock Endpoints.
 */
template <Endpoint::Type Type>
using StockEndpointPtr = std::enable_if_t<IsStockEndpoint<Type>::value, EndpointPtr<Type>>;

/**
 * Tuple of Endpoints.
 */
template <template <Endpoint::Type> typename PtrType = EndpointPtr, Endpoint::Type... Types>
using EndpointTuple = std::enable_if_t<std::negation_v<IsEmpty<Types...>>, std::tuple<PtrType<Types>...>>;

/**
 * Tuple of non-Stock Endpoints.
 */
template <Endpoint::Type... Types>
using BasicEndpointTuple = EndpointTuple<BasicEndpointPtr, Types...>;

/**
 * Tuple of Stock Endpoints.
 */
template <Endpoint::Type... Types>
using StockEndpointTuple = EndpointTuple<StockEndpointPtr, Types...>;

/**
 * Factory method for Basic Endpoints.
 * @tparam Type a Basic Endpoint enumeration member.
 * @param input_json input JSON data
 * @return a BasicEndpointPtr
 */
template <Endpoint::Type Type>
auto EndpointFactory(const json::Json& input_json)
{
  return std::make_shared<typename BasicEndpointPtr<Type>::element_type>(json::JsonStorage{input_json});
}

/**
 * Factory method for Stock Endpoints.
 * @tparam Type a Stock Endpoint enumeration member.
 * @param input_json input JSON data
 * @param symbol the stock's symbol
 * @return a StockEndpointPtr
 */
template <Endpoint::Type Type>
auto EndpointFactory(const json::Json& input_json, const Symbol& symbol)
{
  return std::make_shared<typename StockEndpointPtr<Type>::element_type>(json::JsonStorage{input_json}, symbol);
}

namespace detail
{
// region Curl

json::Json PerformCurl(const curl::Url& url);

template <Endpoint::Type Type>
BasicEndpointPtr<Type> Get(const Endpoint::OptionsObject& options)
{
  const auto url = GetUrl<Type>(options);
  auto json = PerformCurl(url);
  if (json.is_null()) return nullptr;
  return EndpointFactory<Type>(json);
}

template <Endpoint::Type... Types>
auto Get(const SymbolSet& symbols, const Endpoint::OptionsObject& options)
{
  if constexpr (IsPlural<Types...>::value)
  {
    using return_type = SymbolMap<StockEndpointTuple<Types...>>;

    const auto url = GetUrl<Types...>(symbols, options);
    const auto json = PerformCurl(url);
    if (json.is_null()) return return_type();

    return_type map;
    try
    {
      map.reserve(symbols.size());
      for (const auto& symbol : symbols)
      {
        if (const auto symbol_iter = json.find(symbol.Get()); symbol_iter != json.end())
        {
          map.emplace(symbol,
                      std::make_tuple(
                          (symbol_iter->find(EndpointTypedefMap<Types>::kPath) != symbol_iter->end()
                               ? EndpointFactory<Types>(*symbol_iter->find(EndpointTypedefMap<Types>::kPath), symbol)
                               : nullptr)...));
        }
      }
    }
    catch (...)
    {
    }

    return map;
  }
  else
  {
    constexpr Endpoint::Type kType = std::get<0>(std::make_tuple(Types...));
    using return_type = SymbolMap<StockEndpointPtr<kType>>;

    const auto url = GetUrl<kType>(symbols, options);
    const auto json = PerformCurl(url);
    if (json.is_null()) return return_type();

    return_type map;
    try
    {
      map.reserve(symbols.size());
      for (const auto& symbol : symbols)
      {
        if (const auto symbol_iter = json.find(symbol.Get()); symbol_iter != json.end())
        {
          if (const auto endpoint_iter = symbol_iter->find(EndpointTypedefMap<kType>::kPath);
              endpoint_iter != symbol_iter->end())
          {
            map.emplace(symbol, EndpointFactory<kType>(*endpoint_iter, symbol));
          }
        }
      }
    }
    catch (...)
    {
    }

    return map;
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

/**
 * Sets the new RetryBehavior for all API calls.
 * @param new_retry_behavior RetryBehavior
 */
void SetRetryBehavior(curl::RetryBehavior new_retry_behavior);

/**
 * Gets a reference to the current RetryBehavior. The reference is invalidated if you call SetRetryBehavior().
 * @return const reference to current RetryBehavior
 */
const curl::RetryBehavior& GetRetryBehavior();

/**
 * Fetches from IEX Cloud the given Endpoint::Types for the given symbol.
 * @tparam Types A parameter pack of Endpoint::Types (must have positive length)
 * @param symbol The symbol's data to fetch
 * @param options Optional query options
 * @return The fetched data along with an ErrorCode. The ErrorCode will indicate failure if at least one Endpoint
 * failed. Successful endpoints will be non-nullptr if they succeeded even if one failed.
 */
template <Endpoint::Type... Types>
auto Get(const Symbol& symbol, const Endpoint::OptionsObject& options = {})
{
  auto map = detail::Get<Types...>({symbol}, options);
  auto node = map.extract(symbol);
  using return_value_type = typename decltype(node)::mapped_type;
  return return_value_type{!node.empty() ? return_value_type{std::move(node.mapped())} : return_value_type{}};
}

/**
 * Fetches from IEX Cloud the given Endpoint::Types for the given symbols.
 * @tparam Types A parameter pack of Endpoint::Types (must have positive length)
 * @param symbols A collection of symbols to fetch data for
 * @param options Optional query options
 * @return The fetched data along with an ErrorCode. The ErrorCode will indicate failure if at least one Endpoint
 * failed. Successful endpoints will be non-nullptr if they succeeded even if one failed.
 */
template <Endpoint::Type... Types>
auto Get(const SymbolSet& symbols, const Endpoint::OptionsObject& options = {})
{
  return detail::Get<Types...>(symbols, options);
}

/**
 * Fetches from IEX Cloud the given Endpoint::Type.
 * @tparam Type The Endpoint::Type to fetch
 * @param options Optional query options
 * @return The fetched data along with an ErrorCode.
 */
template <Endpoint::Type Type>
auto Get(const Endpoint::OptionsObject& options = {})
{
  return detail::Get<Type>(options);
}

// endregion Interface

}  // namespace iex

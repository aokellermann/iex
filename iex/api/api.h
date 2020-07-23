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
     * @see https://iexcloud.io/docs/api/#symbols
     */
    SYMBOLS,
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

  inline explicit Endpoint(json::JsonStorage data)
      : data_(std::move(data)),
        timestamp_(std::chrono::duration_cast<Timestamp>(std::chrono::system_clock::now().time_since_epoch()))
  {
  }

  ~Endpoint() override = default;

  [[nodiscard]] inline Timestamp GetTimestamp() const noexcept { return timestamp_; }

  // region Json

  [[nodiscard]] ValueWithErrorCode<json::Json> Serialize() const override { return data_.Serialize(); }

  ErrorCode Deserialize(const json::Json& input_json) override { return data_.Deserialize(input_json); }

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
 * For each specialization, the following is defined:
 * type: Corresponding class type.
 * kPath: Url endpoint path.
 * kName: Human-readable label.
 * is_stock_endpoint: Indication of whether the endpoint is a stock endpoint (useful for batch calls).
 */
template <Endpoint::Type>
struct EndpointTypedefMap;

template <>
struct EndpointTypedefMap<Endpoint::Type::SYMBOLS>
{
  using type = Symbols;
  static constexpr Endpoint::Path kPath = "ref-data/symbols";
  static constexpr Endpoint::Name kName = "Stock Symbols";
  using is_stock_endpoint = std::false_type;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::SYSTEM_STATUS>
{
  using type = SystemStatus;
  static constexpr Endpoint::Path kPath = "status";
  static constexpr Endpoint::Name kName = "System Status";
  using is_stock_endpoint = std::false_type;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::QUOTE>
{
  using type = Quote;
  static constexpr Endpoint::Path kPath = "quote";
  static constexpr Endpoint::Name kName = "Quote";
  using is_stock_endpoint = std::true_type;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::COMPANY>
{
  using type = Company;
  static constexpr Endpoint::Path kPath = "company";
  static constexpr Endpoint::Name kName = "Company";
  using is_stock_endpoint = std::true_type;
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
struct IsBasicEndpoint : std::negation<typename EndpointTypedefMap<Type>::is_stock_endpoint>
{
};

template <Endpoint::Type Type>
struct IsStockEndpoint : EndpointTypedefMap<Type>::is_stock_endpoint
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

ValueWithErrorCode<curl::GetMap> PerformCurl(const curl::Url& url);

template <Endpoint::Type Type>
ValueWithErrorCode<BasicEndpointPtr<Type>> Get(const Endpoint::OptionsObject& options)
{
  const auto url = GetUrl<Type>(options);
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
auto Get(const SymbolSet& symbols, const Endpoint::OptionsObject& options)
{
  ErrorCode ec;

  if constexpr (IsPlural<Types...>::value)
  {
    using return_type = ValueWithErrorCode<SymbolMap<StockEndpointTuple<Types...>>>;

    const auto url = GetUrl<Types...>(symbols, options);
    auto vec = PerformCurl(url);
    if (vec.second.Failure())
    {
      return return_type{{}, std::move(vec.second)};
    }

    SymbolMap<StockEndpointTuple<Types...>> map;
    try
    {
      const auto& json = vec.first[url].first;
      map.reserve(symbols.size());
      for (const auto& symbol : symbols)
      {
        const auto symbol_iter = json.find(symbol.Get());
        if (symbol_iter != json.end())
        {
          const auto endpoint_iter =
              map.emplace(symbol,
                          std::make_tuple((
                                              symbol_iter->find(EndpointTypedefMap<Types>::kPath) != symbol_iter->end()
                                              ? EndpointFactory<Types>(*symbol_iter->find(EndpointTypedefMap<Types>::kPath), symbol)
                                              : nullptr)...))
                  .first;
          std::apply(
              [&ec](auto&&... args) {
                ((ec = args == nullptr ? ErrorCode("Get() failed", ErrorCode("no symbol in json")) : ec), ...);
              },
              endpoint_iter->second);
        }
        else
        {
          ec = ErrorCode("Get() failed", ErrorCode("no symbol in json"));
        }
      }
    }
    catch (const std::exception& e)
    {
      ec = ErrorCode("Get() failed", ErrorCode(e.what()));
    }

    return return_type{std::move(map), std::move(ec)};
  }
  else
  {
    constexpr Endpoint::Type kType = std::get<0>(std::make_tuple(Types...));
    using return_type = ValueWithErrorCode<SymbolMap<StockEndpointPtr<kType>>>;

    const auto url = GetUrl<kType>(symbols, options);
    auto vec = PerformCurl(url);
    if (vec.second.Failure())
    {
      return return_type{{}, std::move(vec.second)};
    }

    SymbolMap<StockEndpointPtr<kType>> map;
    try
    {
      const auto& json = vec.first[url].first;
      map.reserve(symbols.size());
      for (const auto& symbol : symbols)
      {
        const auto symbol_iter = json.find(symbol.Get());
        if (symbol_iter != json.end())
        {
          const auto endpoint_iter = symbol_iter->find(EndpointTypedefMap<kType>::kPath);
          if (endpoint_iter != symbol_iter->end())
          {
            map.emplace(symbol, EndpointFactory<kType>(*endpoint_iter, symbol));
          }
          else
          {
            ec = ErrorCode("Get() failed", ErrorCode("no endpoint in json"));
          }
        }
        else
        {
          ec = ErrorCode("Get() failed", ErrorCode("no symbol in json"));
        }
      }
    }
    catch (const std::exception& e)
    {
      ec = ErrorCode("Get() failed", ErrorCode(e.what()));
    }

    return return_type{std::move(map), std::move(ec)};
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
  auto [map, ec] = detail::Get<Types...>({symbol}, options);
  auto node = map.extract(symbol);
  using return_value_type = typename decltype(node)::mapped_type;
  return ValueWithErrorCode<return_value_type>{
      !node.empty() ? return_value_type{std::move(node.mapped())} : return_value_type{}, std::move(ec)};
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

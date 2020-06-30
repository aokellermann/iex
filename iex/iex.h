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
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "iex/api/forward.h"
#include "iex/common.h"
#include "iex/curl_wrapper.h"
#include "iex/json_serializer.h"
#include "iex/keychain.h"

namespace iex
{
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

// iex types
using Keychain = key::Keychain;

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

template <typename E = Endpoint>
using EndpointPtr = std::shared_ptr<const E>;

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
};

template <>
struct EndpointTypedefMap<Endpoint::Type::SYSTEM_STATUS>
{
  using type = const SystemStatus;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::QUOTE>
{
  using type = const Quote;
};

template <>
struct EndpointTypedefMap<Endpoint::Type::COMPANY>
{
  using type = const Company;
};

template <Endpoint::Type T>
using EndpointTypename = typename EndpointTypedefMap<T>::type;

template <typename T>
using EndpointDataMap = std::unordered_map<Version, std::unordered_map<DataType, T>>;

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
  SymbolRequest(Symbol sym, Request request) : Request(std::move(request)), symbol(std::move(sym)) {}

  Symbol symbol;
};
using SymbolRequests = SymbolMap<Requests>;

struct AggregatedRequests
{
  Requests requests;
  SymbolRequests symbol_requests;
};

class Responses
{
 public:
  template <Endpoint::Type T>
  void Put(EndpointPtr<> ptr, const RequestOptions& options)
  {
    endpoint_map_[options.version][options.data_type].emplace(T, std::move(ptr));
  }

  template <Endpoint::Type T>
  [[nodiscard]] EndpointPtr<EndpointTypename<T>> Get(const RequestOptions& options = {}) const
  {
    const auto viter = endpoint_map_.find(options.version);
    if (viter == endpoint_map_.end())
    {
      return nullptr;
    }
    const auto diter = viter->second.find(options.data_type);
    if (diter == viter->second.end())
    {
      return nullptr;
    }
    const auto iter = diter->second.find(T);
    return iter != diter->second.end() ? std::dynamic_pointer_cast<EndpointTypename<T>>(iter->second) : nullptr;
  }

 private:
  EndpointDataMap<Endpoint::TypeMap<EndpointPtr<>>> endpoint_map_;
};

class SymbolResponses
{
 public:
  template <Endpoint::Type T>
  void Put(EndpointPtr<> ptr, const RequestOptions& options, const Symbol& symbol)
  {
    security_map_[symbol].Put<T>(std::move(ptr), options);
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

// region Interface

/**
 * This or the other Init() function must be called once at program startup, before any other threads have been created.
 * @return ErrorCode denoting whether initialization is successful. If failure, this library will not be able to
 * function properly and the program should exit.
 */
ErrorCode Init(const Keychain::EnvironmentFlag&);

/**
 * This or the other Init() function must be called once at program startup, before any other threads have been created.
 * @param keychain_directory The directory to use to store API keys.
 * @return ErrorCode denoting whether initialization is successful. If failure, this library will not be able to
 * function properly and the program should exit.
 */
ErrorCode Init(file::Directory keychain_directory = file::Directory::HOME);

/**
 * @see api::Keychain::Set
 */
ErrorCode SetKey(Keychain::KeyType type, const Keychain::Key& key);

/**
 * @return True if all API keys are set or false otherwise.
 */
bool IsReadyForUse();

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
inline ValueWithErrorCode<EndpointPtr<EndpointTypename<T>>> Get(const RequestOptions& request_options = {})
{
  static_assert(T == Endpoint::Type::SYMBOLS || T == Endpoint::Type::SYSTEM_STATUS, "T is not of valid type");

  const auto response = Get(Request{T, request_options});
  return {response.first.Get<T>(request_options), std::move(response.second)};
}

template <Endpoint::Type T>
inline ValueWithErrorCode<EndpointPtr<EndpointTypename<T>>> Get(const Symbol& symbol,
                                                                const RequestOptions& request_options = {})
{
  static_assert(T == Endpoint::Type::QUOTE || T == Endpoint::Type::COMPANY, "T is not of valid type");

  const auto response = Get(SymbolRequest{symbol, {T, request_options}});
  const auto* const ptr = response.first.Get(symbol);
  if (ptr == nullptr)
  {
    return {{}, ErrorCode{"SymbolResponses::Get return nullptr"}};
  }

  return {ptr->Get<T>(request_options), std::move(response.second)};
}

// endregion Interface

}  // namespace iex

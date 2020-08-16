/**
 * @file symbols.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <string>
#include <utility>

#include "iex/detail/json_serializer.h"
#include "iex/iex.h"

namespace iex
{
/**
 * @see https://iexcloud.io/docs/api/#symbols
 *
 * Data format as of 6/24/20:
 * [
 *   {
 *     "symbol": "ZYXI",
 *     "exchange": "NAS",
 *     "name": "Zynex Inc.",
 *     "date": "2020-06-24",
 *     "type": "cs",
 *     "iexId": "IEX_4E464C4C4A462D52",
 *     "region": "US",
 *     "currency": "USD",
 *     "isEnabled": true,
 *     "figi": "BBG000BJBXZ2",
 *     "cik": null
 *   },
 *   ...
 * ]
 */
class Symbols : public Endpoint
{
 private:
  class SearchComparator;

 public:
  enum MemberType
  {
    ENUM_FIRST,
    /**
     * refers to Exchange using IEX Supported Exchanges list
     * @see https://iexcloud.io/docs/api/#u-s-exchanges
     */
    EXCHANGE = ENUM_FIRST,
    /**
     * refers to the name of the company or security.
     */
    NAME,
    /**
     * refers to the date the symbol reference data was generated.
     */
    DATE,
    /**
     * refers to the common issue type
     * ad - ADR
     * re - REIT
     * ce - Closed end fund
     * si - Secondary Issue
     * lp - Limited Partnerships
     * cs - Common Stock
     * et - ETF
     * wt - Warrant
     * oef - Open Ended Fund
     * cef - Closed Ended Fund
     * ps - Preferred Stock
     * ut - Unit
     * struct - Structured Product
     */
    TYPE,
    /**
     * unique ID applied by IEX to track securities through symbol changes.
     */
    IEX_ID,
    /**
     * refers to the country code for the symbol using ISO 3166-1 alpha-2
     * @see https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
     */
    REGION,
    /**
     * refers to the currency the symbol is traded in using ISO 4217
     * @see https://en.wikipedia.org/wiki/ISO_4217
     */
    CURRENCY,
    /**
     * will be true if the symbol is enabled for trading on IEX.
     */
    IS_ENABLED,
    /**
     * The FIGI id for the security if available
     */
    FIGI,
    /**
     * CIK number for the security if available
     */
    CIK,
    ENUM_LAST,
  };

 private:
  template <Symbols::MemberType>
  struct MemberMap;

  template <Symbols::MemberType T>
  using MemberTypename = typename MemberMap<T>::type;

 public:
  class Member
  {
   public:
    explicit Member(const json::Json& data) : data_(data) {}

    template <MemberType T>
    json::Member<MemberTypename<T>> Get() const noexcept
    {
      return data_.SafeGetMember<MemberTypename<T>>(MemberMap<T>::kName);
    }

   private:
    const json::JsonStorage data_;
  };

  explicit Symbols(json::JsonStorage data = json::JsonStorage{}) : Endpoint(std::move(data)) {}

  ~Symbols() override = default;

  [[nodiscard]] json::Member<Member> Get(const Symbol& symbol) const
  {
    const auto iter = std::lower_bound(data_.begin(), data_.end(), symbol, search_comparator_);
    return iter != data_.end() ? std::make_optional(Member(*iter)) : std::nullopt;
  }

 private:
  class SearchComparator
  {
   public:
    bool operator()(const Symbol& left, const Symbol& right) { return left.Get() < right.Get(); }
    bool operator()(const Symbol& left, const json::Json& right) { return left.Get() < GetSymbol(right); }
    bool operator()(const json::Json& left, const Symbol& right) { return GetSymbol(left) < right.Get(); }

   private:
    static std::string GetSymbol(const json::Json& json)
    {
      return *json::JsonStorage::SafeGetMember<std::string>(json, "symbol");
    }
  };

  static SearchComparator search_comparator_;
};

template <>
struct Symbols::MemberMap<Symbols::EXCHANGE>
{
  using type = std::string;
  static constexpr const char* const kName = "exchange";
};

template <>
struct Symbols::MemberMap<Symbols::NAME>
{
  using type = std::string;
  static constexpr const char* const kName = "name";
};

template <>
struct Symbols::MemberMap<Symbols::DATE>
{
  using type = std::string;
  static constexpr const char* const kName = "date";
};

template <>
struct Symbols::MemberMap<Symbols::TYPE>
{
  using type = std::string;
  static constexpr const char* const kName = "type";
};

template <>
struct Symbols::MemberMap<Symbols::IEX_ID>
{
  using type = std::string;
  static constexpr const char* const kName = "iexId";
};

template <>
struct Symbols::MemberMap<Symbols::REGION>
{
  using type = std::string;
  static constexpr const char* const kName = "region";
};

template <>
struct Symbols::MemberMap<Symbols::CURRENCY>
{
  using type = std::string;
  static constexpr const char* const kName = "currency";
};

template <>
struct Symbols::MemberMap<Symbols::IS_ENABLED>
{
  using type = bool;
  static constexpr const char* const kName = "isEnabled";
};

template <>
struct Symbols::MemberMap<Symbols::FIGI>
{
  using type = std::string;
  static constexpr const char* const kName = "figi";
};

template <>
struct Symbols::MemberMap<Symbols::CIK>
{
  using type = std::string;
  static constexpr const char* const kName = "cik";
};

}  // namespace iex

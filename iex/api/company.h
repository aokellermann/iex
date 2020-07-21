/**
 * @file company.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <string>

#include "iex/detail/json_serializer.h"
#include "iex/iex.h"

namespace iex
{
/**
 * @see https://iexcloud.io/docs/api/#company
 *
 * Data format as of 6/18/20 during trading hours:
 *   {
 *     "symbol": "AAPL",            -- won't be used
 *     "companyName": "Apple, Inc.",
 *     "exchange": "NASDAQ",
 *     "industry": "Telecommunications Equipment",
 *     "website": "http://www.apple.com",
 *     "description": "Apple, Inc. engages in the design, manufacture, and sale of smartphones, personal computers,
 * tablets, wearables and accessories, and other variety of related services. It operates through the following
 * geographical segments: Americas, Europe, Greater China, Japan, and Rest of Asia Pacific. The Americas segment
 * includes North and South America. The Europe segment consists of European countries, as well as India, the Middle
 * East, and Africa. The Greater China segment comprises of China, Hong Kong, and Taiwan. The Rest of Asia Pacific
 * segment includes Australia and Asian countries. Its products and services include iPhone, Mac, iPad, AirPods, Apple
 * TV, Apple Watch, Beats products, Apple Care, iCloud, digital content stores, streaming, and licensing services. The
 * company was founded by Steven Paul Jobs, Ronald Gerald Wayne, and Stephen G. Wozniak on April 1, 1976 and is
 * headquartered in Cupertino, CA.", "CEO": "Timothy Donald Cook", "securityName": "Apple Inc.", "issueType": "cs",
 *     "sector": "Electronic Technology",
 *     "primarySicCode": 3663,
 *     "employees": 137000,
 *     "tags": [
 *       "Electronic Technology",
 *       "Telecommunications Equipment"
 *     ],
 *     "address": "One Apple Park Way",
 *     "address2": null,
 *     "state": "CA",
 *     "city": "Cupertino",
 *     "zip": "95014-2083",
 *     "country": "US",
 *     "phone": "1.408.996.1010"
 *   }
 */
class Company : public SymbolEndpoint
{
 public:
  enum MemberType
  {
    ENUM_FIRST,
    /**
     * Name of the company
     */
    COMPANY_NAME = ENUM_FIRST,
    EXCHANGE,
    INDUSTRY,
    WEBSITE,
    DESCRIPTION,
    CEO,
    /**
     * Name of the security
     */
    SECURITY_NAME,
    /**
     * refers to the common issue type of the stock.
     * ad – American Depository Receipt (ADR’s)
     * re – Real Estate Investment Trust (REIT’s)
     * ce – Closed end fund (Stock and Bond Fund)
     * si – Secondary Issue
     * lp – Limited Partnerships
     * cs – Common Stock
     * et – Exchange Traded Fund (ETF)
     * wt – Warrant
     * rt – Right
     * (blank) – Not Available, i.e., Note, or (non-filing) Closed Ended Funds
     * ut - Unit
     * temp - Temporary
     */
    ISSUE_TYPE,
    SECTOR,
    /**
     * Primary SIC Code for the symbol (if available)
     */
    PRIMARY_SIC_CODE,
    /**
     * Number of employees
     */
    EMPLOYEES,
    /**
     * An array of strings used to classify the company.
     */
    TAGS,
    /**
     * Street address of the company if available
     */
    ADDRESS,
    /**
     * Street address of the company if available
     */
    ADDRESS_2,
    /**
     * State of the company if available
     */
    STATE,
    /**
     * City of the company if available
     */
    CITY,
    /**
     * Zip code of the company if available
     */
    ZIP,
    /**
     * Country of the company if available
     */
    COUNTRY,
    /**
     * Phone number of the company if available
     */
    PHONE,
    ENUM_LAST,
  };

 private:
  template <Company::MemberType>
  struct MemberMap;

  template <Company::MemberType T>
  using MemberTypename = typename MemberMap<T>::type;

 public:
  explicit Company(json::JsonStorage data = json::JsonStorage{}, Symbol sym = {})
      : SymbolEndpoint(std::move(sym), std::move(data))
  {
  }

  template <MemberType T>
  json::Member<MemberTypename<T>> Get() const noexcept
  {
    return data_.SafeGetMember<MemberTypename<T>>(MemberMap<T>::kName);
  }

  ~Company() override = default;
};

template <>
struct Company::MemberMap<Company::COMPANY_NAME>
{
  using type = std::string;
  static constexpr json::MemberName kName = "companyName";
};

template <>
struct Company::MemberMap<Company::EXCHANGE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "exchange";
};

template <>
struct Company::MemberMap<Company::INDUSTRY>
{
  using type = std::string;
  static constexpr json::MemberName kName = "industry";
};

template <>
struct Company::MemberMap<Company::WEBSITE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "website";
};

template <>
struct Company::MemberMap<Company::DESCRIPTION>
{
  using type = std::string;
  static constexpr json::MemberName kName = "description";
};

template <>
struct Company::MemberMap<Company::CEO>
{
  using type = std::string;
  static constexpr json::MemberName kName = "CEO";
};

template <>
struct Company::MemberMap<Company::SECURITY_NAME>
{
  using type = std::string;
  static constexpr json::MemberName kName = "securityName";
};

template <>
struct Company::MemberMap<Company::ISSUE_TYPE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "issueType";
};

template <>
struct Company::MemberMap<Company::SECTOR>
{
  using type = std::string;
  static constexpr json::MemberName kName = "sector";
};

template <>
struct Company::MemberMap<Company::PRIMARY_SIC_CODE>
{
  using type = uint64_t;
  static constexpr json::MemberName kName = "primarySicCode";
};

template <>
struct Company::MemberMap<Company::EMPLOYEES>
{
  using type = uint64_t;
  static constexpr json::MemberName kName = "employees";
};

template <>
struct Company::MemberMap<Company::TAGS>
{
  using type = std::vector<std::string>;
  static constexpr json::MemberName kName = "tags";
};

template <>
struct Company::MemberMap<Company::ADDRESS>
{
  using type = std::string;
  static constexpr json::MemberName kName = "address";
};

template <>
struct Company::MemberMap<Company::ADDRESS_2>
{
  using type = std::string;
  static constexpr json::MemberName kName = "address2";
};

template <>
struct Company::MemberMap<Company::STATE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "state";
};

template <>
struct Company::MemberMap<Company::CITY>
{
  using type = std::string;
  static constexpr json::MemberName kName = "city";
};

template <>
struct Company::MemberMap<Company::ZIP>
{
  using type = std::string;
  static constexpr json::MemberName kName = "zip";
};

template <>
struct Company::MemberMap<Company::COUNTRY>
{
  using type = std::string;
  static constexpr json::MemberName kName = "country";
};

template <>
struct Company::MemberMap<Company::PHONE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "phone";
};
}  // namespace iex
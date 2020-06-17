/**
 * @file quote.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <string>

#include "iex/api.h"
#include "iex/json_serializer.h"

namespace iex::api
{
/**
 * @see https://iexcloud.io/docs/api/#api-system-metadata
 *
 * Data format as of 6/11/20 during trading hours:
 * {
 *   "symbol": "TSLA",                              -- won't be used
      "companyName": "Tesla, Inc.",
      "primaryExchange": "NASDAQ",
      "calculationPrice": "tops",
      "open": null,
      "openTime": null,
      "openSource": "official",
      "close": null,
      "closeTime": null,
      "closeSource": "official",
      "high": null,
      "highTime": 1591885447088,
      "highSource": "15 minute delayed price",
      "low": null,
      "lowTime": 1591882984133,
      "lowSource": "15 minute delayed price",
      "latestPrice": 1011.15,
      "latestSource": "IEX real time price",
      "latestTime": "10:38:58 AM",                  -- won't be used
      "latestUpdate": 1591886338439,
      "latestVolume": null,
      "iexRealtimePrice": 1011.15,
      "iexRealtimeSize": 20,
      "iexLastUpdated": 1591886338439,
      "delayedPrice": null,
      "delayedPriceTime": null,
      "oddLotDelayedPrice": null,
      "oddLotDelayedPriceTime": null,
      "extendedPrice": null,
      "extendedChange": null,
      "extendedChangePercent": null,
      "extendedPriceTime": null,
      "previousClose": 1025.05,
      "previousVolume": 18563413,
      "change": -13.9,
      "changePercent": -0.01356,
      "volume": null,
      "iexMarketPercent": 0.012791345623760948,
      "iexVolume": 68819,
      "avgTotalVolume": 13832059,
      "iexBidPrice": 866,
      "iexBidSize": 100,
      "iexAskPrice": 1018.5,
      "iexAskSize": 100,
      "iexOpen": null,
      "iexOpenTime": null,
      "iexClose": 1011.15,
      "iexCloseTime": 1591886338439,
      "marketCap": 187544057400,
      "peRatio": -1201.18,
      "week52High": 1027.48,
      "week52Low": 207.51,
      "ytdChange": 1.368837,
      "lastTradeTime": 1591886338439,
      "isUSMarketOpen": true
 * }
 */
class Quote : public SymbolEndpoint
{
 public:
  enum MemberType
  {
    COMPANY_NAME,
    PRIMARY_EXCHANGE,
    CALCULATION_PRICE,
    OPEN_PRICE,
    OPEN_TIME,
    OPEN_SOURCE,
    CLOSE_PRICE,
    CLOSE_TIME,
    CLOSE_SOURCE,
    HIGH_PRICE,
    HIGH_TIME,
    HIGH_SOURCE,
    LOW_PRICE,
    LOW_TIME,
    LOW_SOURCE,
    LATEST_PRICE,
    LATEST_UPDATE,
    LATEST_SOURCE,
    LATEST_VOLUME,
    IEX_REALTIME_PRICE,
    IEX_REALTIME_SIZE,
    IEX_LAST_UPDATED,
    DELAYED_PRICE,
    DELAYED_TIME,
    ODD_LOT_DELAYED_PRICE,
    ODD_LOT_DELAYED_TIME,
    EXTENDED_PRICE,
    EXTENDED_TIME,
    EXTENDED_CHANGE,
    EXTENDED_CHANGE_PERCENT,
    PREVIOUS_CLOSE,
    PREVIOUS_VOLUME,
    CHANGE,
    CHANGE_PERCENT,
    VOLUME,
    IEX_MARKET_PERCENT,
    IEX_VOLUME,
    AVERAGE_TOTAL_VOLUME,
    IEX_BID_PRICE,
    IEX_BID_SIZE,
    IEX_ASK_PRICE,
    IEX_ASK_SIZE,
    IEX_OPEN_PRICE,
    IEX_OPEN_TIME,
    IEX_CLOSE_PRICE,
    IEX_CLOSE_TIME,
    MARKET_CAP,
    PE_RATIO,
    WEEK_52_HIGH,
    WEEK_52_LOW,
    YTD_CHANGE,
    LAST_TRADE_TIME,
    IS_US_MARKET_OPEN,
  };

 private:
  template <Quote::MemberType>
  struct MemberMap;

  template <Quote::MemberType T>
  using MemberTypename = typename MemberMap<T>::type;

 public:
  explicit Quote(json::JsonStorage data = json::JsonStorage{}, Symbol sym = {})
      : SymbolEndpoint(std::move(sym), "quote", std::move(data))
  {
  }

  template <MemberType T>
  json::Member<MemberTypename<T>> Get() const noexcept
  {
    return data_.SafeGetMember<MemberTypename<T>>(MemberMap<T>::kName);
  }

  ~Quote() override = default;

  /**
   * All percentage values will be multiplied by a factor of 100
   */
  struct DisplayPercentOption : Option<bool>
  {
    DisplayPercentOption() : Option<bool>("displayPercent", true) {}
  };
};

template <>
struct Quote::MemberMap<Quote::COMPANY_NAME>
{
  using type = std::string;
  static constexpr json::MemberName kName = "companyName";
};

template <>
struct Quote::MemberMap<Quote::PRIMARY_EXCHANGE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "primaryExchange";
};

template <>
struct Quote::MemberMap<Quote::CALCULATION_PRICE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "calculationPrice";
};

template <>
struct Quote::MemberMap<Quote::OPEN_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "open";
};

template <>
struct Quote::MemberMap<Quote::OPEN_TIME>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "openTime";
};

template <>
struct Quote::MemberMap<Quote::OPEN_SOURCE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "openSource";
};

template <>
struct Quote::MemberMap<Quote::CLOSE_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "close";
};

template <>
struct Quote::MemberMap<Quote::CLOSE_TIME>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "closeTime";
};

template <>
struct Quote::MemberMap<Quote::CLOSE_SOURCE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "closeSource";
};

template <>
struct Quote::MemberMap<Quote::HIGH_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "high";
};

template <>
struct Quote::MemberMap<Quote::HIGH_TIME>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "highTime";
};

template <>
struct Quote::MemberMap<Quote::HIGH_SOURCE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "highSource";
};

template <>
struct Quote::MemberMap<Quote::LOW_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "low";
};

template <>
struct Quote::MemberMap<Quote::LOW_TIME>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "lowTime";
};

template <>
struct Quote::MemberMap<Quote::LOW_SOURCE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "lowSource";
};

template <>
struct Quote::MemberMap<Quote::LATEST_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "latestPrice";
};

template <>
struct Quote::MemberMap<Quote::LATEST_UPDATE>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "latestUpdate";
};

template <>
struct Quote::MemberMap<Quote::LATEST_SOURCE>
{
  using type = std::string;
  static constexpr json::MemberName kName = "latestSource";
};

template <>
struct Quote::MemberMap<Quote::LATEST_VOLUME>
{
  using type = Volume;
  static constexpr json::MemberName kName = "latestVolume";
};

template <>
struct Quote::MemberMap<Quote::IEX_REALTIME_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "iexRealtimePrice";
};

template <>
struct Quote::MemberMap<Quote::IEX_REALTIME_SIZE>
{
  using type = Volume;
  static constexpr json::MemberName kName = "iexRealtimeSize";
};

template <>
struct Quote::MemberMap<Quote::IEX_LAST_UPDATED>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "iexLastUpdated";
};

template <>
struct Quote::MemberMap<Quote::DELAYED_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "delayedPrice";
};

template <>
struct Quote::MemberMap<Quote::DELAYED_TIME>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "delayedPriceTime";
};

template <>
struct Quote::MemberMap<Quote::ODD_LOT_DELAYED_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "oddLotDelayedPrice";
};

template <>
struct Quote::MemberMap<Quote::ODD_LOT_DELAYED_TIME>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "oddLotDelayedPriceTime";
};

template <>
struct Quote::MemberMap<Quote::EXTENDED_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "extendedPrice";
};

template <>
struct Quote::MemberMap<Quote::EXTENDED_TIME>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "extendedPriceTime";
};

template <>
struct Quote::MemberMap<Quote::EXTENDED_CHANGE>
{
  using type = Price;
  static constexpr json::MemberName kName = "extendedChange";
};

template <>
struct Quote::MemberMap<Quote::EXTENDED_CHANGE_PERCENT>
{
  using type = Percent;
  static constexpr json::MemberName kName = "extendedChangePercent";
};

template <>
struct Quote::MemberMap<Quote::PREVIOUS_CLOSE>
{
  using type = Price;
  static constexpr json::MemberName kName = "previousClose";
};

template <>
struct Quote::MemberMap<Quote::PREVIOUS_VOLUME>
{
  using type = Volume;
  static constexpr json::MemberName kName = "previousVolume";
};

template <>
struct Quote::MemberMap<Quote::CHANGE>
{
  using type = Price;
  static constexpr json::MemberName kName = "change";
};

template <>
struct Quote::MemberMap<Quote::CHANGE_PERCENT>
{
  using type = Percent;
  static constexpr json::MemberName kName = "changePercent";
};

template <>
struct Quote::MemberMap<Quote::VOLUME>
{
  using type = Volume;
  static constexpr json::MemberName kName = "volume";
};

template <>
struct Quote::MemberMap<Quote::IEX_MARKET_PERCENT>
{
  using type = Percent;
  static constexpr json::MemberName kName = "iexMarketPercent";
};

template <>
struct Quote::MemberMap<Quote::IEX_VOLUME>
{
  using type = Volume;
  static constexpr json::MemberName kName = "iexVolume";
};

template <>
struct Quote::MemberMap<Quote::AVERAGE_TOTAL_VOLUME>
{
  using type = Volume;
  static constexpr json::MemberName kName = "avgTotalVolume";
};

template <>
struct Quote::MemberMap<Quote::IEX_BID_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "iexBidPrice";
};

template <>
struct Quote::MemberMap<Quote::IEX_BID_SIZE>
{
  using type = Volume;
  static constexpr json::MemberName kName = "iexBidSize";
};

template <>
struct Quote::MemberMap<Quote::IEX_ASK_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "iexAskPrice";
};

template <>
struct Quote::MemberMap<Quote::IEX_ASK_SIZE>
{
  using type = Volume;
  static constexpr json::MemberName kName = "iexAskSize";
};

template <>
struct Quote::MemberMap<Quote::IEX_OPEN_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "iexOpen";
};

template <>
struct Quote::MemberMap<Quote::IEX_OPEN_TIME>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "iexOpenTime";
};

template <>
struct Quote::MemberMap<Quote::IEX_CLOSE_PRICE>
{
  using type = Price;
  static constexpr json::MemberName kName = "iexClose";
};

template <>
struct Quote::MemberMap<Quote::IEX_CLOSE_TIME>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "iexCloseTime";
};

template <>
struct Quote::MemberMap<Quote::MARKET_CAP>
{
  using type = Price;
  static constexpr json::MemberName kName = "marketCap";
};

template <>
struct Quote::MemberMap<Quote::PE_RATIO>
{
  using type = double;
  static constexpr json::MemberName kName = "peRatio";
};

template <>
struct Quote::MemberMap<Quote::WEEK_52_HIGH>
{
  using type = Price;
  static constexpr json::MemberName kName = "week52High";
};

template <>
struct Quote::MemberMap<Quote::WEEK_52_LOW>
{
  using type = Price;
  static constexpr json::MemberName kName = "week52Low";
};

template <>
struct Quote::MemberMap<Quote::YTD_CHANGE>
{
  using type = Price;
  static constexpr json::MemberName kName = "ytdChange";
};

template <>
struct Quote::MemberMap<Quote::LAST_TRADE_TIME>
{
  using type = Timestamp;
  static constexpr json::MemberName kName = "lastTradeTime";
};

template <>
struct Quote::MemberMap<Quote::IS_US_MARKET_OPEN>
{
  using type = bool;
  static constexpr json::MemberName kName = "isUSMarketOpen";
};

}  // namespace iex::api

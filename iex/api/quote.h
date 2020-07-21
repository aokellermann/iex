/**
 * @file quote.h
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
 * @see https://iexcloud.io/docs/api/#quote
 * @
 * Data format as of 6/11/20 during trading hours:
 * {
 *   "symbol": "TSLA",                              -- won't be used
 *   "companyName": "Tesla, Inc.",
 *   "primaryExchange": "NASDAQ",
 *   "calculationPrice": "tops",
 *   "open": null,
 *   "openTime": null,
 *   "openSource": "official",
 *   "close": null,
 *   "closeTime": null,
 *   "closeSource": "official",
 *   "high": null,
 *   "highTime": 1591885447088,
 *   "highSource": "15 minute delayed price",
 *   "low": null,
 *   "lowTime": 1591882984133,
 *   "lowSource": "15 minute delayed price",
 *   "latestPrice": 1011.15,
 *   "latestSource": "IEX real time price",
 *   "latestTime": "10:38:58 AM",                  -- won't be used
 *   "latestUpdate": 1591886338439,
 *   "latestVolume": null,
 *   "iexRealtimePrice": 1011.15,
 *   "iexRealtimeSize": 20,
 *   "iexLastUpdated": 1591886338439,
 *   "delayedPrice": null,
 *   "delayedPriceTime": null,
 *   "oddLotDelayedPrice": null,
 *   "oddLotDelayedPriceTime": null,
 *   "extendedPrice": null,
 *   "extendedChange": null,
 *   "extendedChangePercent": null,
 *   "extendedPriceTime": null,
 *   "previousClose": 1025.05,
 *   "previousVolume": 18563413,
 *   "change": -13.9,
 *   "changePercent": -0.01356,
 *   "volume": null,
 *   "iexMarketPercent": 0.012791345623760948,
 *   "iexVolume": 68819,
 *   "avgTotalVolume": 13832059,
 *   "iexBidPrice": 866,
 *   "iexBidSize": 100,
 *   "iexAskPrice": 1018.5,
 *   "iexAskSize": 100,
 *   "iexOpen": null,
 *   "iexOpenTime": null,
 *   "iexClose": 1011.15,
 *   "iexCloseTime": 1591886338439,
 *   "marketCap": 187544057400,
 *   "peRatio": -1201.18,
 *   "week52High": 1027.48,
 *   "week52Low": 207.51,
 *   "ytdChange": 1.368837,
 *   "lastTradeTime": 1591886338439,
 *   "isUSMarketOpen": true
 * }
 */
class Quote : public SymbolEndpoint
{
 public:
  enum MemberType
  {
    ENUM_FIRST,
    /**
     * Refers to the company name.
     */
    COMPANY_NAME = ENUM_FIRST,
    /**
     * Refers to the primary listing exchange for the symbol.
     */
    PRIMARY_EXCHANGE,
    /**
     * @brief Refers to the source of the latest price.
     * Possible values are "tops", "sip", "previousclose" or "close".
     */
    CALCULATION_PRICE,
    /**
     * Refers to the official open price from the SIP. 15 minute delayed (can be null after 00:00 ET, before 9:45 and
     * weekends)
     */
    OPEN_PRICE,
    /**
     * Refers to the official listing exchange time for the open from the SIP. 15 minute delayed
     */
    OPEN_TIME,
    OPEN_SOURCE,
    /**
     * Refers to the official close price from the SIP. 15 minute delayed
     */
    CLOSE_PRICE,
    /**
     * Refers to the official listing exchange time for the close from the SIP. 15 minute delayed
     */
    CLOSE_TIME,
    CLOSE_SOURCE,
    /**
     * Refers to the market-wide highest price from the SIP. 15 minute delayed during normal market hours 9:30 - 16:00
     * (null before 9:45 and weekends).
     */
    HIGH_PRICE,
    HIGH_TIME,
    HIGH_SOURCE,
    /**
     * Refers to the market-wide lowest price from the SIP. 15 minute delayed during normal market hours 9:30 - 16:00
     * (null before 9:45 and weekends).
     */
    LOW_PRICE,
    LOW_TIME,
    LOW_SOURCE,
    /**
     * @brief Use this to get the latest price
     * Refers to the latest relevant price of the security which is derived from multiple sources. We first look for an
     * IEX real time price. If an IEX real time price is older than 15 minutes, 15 minute delayed market price is used.
     * If a 15 minute delayed price is not available, we will use the current day close price. If a current day close
     * price is not available, we will use the last available closing price (listed below as PREVIOUS_CLOSE) IEX real
     * time price represents trades on IEX only. Trades occur across over a dozen exchanges, so the last IEX price can
     * be used to indicate the overall market price. 15 minute delayed prices are from all markets using the
     * Consolidated Tape. This will not included pre or post market prices.
     */
    LATEST_PRICE,
    /**
     * Refers to the machine readable epoch timestamp of when LATEST_PRICE was last updated.
     */
    LATEST_UPDATE,
    /**
     * @brief This will represent a human readable description of the source of LATEST_PRICE.
     * Possible values are "IEX real time price", "15 minute delayed price", "Close" or "Previous close".
     */
    LATEST_SOURCE,
    /**
     * @brief Use this to get the latest volume
     * Refers to the latest total market volume of the stock across all markets. This will be the most recent volume of
     * the stock during trading hours, or it will be the total volume of the last available trading day.
     */
    LATEST_VOLUME,
    /**
     * Refers to the price of the last trade on IEX.
     */
    IEX_REALTIME_PRICE,
    /**
     * Refers to the size of the last trade on IEX.
     */
    IEX_REALTIME_SIZE,
    /**
     * Refers to the last update time of IEX_REALTIME_PRICE. If the value is null, IEX has not quoted the symbol in the
     * trading day.
     */
    IEX_LAST_UPDATED,
    /**
     * Refers to the 15 minute delayed market price from the SIP during normal market hours 9:30 - 16:00 ET.
     */
    DELAYED_PRICE,
    /**
     * Refers to the last update time of the delayed market price during normal market hours 9:30 - 16:00 ET.
     */
    DELAYED_TIME,
    /**
     * Refers to the 15 minute delayed odd Lot trade price from the SIP during normal market hours 9:30 - 16:00 ET.
     */
    ODD_LOT_DELAYED_PRICE,
    /**
     * Refers to the last update time of the odd Lot trade price during normal market hours 9:30 - 16:00 ET.
     */
    ODD_LOT_DELAYED_TIME,
    /**
     * Refers to the 15 minute delayed price outside normal market hours 0400 - 0930 ET and 1600 - 2000 ET. This
     * provides pre market and post market price. This is purposefully separate from latestPrice so users can display
     * the two prices separately.
     */
    EXTENDED_PRICE,
    /**
     * Refers to the last update time of EXTENDED_PRICE
     */
    EXTENDED_TIME,
    /**
     * Refers to the price change between extendedPrice and LATEST_PRICE.
     */
    EXTENDED_CHANGE,
    /**
     * Refers to the price change percent between EXTENDED_PRICE and LATEST_PRICE.
     */
    EXTENDED_CHANGE_PERCENT,
    /**
     * Refers to the previous trading day closing price.
     */
    PREVIOUS_CLOSE,
    /**
     * Refers to the previous trading day volume.
     */
    PREVIOUS_VOLUME,
    /**
     * Refers to the change in price between LATEST_PRICE and PREVIOUS_CLOSE
     */
    CHANGE,
    /**
     * Refers to the percent change in price between latestPrice and previousClose. For example, a 5% change would be
     * represented as 0.05.
     */
    CHANGE_PERCENT,
    /**
     * Total volume for the stock, but only updated after market open. To get premarket volume, use latestVolume.
     */
    VOLUME,
    /**
     * Refers to IEX’s percentage of the market in the stock.
     */
    IEX_MARKET_PERCENT,
    /**
     * Refers to shares traded in the stock on IEX.
     */
    IEX_VOLUME,
    /**
     * Refers to the 30 day average volume.
     */
    AVERAGE_TOTAL_VOLUME,
    /**
     * Refers to the best bid price on IEX.
     */
    IEX_BID_PRICE,
    /**
     * Refers to amount of shares on the bid on IEX.
     */
    IEX_BID_SIZE,
    /**
     * Refers to the best ask price on IEX.
     */
    IEX_ASK_PRICE,
    /**
     * Refers to amount of shares on the ask on IEX.
     */
    IEX_ASK_SIZE,
    IEX_OPEN_PRICE,
    IEX_OPEN_TIME,
    IEX_CLOSE_PRICE,
    IEX_CLOSE_TIME,
    /**
     * is calculated in real time using LATEST_PRICE.
     */
    MARKET_CAP,
    /**
     * Refers to the price-to-earnings ratio for the company.
     */
    PE_RATIO,
    /**
     * Refers to the adjusted 52 week high.
     */
    WEEK_52_HIGH,
    /**
     * Refers to the adjusted 52 week low.
     */
    WEEK_52_LOW,
    /**
     * Refers to the price change percentage from start of year to previous close.
     */
    YTD_CHANGE,
    /**
     * The last market hours trade excluding the closing auction trade.
     */
    LAST_TRADE_TIME,
    /**
     * For US stocks, indicates if the market is in normal market hours. Will be false during extended hours trading.
     */
    IS_US_MARKET_OPEN,
    ENUM_LAST,
  };

 private:
  template <Quote::MemberType>
  struct MemberMap;

  template <Quote::MemberType T>
  using MemberTypename = typename MemberMap<T>::type;

 public:
  explicit Quote(json::JsonStorage data = json::JsonStorage{}, Symbol sym = {})
      : SymbolEndpoint(std::move(sym), std::move(data))
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

}  // namespace iex

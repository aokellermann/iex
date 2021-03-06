/**
 * @file system_status.h
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
 * @see https://iexcloud.io/docs/api/#api-system-metadata
 *
 * Data format as of 6/8/20:
 * {
 *  "status": "up",
 *  "version": "1.32",
 *  "time": 1591638010429,
 *  "currentMonthAPICalls": 4250290506
 * }
 */
class SystemStatus : public Endpoint
{
 public:
  enum MemberType
  {
    ENUM_FIRST,
    STATUS = ENUM_FIRST,
    VERSION,
    TIMESTAMP,
    CURRENT_MONTH_API_CALLS,
    ENUM_LAST,
  };

 private:
  template <SystemStatus::MemberType>
  struct MemberMap;

  template <SystemStatus::MemberType T>
  using MemberTypename = typename MemberMap<T>::type;

 public:
  explicit SystemStatus(json::JsonStorage data = json::JsonStorage{}) : Endpoint(std::move(data)) {}

  template <MemberType T>
  json::Member<MemberTypename<T>> Get() const noexcept
  {
    return data_.SafeGetMember<MemberTypename<T>>(MemberMap<T>::kName);
  }

  ~SystemStatus() override = default;
};

template <>
struct SystemStatus::MemberMap<SystemStatus::STATUS>
{
  using type = std::string;
  static constexpr const char* const kName = "status";
};

template <>
struct SystemStatus::MemberMap<SystemStatus::VERSION>
{
  using type = std::string;
  static constexpr const char* const kName = "version";
};

template <>
struct SystemStatus::MemberMap<SystemStatus::TIMESTAMP>
{
  using type = Timestamp;
  static constexpr const char* const kName = "time";
};

template <>
struct SystemStatus::MemberMap<SystemStatus::CURRENT_MONTH_API_CALLS>
{
  using type = uint64_t;
  static constexpr const char* const kName = "currentMonthAPICalls";
};

}  // namespace iex

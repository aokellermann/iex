/**
 * @file system_status.h
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
  SystemStatus() : Endpoint("status") {}

  ~SystemStatus() override = default;

  explicit SystemStatus(const Json& input_json) : SystemStatus() { Deserialize(input_json); }

  ErrorCode Deserialize(const Json& input_json) final;

  std::string status_;
  std::string version_;
  Timestamp timestamp_;
  uint64_t current_month_api_calls_;
};

}  // namespace iex::api

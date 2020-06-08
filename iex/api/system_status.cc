/**
 * @file system_status.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex/api/system_status.h"

namespace iex::api
{
ErrorCode SystemStatus::Deserialize(const Json& input_json)
{
  try
  {
    input_json.at("status").get_to(status_);
    input_json.at("version").get_to(version_);
    timestamp_ = Timestamp(input_json.at("time").get<uint64_t>());
    input_json.at("currentMonthAPICalls").get_to(current_month_api_calls_);
    return {};
  }
  catch (const std::exception& e)
  {
    return ErrorCode("SystemStatus::Deserialize() failed", {"exception", ErrorCode(e.what())});
  }
}
}  // namespace iex::api

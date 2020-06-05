/**
 * @file system_status.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex/api/system_status.h"

namespace iex::api::system_status
{
ErrorCode SystemStatus::Deserialize(const Json& input_json)
{
  try
  {
    input_json.at("status").get_to(status_);
    input_json.at("version").get_to(version_);
    timestamp_ = Timestamp(input_json, "time");
  }
  catch (const std::exception& e)
  {
    return ErrorCode("SystemStatus::Deserialize() failed", {"exception", ErrorCode(e.what())});
  }

  return {};
}

NamedPair<std::string> SystemStatus::GetNamedOptions(const Endpoint::OptionSet& /*options*/) const { return {}; }

}  // namespace iex::api::system_status

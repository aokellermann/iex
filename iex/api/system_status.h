/**
 * @file system_status.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <chrono>
#include <string>

#include "iex/api.h"
#include "iex/json_serializer.h"

namespace iex::api::system_status
{
class SystemStatus : public Endpoint
{
 public:
  SystemStatus() = default;

  ErrorCode Deserialize(const Json& input_json) override;

  [[nodiscard]] inline std::string GetName() const noexcept override { return std::string("status"); }

  [[nodiscard]] NamedPair<std::string> GetNamedOptions(const OptionSet& options) const override;

  std::string status_;
  std::string version_;
  Timestamp timestamp_;
};

}  // namespace iex::api::system_status

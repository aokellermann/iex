/**
 * @file api.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "api.h"

#include <unordered_map>

#include "iex/api/system_status.h"
#include "iex/curl_wrapper.h"

namespace iex::api
{
namespace
{
const EndpointPtr<> kEndpointPtrs[] = {
  std::make_shared<system_status::SystemStatus>()
};

const std::string kVersionUrlMap[] = {
    "stable",
    "latest",
    "v1",
    "beta",
};

const std::string kBaseUrl = "https://cloud.iexapis.com/";

std::string GetBaseUrl(const Version version) { return kBaseUrl + kVersionUrlMap[version] + '/'; }

curl::Url GetUrl(const Request& request)
{
  // Endpoint endpoint = request.type

  std::string base_url = kBaseUrl + kVersionUrlMap[request.options.version] + '/';
}

curl::UrlSet GetUrls(const AggregatedRequests& requests)
{
  // In the future, this function may call another file/class to optimize calls.
  // For now, it performs no optimization.

  // Only stock endpoint may be batch called according to documentation. https://iexcloud.io/docs/api/#batch-requests

  curl::UrlSet set;

  // First, create non-symbol-related Urls.
  for (const auto& [type, opts] : requests.requests)
  {
    // Endpoint endpoint
    // set.insert(curl::Url())
  }
}

}  // namespace

// region Endpoint::Map Specializations

template <>
struct Endpoint::Map<Endpoint::Type::SYSTEM_STATUS>
{
  using type = system_status::SystemStatus;
};

// endregion Endpoint::Map Specializations

// region Interface

ValueWithErrorCode<AggregatedResponses> Get(const AggregatedRequests& requests) {}

// endregion Interface

}  // namespace iex::api

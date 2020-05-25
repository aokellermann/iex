/**
 * @file iex.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex/iex.h"

#include "iex/curl_wrapper.h"

namespace iex
{
ErrorCode Init()
{
  const auto ec = curl::Init();
  if (ec.Failure())
  {
    return ErrorCode("iex::Init failed", ec);
  }
  return ErrorCode();
}
}  // namespace iex

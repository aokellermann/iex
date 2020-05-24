/**
 * Copyright 2020 Antony Kellermann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <shared_mutex>  // NOLINT
#include <string>

#include "iex/iex.h"

namespace iex::env
{
namespace
{
std::shared_mutex mutex;
}

ValueWithErrorCode<std::string> GetEnv(const std::string& name)
{
  if (name.empty())
  {
    return {{}, ErrorCode("Environment variable name may not be empty", {"name", ErrorCode(name)})};
  }

  if (name.find('=') != std::string::npos)
  {
    return {{}, ErrorCode("Environment variable name may not contain '=' character", {"name", ErrorCode(name)})};
  }

  std::shared_lock lock(mutex);
  const char* env_str = std::getenv(name.c_str());
  if (env_str == nullptr)
  {
    return {{}, ErrorCode("Failed to get environment variable", {"name", ErrorCode(name)})};
  }

  return {std::string(env_str), {}};
}

ErrorCode SetEnv(const std::string& name, const std::string& value)
{
  if (name.empty())
  {
    return {{},
            ErrorCode("Environment variable name may not be empty",
                      {{"name", ErrorCode(name)}, {"value", ErrorCode(value)}})};
  }

  if (value.empty())
  {
    return {{},
            ErrorCode("Environment variable value may not be empty",
                      {{"name", ErrorCode(name)}, {"value", ErrorCode(value)}})};
  }

  if (name.find('=') != std::string::npos)
  {
    return {{},
            ErrorCode("Environment variable name may not contain '=' character",
                      {{"name", ErrorCode(name)}, {"value", ErrorCode(value)}})};
  }

  if (value.find('=') != std::string::npos)
  {
    return {{},
            ErrorCode("Environment variable value may not contain '=' character",
                      {{"name", ErrorCode(name)}, {"value", ErrorCode(value)}})};
  }

  std::unique_lock lock(mutex);
  int success = setenv(name.c_str(), value.c_str(), 1) == 0;
  if (!success)
  {
    return {{},
            ErrorCode(
                "Failed to get environment variable",
                {{"name", ErrorCode(name)}, {"value", ErrorCode(value)}, {"error", ErrorCode(std::strerror(errno))}})};
  }

  return {};
}

ErrorCode UnsetEnv(const std::string& name)
{
  if (name.empty())
  {
    return {{}, ErrorCode("Environment variable name may not be empty", {"name", ErrorCode(name)})};
  }

  if (name.find('=') != std::string::npos)
  {
    return {{}, ErrorCode("Environment variable name may not contain '=' character", {"name", ErrorCode(name)})};
  }

  std::unique_lock lock(mutex);
  int success = unsetenv(name.c_str()) == 0;
  if (!success)
  {
    return {{},
            ErrorCode("Failed to unset environment variable",
                      {{"name", ErrorCode(name)}, {"error", ErrorCode(std::strerror(errno))}})};
  }

  return {};
}
}  // namespace iex::env

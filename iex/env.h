/**
 * @file env.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <shared_mutex>  // NOLINT
#include <string>

#include "iex/iex.h"

/**
 * Contains synchronized methods for reading from and writing to environment variables.
 */
namespace iex::env
{
namespace
{
/**
 * This mutex is used to synchronize access to all environment variables.
 */
std::shared_mutex mutex;
}  // namespace

/**
 * Returns the value of the environment variable with the given name.
 * @param name the name of the environment variable
 * @return environment variable value if success and ErrorCode denoting success or failure
 */
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

  std::string env_str;

  {
    std::shared_lock lock(mutex);
    const char* env_c_str = std::getenv(name.c_str());
    if (env_c_str != nullptr)
    {
      env_str = env_c_str;
    }
  }

  if (env_str.empty())
  {
    return {{}, ErrorCode("Failed to get environment variable", {"name", ErrorCode(name)})};
  }

  return {std::string(env_str), {}};
}

/**
 * Sets the value of the environment variable name to value. If the environment variable doesn't exist, it will be
 * created. If it already exists, it will be modified.
 * @param name the name of the environment variable
 * @param value the value of the environment variable
 * @return ErrorCode denoting success or failure
 */
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

  int success;

  {
    std::unique_lock lock(mutex);
    success = setenv(name.c_str(), value.c_str(), 1) == 0;
  }

  if (!success)
  {
    return {{},
            ErrorCode(
                "Failed to get environment variable",
                {{"name", ErrorCode(name)}, {"value", ErrorCode(value)}, {"error", ErrorCode(std::strerror(errno))}})};
  }

  return {};
}

/**
 * Removes the environment variable with the given name.
 * @param name the name of the environment variable
 * @return ErrorCode denoting success or failure
 */
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

  int success;

  {
    std::unique_lock lock(mutex);
    success = unsetenv(name.c_str()) == 0;
  }

  if (!success)
  {
    return {{},
            ErrorCode("Failed to unset environment variable",
                      {{"name", ErrorCode(name)}, {"error", ErrorCode(std::strerror(errno))}})};
  }

  return {};
}
}  // namespace iex::env

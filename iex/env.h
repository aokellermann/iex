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

/**
 * Validates a given environment variable.
 *
 * An environment variable is valid if both the name and value are non-empty and the name and value do not contain any
 * '=' characters.
 * @param name the name of the environment variable
 * @param value the value of the environment variable, or nullptr if not relevant
 * @return ErrorCode denoting success or failure
 */
ErrorCode Validate(const std::string& name, const std::string* value)
{
  std::string message;

  if (name.empty())
  {
    message = "Environment variable name may not be empty";
  }
  else if (value != nullptr && value->empty())
  {
    message = "Environment variable value may not be empty";
  }
  else if (name.find('=') != std::string::npos)
  {
    message = "Environment variable name may not contain '=' character";
  }
  else if (value != nullptr && value->find('=') != std::string::npos)
  {
    message = "Environment variable value may not contain '=' character";
  }
  else
  {
    return {};
  }

  if (value == nullptr)
  {
    return ErrorCode(message, {"name", ErrorCode(name)});
  }
  else
  {
    return ErrorCode(message, {{"name", ErrorCode(name)}, {"value", ErrorCode(*value)}});
  }
}
}  // namespace

/**
 * Returns the value of the environment variable with the given name.
 * @param name the name of the environment variable
 * @return environment variable value if success and ErrorCode denoting success or failure
 */
ValueWithErrorCode<std::string> GetEnv(const std::string& name)
{
  const auto validate_ec = Validate(name, nullptr);
  if (validate_ec.Failure())
  {
    return {{}, validate_ec};
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
  const auto validate_ec = Validate(name, &value);
  if (validate_ec.Failure())
  {
    return {{}, validate_ec};
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
  const auto validate_ec = Validate(name, nullptr);
  if (validate_ec.Failure())
  {
    return {{}, validate_ec};
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

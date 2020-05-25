/**
 * @file env.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <string>

#include "iex/iex.h"

/**
 * Contains synchronized methods for reading from and writing to environment variables.
 */
namespace iex::env
{
/**
 * Returns the value of the environment variable with the given name.
 * @param name the name of the environment variable
 * @return environment variable value if success and ErrorCode denoting success or failure
 */
ValueWithErrorCode<std::string> GetEnv(const std::string& name);

/**
 * Sets the value of the environment variable name to value. If the environment variable doesn't exist, it will be
 * created. If it already exists, it will be modified.
 * @param name the name of the environment variable
 * @param value the value of the environment variable
 * @return ErrorCode denoting success or failure
 */
ErrorCode SetEnv(const std::string& name, const std::string& value);

/**
 * Removes the environment variable with the given name.
 * @param name the name of the environment variable
 * @return ErrorCode denoting success or failure
 */
ErrorCode UnsetEnv(const std::string& name);

}  // namespace iex::env

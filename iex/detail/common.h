/**
 * @file iex.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <chrono>
#include <initializer_list>
#include <string>
#include <utility>

namespace iex
{
// region Generic Types

/**
 * Represents a timestamp in milliseconds.
 */
using Timestamp = std::chrono::milliseconds;

// endregion Generic Types

// region named pair

/**
 * Represents a generic named pair.
 */
template <typename T>
using NamedPair = std::pair<std::string, T>;

// endregion named pair

// region error code

/**
 * This class represents an error code.
 */
class ErrorCode : public std::string
{
 public:
  // region constructors

  ErrorCode() = default;

  ErrorCode(const ErrorCode& ec) = default;

  explicit ErrorCode(const std::string& str) : std::string(str) {}

  ErrorCode(const std::string& message, const ErrorCode& inner_ec) : std::string(message + ": [" + inner_ec + ']') {}

  explicit ErrorCode(const NamedPair<ErrorCode>& named_ec) : ErrorCode(named_ec.first, named_ec.second) {}

  ErrorCode(const std::string& message, const std::initializer_list<NamedPair<ErrorCode>>& named_ec_list)
      : ErrorCode(message, named_ec_list.begin(), named_ec_list.end())
  {
  }

  template <class InputIt>
  ErrorCode(const std::string& message, InputIt named_ecs_begin, InputIt named_ecs_end) : std::string(message)
  {
    if (named_ecs_begin != named_ecs_end)
    {
      append(": [");

      for (auto iter = named_ecs_begin; iter != named_ecs_end; ++iter)
      {
        append(ErrorCode(*iter) + ", ");
      }
      pop_back();
      pop_back();
      append("]");
    }
  }

  // endregion constructors

  // region Success/Failure

  [[nodiscard]] inline bool Success() const noexcept { return empty(); }

  [[nodiscard]] inline bool Failure() const noexcept { return !Success(); }

  // endregion Success/Failure

  // region Helpers

  friend void PrintTo(const ErrorCode& ec, std::ostream* os) { *os << ec; }

  // endregion Helpers
};

template <typename T>
using ValueWithErrorCode = std::pair<T, ErrorCode>;

// endregion error code
}  // namespace iex
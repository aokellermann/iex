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

#include <initializer_list>
#include <string>
#include <utility>

namespace iex
{
// region named pair

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

  explicit ErrorCode(const char* str) : std::string(str) {}

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
};

template <typename T>
using ValueWithErrorCode = std::pair<T, ErrorCode>;

// endregion error code

// region Interface
/**
 * This function must be called once at program startup, before any other threads have been created.
 * @return ErrorCode
 */
ErrorCode Init();

// endregion Interface

}  // namespace iex

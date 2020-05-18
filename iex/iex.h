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
using named_pair = std::pair<std::string, T>;

// endregion named pair

// region error code

/**
 * This class represents an error code.
 */
class ec : public std::string
{
 public:
  // region constructors

  ec() = default;

  ec(const ec& ec) = default;

  ec(ec&& ec) = default;

  explicit ec(const char* str) : std::string(str) {}

  explicit ec(const std::string& str) : std::string(str) {}

  ec(const std::string& message, const ec& inner_ec) : std::string(message + ": [" + inner_ec + ']') {}

  explicit ec(const named_pair<ec>& named_ec) : ec(named_ec.first, named_ec.second) {}

  ec(const std::string& message, const std::initializer_list<named_pair<ec>>& named_ec_list) : std::string(message)
  {
    if (named_ec_list.size() != 0)
    {
      append(": [");

      for (const auto& named_ec : named_ec_list)
      {
        append(ec(named_ec) + ", ");
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

// endregion error code

ec Init();

}  // namespace iex

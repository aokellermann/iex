/**
 * @file iex.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <chrono>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>

#include "iex/detail/utils.h"

namespace iex
{
// region Generic Types

/**
 * Represents a timestamp in milliseconds.
 */
using Timestamp = std::chrono::milliseconds;

// endregion Generic Types

// region KeyValuePair

template <typename T>
struct KVP
{
 public:
  using KeyType = std::string;
  using ValueType = T;

  struct KeyHasher
  {
    std::size_t operator()(const KVP<T>& k) const noexcept { return std::hash<KeyType>()(k.key); }
  };

  struct KeyCompare
  {
    bool operator()(const KVP<T>& a, const KVP<T>& b) const noexcept { return a.key < b.key; }
  };

  KVP() = default;
  KVP(KeyType k, ValueType v) : key(std::move(k)), value(std::move(v)) {}
  virtual ~KVP() = default;

  bool operator==(const KVP<ValueType>& other) const { return key == other.key && value == other.value; }

  KeyType key;
  ValueType value;
};

// endregion KeyValuePair

// region ErrorCode

/**
 * Base class for exception types
 */
class ErrorCode : public std::exception
{
 public:
  static const ErrorCode kSuccess;

  ErrorCode() noexcept = default;
  ErrorCode(const ErrorCode&) = default;
  ErrorCode& operator=(const ErrorCode&) = default;
  ErrorCode(ErrorCode&&) = default;
  ErrorCode& operator=(ErrorCode&&) = default;

  explicit ErrorCode(std::string message) : what_(std::move(message)) {}
  ErrorCode(std::string message, std::string inner) : ErrorCode(std::move(message) + ": [" + std::move(inner) + ']') {}

  template <class InputIt>
  ErrorCode(const std::string& message, InputIt ecs_begin, InputIt ecs_end)
      : ErrorCode(std::move(message) + utils::Join<ErrorCode>(ecs_begin, ecs_end, ", "))
  {
  }

  ~ErrorCode() override = default;

  operator const std::string&() const noexcept { return what_; }  // NOLINT
  [[nodiscard]] const char* what() const noexcept override { return what_.c_str(); }

  [[nodiscard]] bool Success() const noexcept { return what_.empty(); }
  [[nodiscard]] bool Failure() const noexcept { return !Success(); }

  /// For Gtest printing
  friend void PrintTo(const ErrorCode& ec, std::ostream* os) { *os << std::string(ec); }

 private:
  std::string what_;
};

inline const ErrorCode ErrorCode::kSuccess;

// endregion ErrorCode
}  // namespace iex

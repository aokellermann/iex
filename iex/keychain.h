/**
 * @file api_key.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <string>

#include "iex/file_serializable.h"
#include "iex/json_serializer.h"

namespace iex::api::key
{
class Keychain : private file::FileIoBase, private json::JsonBidirectionalSerializable
{
 public:
  using Key = std::string;

  /**
   * @see https://iexcloud.io/docs/api/#authentication
   */
  enum KeyType
  {
    PUBLIC,
    SECRET,
    SANDBOX_PUBLIC,
    SANDBOX_SECRET,
    NUM_KEYS
  };

  struct EnvironmentFlag
  {
  };

  explicit Keychain(const EnvironmentFlag& environment_flag);

  explicit Keychain(file::Directory directory = file::Directory::HOME);

  ErrorCode Set(KeyType type, const Key& key, bool write = true);

  [[nodiscard]] ValueWithErrorCode<Key> Get(KeyType type) const;

  [[nodiscard]] bool Populated() const noexcept;

  [[nodiscard]] const ErrorCode& KeychainValidity() const noexcept { return ec_; }

 private:
  ValueWithErrorCode<json::Json> Serialize() final;

  ErrorCode Deserialize(const json::Json& input_json) final;

  enum KeyLocation
  {
    ENVIRONMENT,
    FILE
  };

  KeyLocation key_location_;
  std::string keys_[NUM_KEYS];
  ErrorCode ec_;
};
}  // namespace iex::api::key
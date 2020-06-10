/**
 * @file keychain.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <string>

#include "iex/file_serializable.h"
#include "iex/json_serializer.h"

/**
 * Contains methods and classes for reading and writing IEX API keys.
 */
namespace iex::api::key
{
/**
 * Container for managing IEX API keys.
 */
class Keychain : private file::FileIoBase, private json::JsonBidirectionalSerializable
{
 public:
  /**
   * Represents an API key
   */
  using Key = std::string;

  /**
   * Represents the type of API key
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

  /**
   * Passed to constructor to indicate that Keychain should read from and write to environment variables
   */
  struct EnvironmentFlag
  {
    EnvironmentFlag() = default;
  };

  /**
   * Instantiates Keychain such that all keys are read from and written to environment variables.
   * @note All API key changes made will be reset after the user logs out.
   * @param environment_flag
   */
  explicit Keychain(const EnvironmentFlag& environment_flag);

  /**
   * Instantiates Keychain such that all keys are read from and written to a file with the specified directory.
   * @param directory the location of the keychain file
   */
  explicit Keychain(file::Directory directory = file::Directory::HOME);

  /**
   * Stores or overwrites an API key.
   * @param type the type of key
   * @param key the key
   * @param write if true, will write to environment or file
   * @return ErrorCode indicating success or failure
   */
  ErrorCode Set(KeyType type, const Key& key, bool write = true);

  /**
   * Gets the specified key.
   * @param type the type of key to get
   * @return ErrorCode indicating success or failure along with the requested key if success
   */
  [[nodiscard]] ValueWithErrorCode<Key> Get(KeyType type) const;

  /**
   * Indicates whether all keys are populated and valid.
   * @return true if populated and valid, false otherwise
   */
  [[nodiscard]] bool Populated() const noexcept;

  /**
   * Indicates whether key initialization was successful or not. This should be called directly after construction.
   * @return ErrorCode indicating success or failure
   */
  [[nodiscard]] const ErrorCode& KeychainValidity() const noexcept { return ec_; }

 private:
  ValueWithErrorCode<json::Json> Serialize() final;

  ErrorCode Deserialize(const json::Json& input_json) final;

  enum KeyLocation
  {
    ENVIRONMENT,
    FILE
  };

  /**
   * Where to read from and write keys
   */
  KeyLocation key_location_;

  /**
   * Key array
   */
  Key keys_[NUM_KEYS];

  /**
   * Stores construction error if any
   */
  ErrorCode ec_;
};
}  // namespace iex::api::key

/**
 * @file keychain.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex/keychain.h"

#include <cctype>
#include <utility>

#include "iex/env.h"

namespace iex::api::key
{
namespace
{
constexpr const char* kKeyNameMap[Keychain::NUM_KEYS]{"IEX_PUBLIC_KEY", "IEX_SECRET_KEY", "IEX_SANDBOX_PUBLIC_KEY",
                                                      "IEX_SANDBOX_SECRET_KEY"};

constexpr const std::size_t kKeySizesMap[Keychain::NUM_KEYS]{35, 35, 36, 36};

constexpr const char* kKeyPrefixesMap[Keychain::NUM_KEYS]{"pk_", "sk_", "Tpk_", "Tsk_"};

constexpr const std::size_t kPrefixSizesMap[Keychain::NUM_KEYS]{3, 3, 4, 4};

ErrorCode Validate(const Keychain::KeyType type, const Keychain::Key& key)
{
  const auto actual_size = key.size();
  const auto expected_size = kKeySizesMap[type];
  if (actual_size != expected_size)
  {
    return ErrorCode{"Invalid key length",
                     {{"actual", ErrorCode(std::to_string(actual_size))},
                      {"expected", ErrorCode(std::to_string(expected_size))},
                      {"type", ErrorCode(kKeyNameMap[type])},
                      {"key", ErrorCode(key)}}};
  }

  const auto actual_prefix = key.substr(0, kPrefixSizesMap[type]);
  const auto* const expected_prefix = kKeyPrefixesMap[type];
  if (actual_prefix != expected_prefix)
  {
    return ErrorCode{"Invalid key prefix",
                     {{"actual", ErrorCode(actual_prefix)},
                      {"expected", ErrorCode(expected_prefix)},
                      {"type", ErrorCode(kKeyNameMap[type])},
                      {"key", ErrorCode(key)}}};
  }

  for (auto c = key.begin() + kPrefixSizesMap[type]; c != key.end(); ++c)
  {
    if (!std::isxdigit(*c))
    {
      return ErrorCode{"Invalid key character",
                       {{"actual", ErrorCode({*c})},
                        {"expected", ErrorCode("element of [0123456789abcdefABCDEF]")},
                        {"type", ErrorCode(kKeyNameMap[type])},
                        {"key", ErrorCode(key)}}};
    }
  }

  return {};
}
}  // namespace

// region Interface

Keychain::Keychain(const EnvironmentFlag&)
    : file::FileIoBase("keychain", file::Directory::TEMP), key_location_(KeyLocation::ENVIRONMENT)
{
  for (int i = 0; i < NUM_KEYS; ++i)
  {
    const auto pair = env::GetEnv(kKeyNameMap[i]);
    if (pair.second.Failure())  // keys have not been set yet
    {
      return;
    }

    auto ec = Set(static_cast<KeyType>(i), pair.first);
    if (ec.Failure())
    {
      ec_ = {"Keychain::Keychain() failed", {std::move(ec)}};
      return;
    }
  }
}

Keychain::Keychain(file::Directory directory)
    : file::FileIoBase("keychain", directory), key_location_(KeyLocation::FILE)
{
  auto response = ReadFile();
  if (response.second.Failure())
  {
    ec_ = {"Keychain::Keychain() failed", std::move(response.second)};
  }
  else if (!response.first.empty())
  {
    ec_ = Deserialize(json::Json::parse(response.first));
  }
}

ErrorCode Keychain::Set(KeyType type, const Key& key, bool write)
{
  if (ec_.Failure())
  {
    return {"Keychain::Set() failed", ec_};
  }

  auto validity = Validate(type, key);
  if (validity.Failure())
  {
    return {"Keychain::Set() failed", std::move(validity)};
  }

  if (write)
  {
    if (key_location_ == ENVIRONMENT)
    {
      auto ec = env::SetEnv(kKeyNameMap[type], key);
      if (ec.Failure())
      {
        return {"Keychain::Set() failed", std::move(ec)};
      }
    }
    else if (key_location_ == FILE)
    {
      auto json = Serialize();
      if (json.second.Failure())
      {
        return {"Keychain::Set() failed", std::move(json.second)};
      }

      auto ec = WriteFile(json.first.dump());
      if (ec.Failure())
      {
        return {"Keychain::Set() failed", std::move(ec)};
      }
    }
  }

  keys_[type] = std::move(key);
  return {};
}

ValueWithErrorCode<Keychain::Key> Keychain::Get(Keychain::KeyType type) const
{
  if (ec_.Failure())
  {
    return {{}, {"Keychain::Get() failed", ec_}};
  }

  return {keys_[type], {}};
}

bool Keychain::Populated() const noexcept
{
  for (int i = 0; i < NUM_KEYS; ++i)
  {
    if (Validate(static_cast<KeyType>(i), keys_[i]).Failure())
    {
      return false;
    }
  }

  return true;
}

// endregion Interface

// region Serialization

ValueWithErrorCode<json::Json> Keychain::Serialize()
{
  if (ec_.Failure())
  {
    return ValueWithErrorCode<json::Json>{{}, ErrorCode{"Keychain::Serialize() failed", ec_}};
  }

  json::Json json;
  for (int i = 0; i < NUM_KEYS; ++i)
  {
    auto response = Get(static_cast<KeyType>(i));
    if (response.second.Failure())
    {
      return {"Keychain::Serialize() failed", std::move(response.second)};
    }
    json[kKeyNameMap[i]] = Get(static_cast<KeyType>(i)).first;
  }

  return {std::move(json), {}};
}

ErrorCode Keychain::Deserialize(const json::Json& input_json)
{
  if (input_json.is_null())
  {
    return {};
  }

  try
  {
    for (int i = 0; i < NUM_KEYS; ++i)
    {
      auto ec = Set(static_cast<KeyType>(i), input_json[std::string(kKeyNameMap[i])].get<std::string>(), false);
      if (ec.Failure())
      {
        return {"Keychain::Deserialize() failed", std::move(ec)};
      }
    }
    return {};
  }
  catch (const std::exception& e)
  {
    return {"Keychain::Deserialize() failed", ErrorCode{e.what()}};
  }
}

// endregion Serialization

}  // namespace iex::api::key

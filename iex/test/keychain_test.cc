/**
 * @file key.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "iex/file_serializable.h"
#include "iex/json_serializer.h"
#include "iex/keychain.h"

using Keychain = iex::api::key::Keychain;

TEST(Key, Environment)
{
  Keychain key(Keychain::EnvironmentFlag{});
  EXPECT_EQ(key.KeychainValidity(), "");
  ASSERT_TRUE(key.KeychainValidity().Success());

  static constexpr const char* kKeyNameArray[]{"IEX_PUBLIC_KEY", "IEX_SECRET_KEY", "IEX_SANDBOX_PUBLIC_KEY",
                                               "IEX_SANDBOX_SECRET_KEY"};

  for (int i = 0; i < Keychain::KeyType::NUM_KEYS; ++i)
  {
    auto response = key.Get(static_cast<Keychain::KeyType>(i));
    ASSERT_TRUE(response.second.Success());
    EXPECT_EQ(response.first, std::getenv(kKeyNameArray[i]));
  }
}

TEST(Key, File)
{
  static constexpr const char* kKeyNameArray[Keychain::NUM_KEYS]{"IEX_PUBLIC_KEY", "IEX_SECRET_KEY",
                                                                 "IEX_SANDBOX_PUBLIC_KEY", "IEX_SANDBOX_SECRET_KEY"};
  static constexpr const char* kDummyKeys[Keychain::NUM_KEYS]{
      "pk_483bb0e8c5dd4a2974d362dd8aad154d", "sk_12d3caa449bd4de4b9f063089c47f69b",
      "Tpk_fb19c49530a6f1e9158142010a80043c", "Tsk_d405c80f30a6f1e895814201aa80043f"};

  iex::json::Json json;
  for (int i = 0; i < Keychain::KeyType::NUM_KEYS; ++i)
  {
    json[kKeyNameArray[i]] = kDummyKeys[i];
  }

  std::filesystem::create_directory("/tmp/iex");
  std::ofstream ofstream("/tmp/iex/keychain.json");
  const auto str = json.dump();
  ofstream << str;
  ofstream.close();

  Keychain key(iex::file::TEMP);
  EXPECT_EQ(key.KeychainValidity(), "");
  ASSERT_TRUE(key.KeychainValidity().Success());

  for (int i = 0; i < Keychain::KeyType::NUM_KEYS; ++i)
  {
    auto response = key.Get(static_cast<Keychain::KeyType>(i));
    ASSERT_TRUE(response.second.Success());
    EXPECT_EQ(response.first, kDummyKeys[i]);
  }

  EXPECT_TRUE(key.Populated());
}

TEST(Key, ValidKeys)
{
  Keychain key(iex::file::TEMP);
  EXPECT_EQ(key.KeychainValidity(), "");
  ASSERT_TRUE(key.KeychainValidity().Success());

  EXPECT_EQ(key.Set(Keychain::KeyType::PUBLIC, "pk_483bb0e8c5dd4a2974d362dd8aad154d"), "");
  EXPECT_EQ(key.Set(Keychain::KeyType::SECRET, "sk_12d3caa449bd4de4b9f063089c47f69b"), "");
  EXPECT_EQ(key.Set(Keychain::KeyType::SANDBOX_PUBLIC, "Tpk_fb19c49530a6f1e9158142010a80043c"), "");
  EXPECT_EQ(key.Set(Keychain::KeyType::SANDBOX_SECRET, "Tsk_d405c80f30a6f1e895814201aa80043f"), "");

  EXPECT_TRUE(key.Populated());
}

TEST(Key, InvalidLengthKeys)
{
  Keychain key(Keychain::EnvironmentFlag{});
  EXPECT_EQ(key.KeychainValidity(), "");
  ASSERT_TRUE(key.KeychainValidity().Success());

  // Invalid length
  EXPECT_NE(key.Set(Keychain::KeyType::PUBLIC, "pk_483bb0e8c5dd4a2974d362dd8aad154"), "");
  EXPECT_NE(key.Set(Keychain::KeyType::SECRET, "sk_12d3caa449bd4de4b9f063089c47f69"), "");
  EXPECT_NE(key.Set(Keychain::KeyType::SANDBOX_PUBLIC, "Tpk_fb19c49530a6f1e9158142010a80043"), "");
  EXPECT_NE(key.Set(Keychain::KeyType::SANDBOX_SECRET, "Tsk_d405c80f30a6f1e895814201aa80043"), "");

  // Invalid characters
  EXPECT_NE(key.Set(Keychain::KeyType::PUBLIC, "pk_483bb0e8c5dd4a2974d362dg8aad154d"), "");
  EXPECT_NE(key.Set(Keychain::KeyType::SECRET, "sk_12d3caa449bd4de4b9f063089c47f-9b"), "");
  EXPECT_NE(key.Set(Keychain::KeyType::SANDBOX_PUBLIC, "Tpk_fb19?49530a6f1e9158142010a80043c"), "");
  EXPECT_NE(key.Set(Keychain::KeyType::SANDBOX_SECRET, "Tsk_d405c80f30a f1e895814201aa80043f"), "");

  // Invalid prefix
  EXPECT_NE(key.Set(Keychain::KeyType::PUBLIC, "sk_483bb0e8c5dd4a2974d362d18aad154d"), "");
  EXPECT_NE(key.Set(Keychain::KeyType::SECRET, "pk_12d3caa449bd4de4b9f063089c47f19b"), "");
  EXPECT_NE(key.Set(Keychain::KeyType::SANDBOX_PUBLIC, "Tsk_fb19149530a6f1e9158142010a80043c"), "");
  EXPECT_NE(key.Set(Keychain::KeyType::SANDBOX_SECRET, "Tpk_d405c80f30a1f1e895814201aa80043f"), "");
}

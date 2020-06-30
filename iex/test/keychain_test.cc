/**
 * @file keychain_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include "iex/keychain.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <vector>

#include "iex/file_serializable.h"
#include "iex/json_serializer.h"

using Keychain = iex::key::Keychain;
namespace fs = std::filesystem;

static constexpr const char* kKeyNameMap[]{"IEX_PUBLIC_KEY", "IEX_SECRET_KEY", "IEX_SANDBOX_PUBLIC_KEY",
                                           "IEX_SANDBOX_SECRET_KEY"};

std::vector<std::string> GetEnv(bool extract = true)
{
  std::vector<std::string> vars(Keychain::NUM_KEYS);

  for (int i = 0; i < Keychain::NUM_KEYS; ++i)
  {
    const char* var = std::getenv(kKeyNameMap[i]);
    if (var == nullptr)
    {
      return {};
    }

    vars[i] = var;
    if (extract && unsetenv(kKeyNameMap[i]))
    {
      return {};
    }
  }
  return vars;
}

void SetEnv(const std::vector<std::string>& vars)
{
  for (int i = 0; i < Keychain::NUM_KEYS; ++i)
  {
    setenv(kKeyNameMap[i], vars[i].c_str(), 1);
  }
}

TEST(Key, EnvironmentEmpty)
{
  const auto vars = GetEnv();
  ASSERT_FALSE(vars.empty());

  const Keychain key(Keychain::EnvironmentFlag{});
  EXPECT_TRUE(key.KeychainValidity().Success()) << key.KeychainValidity();
  EXPECT_FALSE(key.Populated());

  SetEnv(vars);
}

TEST(Key, EnvironmentNotEmptyAndInvalid)
{
  const auto vars = GetEnv();
  ASSERT_FALSE(vars.empty());

  // Second key here in invalid
  SetEnv({"pk_483bb0e8c5dd4a2974d362dd8aad154d", "sk_12d3caa449bd4de4b9f063089c47f",
          "Tpk_fb19c49530a6f1e9158142010a80043c", "Tsk_d405c80f30a6f1e895814201aa80043f"});

  Keychain key(Keychain::EnvironmentFlag{});
  EXPECT_TRUE(key.KeychainValidity().Failure());
  EXPECT_FALSE(key.Populated());
  EXPECT_TRUE(key.Set(Keychain::PUBLIC, "", false).Failure());
  EXPECT_TRUE(key.Get(Keychain::SECRET).second.Failure());

  SetEnv(vars);
}

TEST(Key, EnvironmentPopulated)
{
  const auto vars = GetEnv(false);
  ASSERT_FALSE(vars.empty());

  // This test assumes environment already has valid keys
  const Keychain key(Keychain::EnvironmentFlag{});
  EXPECT_TRUE(key.KeychainValidity().Success()) << key.KeychainValidity();
  EXPECT_TRUE(key.Populated());

  for (int i = 0; i < Keychain::KeyType::NUM_KEYS; ++i)
  {
    const auto response = key.Get(static_cast<Keychain::KeyType>(i));
    ASSERT_TRUE(response.second.Success());
    EXPECT_EQ(response.first, vars[i]);
  }
}

TEST(Key, FileReadFailure)
{
  // Induce failed read
  const fs::path path = "/tmp/iex/keychain.json";
  fs::remove(path);
  fs::create_directory(path);

  const Keychain key(iex::file::TEMP);
  EXPECT_TRUE(key.KeychainValidity().Failure());
  EXPECT_FALSE(key.Populated());

  fs::remove(path);
}

TEST(Key, FileEmpty)
{
  const fs::path path = "/tmp/iex/keychain.json";
  fs::remove(path);

  const Keychain key(iex::file::TEMP);
  EXPECT_TRUE(key.KeychainValidity().Success()) << key.KeychainValidity();
  EXPECT_FALSE(key.Populated());
}

TEST(Key, FileInvalidJSON)
{
  std::ofstream ofstream("/tmp/iex/keychain.json");
  const auto str = iex::json::Json::array();
  ofstream << str;
  ofstream.close();

  const Keychain key(iex::file::TEMP);
  ASSERT_TRUE(key.KeychainValidity().Failure());
  EXPECT_FALSE(key.Populated());
}

TEST(Key, FilePopulated)
{
  static constexpr const char* kDummyKeys[Keychain::NUM_KEYS]{
      "pk_483bb0e8c5dd4a2974d362dd8aad154d", "sk_12d3caa449bd4de4b9f063089c47f69b",
      "Tpk_fb19c49530a6f1e9158142010a80043c", "Tsk_d405c80f30a6f1e895814201aa80043f"};

  iex::json::Json json;
  for (int i = 0; i < Keychain::KeyType::NUM_KEYS; ++i)
  {
    json[kKeyNameMap[i]] = kDummyKeys[i];
  }

  fs::create_directory("/tmp/iex");
  std::ofstream ofstream("/tmp/iex/keychain.json");
  const auto str = json.dump();
  ofstream << str;
  ofstream.close();

  Keychain key(iex::file::TEMP);
  ASSERT_TRUE(key.KeychainValidity().Success()) << key.KeychainValidity();
  EXPECT_TRUE(key.Populated());

  for (int i = 0; i < Keychain::KeyType::NUM_KEYS; ++i)
  {
    auto response = key.Get(static_cast<Keychain::KeyType>(i));
    ASSERT_TRUE(response.second.Success());
    EXPECT_EQ(response.first, kDummyKeys[i]);
  }
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

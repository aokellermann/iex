/**
 * @file file_test.cc
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#include <gtest/gtest.h>

#include <ctime>
#include <string>

#include "iex/file_serializable.h"
#include "iex/json_file_serializer.h"
#include "iex/iex.h"

namespace file = iex::file;
namespace json_file = iex::json_file;

struct FileImpl : file::FileIoBase
{
  FileImpl(const std::string& file_name, file::Directory directory, file::Extension extension)
      : file::FileIoBase(file_name, directory, extension)
  {
  }

  [[nodiscard]] iex::ErrorCode Write(const std::string& contents) const { return FileIoBase::WriteFile(contents); }
  [[nodiscard]] iex::ValueWithErrorCode<std::string> Read() const { return FileIoBase::ReadFile(); }
  [[nodiscard]] iex::ErrorCode Valid() const { return FileIoBase::Validity(); }
};

TEST(File, ReadWrite)
{
  std::string file_name = std::to_string(std::time(nullptr)) + "test";
  FileImpl impl(file_name, file::Directory::TEMP, file::Extension::TEXT);
  EXPECT_EQ(impl.Valid(), iex::ErrorCode());

  std::string test_text("Testing text:\nTesting");
  const auto write_ec = impl.Write(test_text);
  EXPECT_EQ(write_ec, iex::ErrorCode());

  const auto read_response = impl.Read();
  EXPECT_EQ(read_response.first, test_text);
  EXPECT_EQ(read_response.second, iex::ErrorCode());
}

struct JsonFileImpl : json_file::JsonFileBidirectionalSerializable
{
  using json_file::JsonFileBidirectionalSerializable::JsonFileBidirectionalSerializable;

  iex::ErrorCode Serialize() override
  {
    repr_ = json_to_write_;
    return {};
  }

  iex::ErrorCode Deserialize() override
  {
    json_read_ = repr_;
    return {};
  }

  inline static json_file::Json json_to_write_ = { {"name", "value"} };
  json_file::Json json_read_;
};

TEST(JsonFile, ReadWrite)
{
  std::string file_name = std::to_string(std::time(nullptr)) + "json_test_1";

  {
    JsonFileImpl impl(file_name, file::Directory::TEMP);
    EXPECT_EQ(impl.Serialize(), iex::ErrorCode());
  }

  {
    JsonFileImpl impl(file_name, file::Directory::TEMP);
    EXPECT_EQ(impl.Deserialize(), iex::ErrorCode());
    EXPECT_EQ(impl.json_to_write_.dump(4), impl.json_read_.dump(4));
    EXPECT_EQ(impl.json_to_write_["name"].get<std::string>(), "value");
  }
}
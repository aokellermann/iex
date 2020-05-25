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

#include <gtest/gtest.h>

#include <ctime>
#include <string>

#include "iex/file_serializable.h"
#include "iex/iex.h"

namespace file = iex::file;

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
  FileImpl impl(file_name, file::Directory::Temp, file::Extension::Text);
  EXPECT_EQ(impl.Valid(), iex::ErrorCode());

  std::string test_text("Testing text:\nTesting");
  const auto write_ec = impl.Write(test_text);
  EXPECT_EQ(write_ec, iex::ErrorCode());

  const auto read_response = impl.Read();
  EXPECT_EQ(read_response.first, test_text);
  EXPECT_EQ(read_response.second, iex::ErrorCode());
}

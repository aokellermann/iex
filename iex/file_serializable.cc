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

#include "iex/file_serializable.h"

#include <fstream>
#include <stdexcept>
#include <string>

#include "iex/env.h"

namespace iex::file
{
namespace
{
// region Reading and Writing

ErrorCode CreateDirectory(const Path &path)
{
  if (fs::is_directory(path))
  {
    return {};
  }

  if (fs::exists(path))
  {
    return ErrorCode("Path is a file", {"path", ErrorCode(path.string())});
  }

  bool success = false;
  try
  {
    success = fs::create_directory(path);
  }
  catch (const std::exception &e)
  {
    return ErrorCode("fs::create_directory failed",
                     {{"path", ErrorCode(path.string())}, {"error", ErrorCode(e.what())}});
  }

  if (!success)
  {
    return ErrorCode("fs::create_directory failed", {"path", ErrorCode(path.string())});
  }

  return {};
}

ErrorCode OpenFileStream(const Path &path, std::fstream &stream, std::ios_base::openmode om)
{
  try
  {
    stream.open(path, om);
  }
  catch (const std::exception &e)
  {
    return ErrorCode("fstream::open failed",
                     {{"path", ErrorCode(path.string())},
                      {"error", ErrorCode(e.what())},
                      {"openmode", ErrorCode(std::to_string(static_cast<int64_t>(om)))}});
  }

  if (!stream)
  {
    return ErrorCode(
        "fstream::open failed",
        {{"path", ErrorCode(path.string())}, {"openmode", ErrorCode(std::to_string(static_cast<int64_t>(om)))}});
  }

  return {};
}

ErrorCode WriteStream(const Path &path, const std::string &contents, std::ostream &ostream)
{
  try
  {
    ostream.write(contents.c_str(), contents.size());
  }
  catch (const std::exception &e)
  {
    return ErrorCode("ostream::write failed", {{"path", ErrorCode(path.string())}, {"error", ErrorCode(e.what())}});
  }

  if (!ostream)
  {
    return ErrorCode("ostream::write failed", {{"path", ErrorCode(path.string())}});
  }

  return {};
}

ValueWithErrorCode<std::string> ReadStream(const std::filesystem::path &path, std::istream &istream)
{
  const auto size = fs::file_size(path);
  std::string data(size, '\0');
  try
  {
    istream.read(data.data(), size);
  }
  catch (const std::exception &e)
  {
    return {{},
            ErrorCode("istream::read failed", {{"path", ErrorCode(path.string())}, {"error", ErrorCode(e.what())}})};
  }

  if (!istream)
  {
    return {{}, ErrorCode("istream::read failed", {{"path", ErrorCode(path.string())}})};
  }

  return {data, {}};
}

ErrorCode WriteFile(const Path &path, const std::string &contents)
{
  std::fstream out;
  OpenFileStream(path, out, std::ios_base::out);
  return WriteStream(path, contents, out);
}

ValueWithErrorCode<std::string> ReadFile(const Path &path)
{
  std::fstream in;
  OpenFileStream(path, in, std::ios_base::in);
  return ReadStream(path, in);
}

// endregion Reading and Writing

}  // namespace

FileIoBase::FileIoBase(const Path &relative_path, const Directory directory, const Extension extension)
    : directory_path_(GetDirectoryName(directory)),
      full_path_((directory_path_ / relative_path).string() + GetExtension(extension))
{
  if (ec_.Success())
  {
    ec_ = CreateDirectory(directory_path_);
  }
}

Path FileIoBase::GetDirectoryName(const Directory directory)
{
  switch (directory)
  {
    case Directory::Home:
    {
      const auto res = env::GetEnv("HOME");
      ec_ = res.second;
      return res.first / Path(".iex");
    }

    case Directory::Temp:
    {
      return "/tmp/iex";
    }

    default:
      ec_ = ErrorCode("Invalid Directory");
      return Path();
  }
}

std::string FileIoBase::GetExtension(const Extension extension)
{
  switch (extension)
  {
    case Extension::Text:
    {
      return ".txt";
    }
    case Extension::Json:
    {
      return ".json";
    }
    default:
    {
      ec_ = ErrorCode("Invalid Extension");
      return "";
    }
  }
}

ErrorCode FileIoBase::WriteFile(const std::string &contents) const
{
  return ::iex::file::WriteFile(full_path_, contents);
}

ValueWithErrorCode<std::string> FileIoBase::ReadFile() const { return ::iex::file::ReadFile(full_path_); }

}  // namespace iex::file

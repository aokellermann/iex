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

#include <filesystem>
#include <string>
#include <utility>

#include "iex/iex.h"

namespace iex::file
{
namespace fs = std::filesystem;
using Path = fs::path;

/**
 * The directory that a FileIOBase is associated with.
 */
enum Directory
{
  /**
   * $HOME/.iex
   */
  Home,

  /**
   * /tmp/iex
   */
  Temp
};

/**
 * The extension that a file has.
 */
enum Extension
{
  /**
   * Plain text: .txt
   */
  Text,

  /**
   * JSON format: .json
   */
  Json
};

/**
 * This is used as a base class for classes that can be read from or written to an associated file.
 * Note: these member functions are not thread-safe by design.
 */
class FileIoBase
{
 public:
  /**
   * Creates a FileIOBase with an associated file path based on relative_path and directory.
   * @param relative_path the relative path from directory to which the FileIOBase is associated with
   * @param directory Directory enum indicating absolute path to which relative_path is in
   */
  explicit FileIoBase(const Path& relative_path, Directory directory = Home, Extension extension = Json);

 protected:
  /**
   * Writes contents to the associated file.
   * @param contents data to write
   * @return ErrorCode indicating success or failure
   */
  [[nodiscard]] ErrorCode WriteFile(const std::string& contents) const;

  /**
   * Reads contents of associated file.
   * @return Contents of file if success, or ErrorCode denoting failure.
   */
  [[nodiscard]] ValueWithErrorCode<std::string> ReadFile() const;

  /**
   * Returns whether this class instance is valid to be used.
   * @return ErrorCode denoting whether valid or not
   */
  [[nodiscard]] const ErrorCode& Validity() const noexcept { return ec_; }

 private:
  [[nodiscard]] Path GetDirectoryName(Directory directory);

  [[nodiscard]] std::string GetExtension(Extension extension);

  const Path directory_path_;
  const Path full_path_;
  ErrorCode ec_;
};

}  // namespace iex::file

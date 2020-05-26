/**
 * @file file_serializable.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <filesystem>
#include <string>
#include <utility>

#include "iex/iex.h"

/**
 * Contains declarations necessary for writing to and reading from local files.
 */
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
  HOME,

  /**
   * /tmp/iex
   */
  TEMP
};

/**
 * The extension that a file has.
 */
enum Extension
{
  /**
   * Plain text: .txt
   */
  TEXT,

  /**
   * JSON format: .json
   */
  JSON
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
  explicit FileIoBase(const Path& relative_path, Directory directory = HOME, Extension extension = JSON);

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
  [[nodiscard]] Path GetDirectoryPath(Directory directory);

  [[nodiscard]] std::string GetExtensionString(Extension extension);

  const Path directory_path_;
  const Path full_path_;
  ErrorCode ec_;
};

}  // namespace iex::file

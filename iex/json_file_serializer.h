/**
 * @file json_file_serializer.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include "iex/file_serializable.h"
#include "iex/json_serializer.h"

/**
 * Contains base classes for classes than can be serialized in JSON format to and from an associated file.
 */
namespace iex::json_file
{
using Json = json::Json;
using Path = file::Path;
using Directory = file::Directory;
using Extension = file::Extension;

/**
 * Base class for types that can be serialized in JSON format to an associated file.
 *
 * On destruction, serializes the data in JsonBase::repr_ and writes the data to the associated file.
 *
 * Derived classes are able to override JsonBidirectionalSerializable with custom serialization logic.
 */
struct JsonFileSerializable : public file::FileIoBase, public json::JsonSerializable
{
  explicit JsonFileSerializable(const file::Path& relative_path, Directory directory = Directory::HOME)
      : file::FileIoBase(relative_path, directory, Extension::JSON)
  {
  }

  ~JsonFileSerializable() { WriteFile(repr_.dump()); }  // NOLINT
};

/**
 * Base class for types that can be serialized in JSON format from an associated file.
 *
 * On instantiation, deserializes from the associated file and stores the data in JsonBase::repr_.
 *
 * Derived classes are able to override JsonBidirectionalSerializable with custom deserialization logic.
 */
struct JsonFileDeserializable : public file::FileIoBase, public json::JsonDeserializable
{
  explicit JsonFileDeserializable(const file::Path& relative_path, Directory directory = Directory::HOME)
      : file::FileIoBase(relative_path, directory, Extension::JSON)
  {
    const auto file_data = ReadFile().first;
    if (!file_data.empty())
    {
      repr_ = Json::parse(ReadFile().first);
    }
  }
};

/**
 * Base class for types that can be serialized in JSON format to and from an associated file.
 *
 * On instantiation, deserializes from the associated file and stores the data in JsonBase::repr_.
 * On destruction, serializes the data in JsonBase::repr_ and writes the data to the associated file.
 *
 * Derived classes are able to override JsonBidirectionalSerializable with custom serialization logic.
 */
struct JsonFileBidirectionalSerializable : public file::FileIoBase, public json::JsonBidirectionalSerializable
{
  explicit JsonFileBidirectionalSerializable(const file::Path& relative_path, Directory directory = Directory::HOME)
      : file::FileIoBase(relative_path, directory, Extension::JSON)
  {
    const auto file_data = ReadFile().first;
    if (!file_data.empty())
    {
      repr_ = Json::parse(ReadFile().first);
    }
  }

  ~JsonFileBidirectionalSerializable() { WriteFile(repr_.dump()); }  // NOLINT
};

}  // namespace iex::json_file

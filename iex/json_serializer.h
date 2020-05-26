/**
 * @file json_serializer.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <nlohmann/json.hpp>

#include "iex/iex.h"

/**
 * Contains abstract interfaces used for converting to and from JSON format.
 */
namespace iex::json
{
using Json = nlohmann::json;

class JsonBase
{
 protected:
  Json repr_;
};

/**
 * This is an abstract interface for objects that can be serialized with Json data.
 */
struct JsonSerializable : virtual JsonBase
{
  /**
   * This function is intended to be overridden such that it modifies JsonBase::repr_ with the type's intended Json
   * representation. The default implementation of this function does nothing.
   * @return ErrorCode denoting success or failure
   */
  virtual ErrorCode Serialize() { return {}; }
};

/**
 * This is an abstract interface for objects that can be deserialized with Json data.
 */
struct JsonDeserializable : virtual JsonBase
{
  /**
   * This function is intended to be overridden such that it reads JsonBase::repr_ and populates the derived class with
   * the contained data. The default implementation of this function does nothing.
   * @return ErrorCode denoting success or failure
   */
  virtual ErrorCode Deserialize() { return {}; }
};

/**
 * This is an abstract interface for objects that can be both serialized and deserialized with Json data.
 */
struct JsonBidirectionalSerializable : JsonSerializable, JsonDeserializable
{
};

}  // namespace iex::json

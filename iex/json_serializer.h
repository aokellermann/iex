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
   * Creates and returns a Json object representing this object. This function must be overridden.
   * @return Json
   */
  virtual ErrorCode Serialize() = 0;
};

/**
 * This is an abstract interface for objects that can be deserialized with Json data.
 */
struct JsonDeserializable : virtual JsonBase
{
  /**
   * Stores data from input_json in this object. This function must be overridden.
   * @return ErrorCode if failure
   */
  virtual ErrorCode Deserialize() = 0;
};

/**
 * This is an abstract interface for objects that can be both serialized and deserialized with Json data.
 */
struct JsonBidirectionalSerializable : JsonSerializable, JsonDeserializable
{
};

}  // namespace iex::json

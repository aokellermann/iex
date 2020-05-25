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

/**
 * This is an abstract interface for objects that can be serialized with Json data.
 */
class JsonSerializable
{
 protected:
  /**
   * Creates and returns a Json object representing this object. This function must be overridden.
   * @return Json
   */
  virtual ValueWithErrorCode<Json> Serialize() = 0;
};

/**
 * This is an abstract interface for objects that can be deserialized with Json data.
 */
class JsonDeserializable
{
 protected:
  /**
   * Stores data from input_json in this object. This function must be overridden.
   * @return ErrorCode if failure
   */
  virtual ErrorCode Deserialize(const Json& input_json) = 0;
};

/**
 * This is an abstract interface for objects that can be both serialized and deserialized with Json data.
 */
class JsonBidirectionalSerializable : protected JsonSerializable, protected JsonDeserializable
{
};

}  // namespace iex::json

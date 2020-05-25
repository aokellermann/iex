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

#include <nlohmann/json.hpp>

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

/**
 * @file json_serializer.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>

#include "iex/iex.h"

/**
 * Contains abstract interfaces used for converting to and from JSON format.
 */
namespace iex::json
{
using Json = nlohmann::json;

template <typename T>
using Member = std::optional<T>;

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

class JsonStorage : public JsonDeserializable
{
 public:
  explicit JsonStorage(const json::Json& json = {}) { Deserialize(json); }

  ErrorCode Deserialize(const Json& input_json) final
  {
    json_ = input_json;
    return {};
  }

  template <typename T>
  Member<T> SafeGetMember(const char* member_name) const noexcept
  {
    try
    {
      const auto ref = json_.at(member_name);
      if (ref.is_null())
      {
        return std::nullopt;
      }

      return GetMember<T>(ref);
    }
    catch (const std::exception& e)
    {
      return std::nullopt;
    }
  }

 private:
  template <typename T>
  T GetMember(Json::const_reference ref) const
  {
    return ref.get<T>();
  }

  Json json_;
};

template <>
inline Timestamp JsonStorage::GetMember<Timestamp>(Json::const_reference ref) const
{
  return Timestamp{ref.get<uint64_t>()};
}

}  // namespace iex::json

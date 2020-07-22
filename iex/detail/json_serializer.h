/**
 * @file json_serializer.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <type_traits>

#include "iex/detail/common.h"

/**
 * Contains abstract interfaces used for converting to and from JSON format.
 */
namespace iex::json
{
using Json = nlohmann::json;

template <typename T>
using Member = std::optional<T>;

using MemberName = const char* const;

/**
 * This is an abstract interface for objects that can be serialized with Json data.
 */
class JsonSerializable
{
 public:
  virtual ~JsonSerializable() = default;

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
 public:
  virtual ~JsonDeserializable() = default;

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
 public:
  ~JsonBidirectionalSerializable() override = default;
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
  static Member<T> SafeGetMember(const Json& json, const MemberName& member_name) noexcept
  {
    try
    {
      const auto ref = json.at(member_name);
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

  template <typename T>
  Member<T> SafeGetMember(const MemberName& member_name) const noexcept
  {
    return SafeGetMember<T>(json_, member_name);
  }

  [[nodiscard]] auto Type() const noexcept { return json_.type(); }

  [[nodiscard]] auto begin() const noexcept { return json_.begin(); }

  [[nodiscard]] auto end() const noexcept { return json_.end(); }

 private:
  template <typename T>
  static Member<T> GetMember(Json::const_reference& ref)
  {
    return ref.get<T>();
  }

  Json json_;
};

template <>
inline Member<Timestamp> JsonStorage::GetMember<Timestamp>(Json::const_reference& ref)
{
  const auto ms = ref.get<int64_t>();

  // IEX documentation specifies that a 0 or -1 value may be present.
  if (ms <= 0)
  {
    return std::nullopt;
  }

  return Timestamp{ms};
}

}  // namespace iex::json
/**
 * @file utils.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <cctype>
#include <string>

namespace iex::utils
{
/**
 * Converts all letters in str to uppercase.
 * @param str the string to modify
 * @return str
 */
inline std::string& ToUpper(std::string& str)
{
  for (auto& c : str)
  {
    c = std::toupper(static_cast<unsigned char>(c));
  }

  return str;
}
}  // namespace iex::utils
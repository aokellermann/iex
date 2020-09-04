/**
 * @file utils.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <numeric>
#include <string>

namespace iex::utils
{
template <typename T>
const std::function<T&(T&)> kIdentityFunc = [](T& t) -> T& { return t; };

/**
 * Converts all letters in str to uppercase.
 * @param str the string to modify
 * @return str
 */
template <typename InputIt>
void ToUpper(InputIt begin, InputIt end)
{
  std::for_each(begin, end, [](auto& c) { c = static_cast<std::decay_t<decltype(c)>>(std::toupper(c)); });
}

template <typename T>
T& ToUpper(T& t)
{
  ToUpper(t.begin(), t.end());
  return t;
}

template <typename T = std::string, typename D, typename Transform, typename InputIt>
T Join(InputIt begin, InputIt end, const D& delimiter, const Transform& transform_func = kIdentityFunc<T>)
{
  if (begin == end) return T();
  auto first = T(transform_func(*begin));
  return std::accumulate(++begin, end, std::move(first), [&delimiter, &transform_func](auto& acc, auto& next) {
    auto xform = transform_func(next);
    auto news = T(xform);
    return acc += delimiter + news;
  });
}
}  // namespace iex::utils
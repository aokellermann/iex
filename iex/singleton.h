/**
 * @file singleton.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

/**
 * Contains all declarations necessary for obtaining a program-wide unique object of a given type.
 */
namespace iex::singleton
{
/**
 * @brief Thread-safe singleton getter.
 * @see https://en.cppreference.com/w/cpp/language/storage_duration#Static_local_variables
 * @tparam T The class or struct you want to get the instance of.
 * @return Reference to a static instance of T.
 */
template <typename T>
static T& GetInstance()
{
  // Meyers Singleton
  static T instance;
  return instance;
}
}  // namespace iex::singleton

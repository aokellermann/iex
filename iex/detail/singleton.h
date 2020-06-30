/**
 * @file singleton.h
 * @author Antony Kellermann
 * @copyright 2020 Antony Kellermann
 */

#pragma once

#include <memory>
#include <mutex>
#include <utility>

/**
 * Contains all declarations necessary for obtaining a unique static instance of a given type.
 */
namespace iex::singleton
{
namespace detail
{
/**
 * Contains an std::once_flag used for T's construction, and a corresponding std::unique_ptr of that type so that the
 * memory is automatically managed.
 * @tparam T The type to get instance-related information about
 */
template <typename T>
struct PtrAndOnceFlag
{
  std::once_flag flag;
  std::unique_ptr<T> ptr;
};

/**
 * Gets a singleton PtrAndOnceFlag corresponding to T.
 * @tparam T The type to get instance-related information about
 * @return Reference to PtrAndOnceFlag<T> singleton
 */
template <typename T>
PtrAndOnceFlag<T>& GetPtrAndFlag()
{
  // Meyers Singleton aka "Magic Static"
  static PtrAndOnceFlag<T> ptr_and_flag;
  return ptr_and_flag;
}

}  // namespace detail

/**
 * @brief Thread-safe singleton getter.
 * @see https://en.cppreference.com/w/cpp/language/storage_duration#Static_local_variables
 * @tparam T The class or struct you want to get the instance of.
 * @tparam Args Types of parameters to be passed to T's constructor
 * @param args Parameters to be passed to T's constructor
 * @return Reference to a static instance of T.
 */
template <typename T, typename... Args>
static T& GetInstance(Args&&... args)
{
  // Due to usage of parameter pack, Meyers Singletons ("magic static") cannot be used in this template function.
  // However, we can implement a workaround using a Meyers Singleton in a separate function.

  auto& ptr_and_flag = detail::GetPtrAndFlag<T>();
  std::call_once(ptr_and_flag.flag,
                 [&ptr_and_flag, &args...] { ptr_and_flag.ptr = std::make_unique<T>(std::forward<Args>(args)...); });
  return *ptr_and_flag.ptr;
}
}  // namespace iex::singleton

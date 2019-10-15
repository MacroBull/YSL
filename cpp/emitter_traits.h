
#pragma once

#include <type_traits>

namespace YAML
{

namespace detail
{

//// traits

template <bool P, typename T = void>
using enable_if_t = typename std::enable_if<P, T>::type;

template <typename T>
inline enable_if_t<!(std::is_same<T, char>::value || std::is_same<T, unsigned char>::value), T>
as_numeric(const T& value)
{
	return value;
}

template <typename T>
inline enable_if_t<std::is_same<T, char>::value || std::is_same<T, unsigned char>::value, int>
as_numeric(const T& value)
{
	return static_cast<int>(value);
}

} // namespace detail

} // namespace YAML

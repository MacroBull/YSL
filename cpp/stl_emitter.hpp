/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <complex>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "emitter_extra.hpp"

namespace YAML
{

namespace detail
{

//// traits

template <typename T, typename Test = void>
struct stl_is_std_iterable : std::false_type
{};

template <typename T>
struct stl_is_std_iterable<
		T,
		enable_if_t<std::is_same<decltype(std::begin(std::declval<T>())),
								 decltype(std::end(std::declval<T>()))>::value>>
	: std::true_type
{};

#define TRAITS_DECL_CLASS_HAS_TYPE(name)                                                       \
	template <class T, typename Test = void>                                                   \
	struct stl_has_type_##name : std::false_type                                               \
	{};                                                                                        \
	template <class T>                                                                         \
	struct stl_has_type_##name<T, void_t<typename T::name>> : std::true_type                   \
	{};

TRAITS_DECL_CLASS_HAS_TYPE(element_type)
TRAITS_DECL_CLASS_HAS_TYPE(mapped_type)

#undef TRAITS_DECL_CLASS_HAS_TYPE

} // namespace detail

//// std::complex

template <typename T>
inline Emitter& operator<<(Emitter& emitter, const std::complex<T>& value)
{
	//	return detail::emit_streamable(emitter << LocalTag("complex"), value); // stl style
	return detail::emit_complex(emitter, value.real(), value.imag());
}

//// std::pair

template <typename T1, typename T2>
inline Emitter& operator<<(Emitter& emitter, const std::pair<T1, T2>& value)
{
	return emitter << Flow << BeginSeq << value.first << value.second << EndSeq;
}

//// std::tuple

template <typename... Args>
inline Emitter& operator<<(Emitter& emitter, const std::tuple<Args...>& value)
{
	emitter << Flow << BeginSeq;
	detail::sequential_printer<std::tuple<Args...>, sizeof...(Args)>::print(emitter, value);
	return emitter << EndSeq;
}

//// std::array, std::vector, std::deque, std::list, std::forward_list
//// std::set, std::multiset, std::unordered_set

template <typename T>
inline detail::enable_if_t<detail::stl_is_std_iterable<T>::value &&
								   !detail::stl_has_type_mapped_type<T>::value,
						   Emitter&>
operator<<(Emitter& emitter, const T& value)
{
	return detail::emit_sequence(emitter, value);
}

//// std::map, std::unordered_map

template <typename T>
inline detail::enable_if_t<detail::stl_is_std_iterable<T>::value &&
								   detail::stl_has_type_mapped_type<T>::value,
						   Emitter&>
operator<<(Emitter& emitter, const T& value)
{
	return detail::emit_mapping(emitter, value);
}

//// std::unique_ptr, std::shared_ptr

template <typename T>
inline detail::enable_if_t<detail::stl_has_type_element_type<T>::value, Emitter&>
operator<<(Emitter& emitter, const T& value)
{
	return emitter << value.get();
}

//// HINT: general (non-STL) streamable (AS TAGGED LITERAL)

template <typename T>
inline detail::enable_if_t<!(std::is_enum<T>::value || std::is_pointer<T>::value ||
							 detail::stl_is_std_iterable<T>::value ||
							 detail::stl_has_type_element_type<T>::value),
						   Emitter&>
operator<<(Emitter& emitter, const T& value)
{
	return detail::emit_streamable(
			emitter << LocalTag(detail::typeid_name<T>()) << Literal, value);
}

} // namespace YAML

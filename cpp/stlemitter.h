
#pragma once

#include <complex>
#include <tuple>
#include <type_traits>
#include <utility>

#if !defined(YAML_EMITTER_NO_COMPLEX)

#include <sstream>

#endif

#include <yaml-cpp/emitter.h>

#include "emitter_traits.h"

namespace YAML
{

namespace detail
{

//// traits

template <typename... Args>
struct make_void
{
	using type = void;
};

template <typename... Args>
using void_t = typename make_void<Args...>::type;

template <typename T, typename Test = void>
struct is_std_iterable : std::false_type
{
};

template <typename T>
struct is_std_iterable<T,
					   enable_if_t<std::is_same<decltype(std::begin(std::declval<T>())),
												decltype(std::end(std::declval<T>()))>::value>>
	: std::true_type
{
};

#define TRAITS_DECL_CLASS_HAS_TYPE(name)                                                       \
	template <class T, typename Test = void>                                                   \
	struct has_type_##name : std::false_type                                                   \
	{                                                                                          \
	};                                                                                         \
	template <class T>                                                                         \
	struct has_type_##name<T, void_t<typename T::name>> : std::true_type                       \
	{                                                                                          \
	};

TRAITS_DECL_CLASS_HAS_TYPE(element_type)
TRAITS_DECL_CLASS_HAS_TYPE(mapped_type)

#undef TRAITS_DECL_CLASS_HAS_TYPE

//// helpers

template <typename T, size_t N>
struct tuple_emitter
{
	inline static void emit(Emitter& emitter, const T& value)
	{
		const auto i = N - 1;

		tuple_emitter<T, i>::emit(emitter, value);
		emitter << std::get<i>(value);
	}
};

template <typename T>
struct tuple_emitter<T, 1>
{
	inline static void emit(Emitter& emitter, const T& value)
	{
		emitter << std::get<0>(value);
	}
};

template <typename T>
inline Emitter& emit_sequence(Emitter& emitter, const T& value)
{
	emitter << BeginSeq;
	for (const auto& item : value)
	{
		emitter << item;
	}
	return emitter << EndSeq;
}

template <typename T>
inline Emitter& emit_mapping(Emitter& emitter, const T& value)
{
	emitter << BeginMap;
	for (const auto& key_value : value)
	{
		emitter << Key << std::get<0>(key_value) << Value << std::get<1>(key_value);
	}
	return emitter << EndMap;
}

} // namespace detail

//// std::complex

template <typename T>
inline Emitter& operator<<(Emitter& emitter, const std::complex<T>& value)
{
#ifndef YAML_EMITTER_NO_COMPLEX

	std::stringstream ss;
	ss << detail::as_numeric(value.real()) << '+' << detail::as_numeric(value.imag()) << 'j';
	emitter << LocalTag("complex") << ss.str();
	return emitter;

#else

	return emitter << Flow << BeginSeq << value.real() << value.imag() << EndSeq;

#endif
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
	detail::tuple_emitter<std::tuple<Args...>, sizeof...(Args)>::emit(emitter, value);
	return emitter << EndSeq;
}

//// std::array, std::vector, std::deque, std::list, std::forward_list
//// std::set, std::multiset, std::unordered_set

template <typename T>
inline detail::enable_if_t<
		detail::is_std_iterable<T>::value && !detail::has_type_mapped_type<T>::value, Emitter&>
operator<<(Emitter& emitter, const T& value)
{
	return detail::emit_sequence(emitter, value);
}

//// std::map, std::unordered_map

template <typename T>
inline detail::enable_if_t<
		detail::is_std_iterable<T>::value && detail::has_type_mapped_type<T>::value, Emitter&>
operator<<(Emitter& emitter, const T& value)
{
	return detail::emit_mapping(emitter, value);
}

//// std::unique_ptr, std::shared_ptr

template <typename T>
inline detail::enable_if_t<detail::has_type_element_type<T>::value, Emitter&>
operator<<(Emitter& emitter, const T& value)
{
	return emitter << value.get();
}

} // namespace YAML

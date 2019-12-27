/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <sstream>
#include <tuple>
#include <typeinfo>
#include <utility>

//// define YAML_DEF_EMIT_NO_COMPLEX to emit complex numbers as sequence
//// define YAML_DEF_EMIT_ENABLE_GENERAL_DEMANGLED_TAG to enable general demangled tag
////   currently for GCC only

// #define YAML_DEF_EMIT_NO_COMPLEX
// #define YAML_DEF_EMIT_ENABLE_GENERAL_DEMANGLED_TAG

#if defined(YAML_DEF_EMIT_ENABLE_GENERAL_DEMANGLED_TAG) && defined(__GNUG__)

#include <cassert>

#include <cxxabi.h>

#endif

#include "yaml-cpp/emitter.h"

// #include "yaml-cpp/traits.h" // HINT: provide YAML::is_streamable since 0.6.3

namespace YAML
{

namespace detail
{

//// traits

template <bool P, typename T = void>
using enable_if_t = typename std::enable_if<P, T>::type;

template <typename T>
using decay_t = typename std::decay<T>::type;

template <typename T>
using remove_pointer_t = typename std::remove_pointer<T>::type;

template <typename... Args>
struct make_void
{
	using type = void;
};

template <typename... Args>
using void_t = typename make_void<Args...>::type;

template <typename>
unsigned int is_complete_helper(...);

template <typename T>
signed int is_complete_helper(char (*)[sizeof(T)]);

template <typename T>
using is_complete = std::is_same<decltype(is_complete_helper<T>(nullptr)), signed int>;

template <class S, class T, typename Test = void>
struct is_streamable : std::false_type
{};

template <class S, class T>
struct is_streamable<S, T, void_t<decltype(std::declval<S&>() << std::declval<T>())>>
	: std::true_type
{};

//// helpers

template <typename T>
inline T as_numeric(T&& value)
{
	return value;
}

inline int as_numeric(char value)
{
	return static_cast<int>(value);
}

inline int as_numeric(unsigned char value)
{
	return static_cast<unsigned int>(value);
}

template <typename T, size_t N>
struct sequential_printer
{
	template <typename S>
	inline static void print(S& stream, const T& value)
	{
		constexpr auto i = N - 1;

		sequential_printer<T, i>::print(stream, value);
		stream << std::get<i>(value);
	}
};

template <typename T>
struct sequential_printer<T, 0>
{
	template <typename S>
	inline static void print(S& /*stream*/, const T& /*value*/)
	{}
};

template <typename E, typename T> // E can be any Emitter-like
inline E& emit_sequence(E& emitter, T&& value)
{
	emitter << BeginSeq;
	for (const auto& item : value)
	{
		emitter << item;
	}
	return emitter << EndSeq;
}

template <typename E, typename T> // E can be any Emitter-like
inline E& emit_mapping(E& emitter, T&& value)
{
	emitter << BeginMap;
	for (const auto& key_value : value)
	{
		emitter << Key << std::get<0>(key_value) << Value << std::get<1>(key_value);
	}
	return emitter << EndMap;
}

template <typename T>
inline Emitter&
emit_streamable(Emitter& emitter, T&& value, std::stringstream* stream = nullptr)
{
	std::stringstream stream_;
	if (stream == nullptr)
	{
#ifdef YSL_NAMESPACE // extension

		emitter.SetStreamablePrecision<T>(stream_);

#endif

		stream = &stream_;
	}
	else
	{
		stream->str("");
	}

	*stream << value;
	return emitter << stream->str();
}

template <typename T>
inline Emitter& emit_complex(Emitter& emitter, T&& real, T&& imag)
{
#ifndef YAML_DEF_EMIT_NO_COMPLEX

	std::stringstream ss;

#ifdef YSL_NAMESPACE // extension

	emitter.SetStreamablePrecision<T>(ss);

#endif

	ss << as_numeric(std::forward<T>(real)) << '+' << as_numeric(std::forward<T>(imag)) << 'j';
	return emitter << LocalTag("complex") << ss.str();

#else

	return emitter << Flow << BeginSeq << as_numeric(real) << as_numeric(imag) << EndSeq;

#endif
}

template <typename T>
inline std::string typeid_name()
{

#if defined(YAML_DEF_EMIT_ENABLE_GENERAL_DEMANGLED_TAG) && defined(__GNUG__)

	int status(0);

	const auto realname = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
	assert(status == 0 && "call abi::__cxa_demangle() failed");

	const std::string name(realname);
	free(realname);

#else

	const std::string name(typeid(T).name());

#endif

	return name;
}

//// ranked generic emit implementation

template <typename T, size_t R, typename Test = void>
struct generic_emitter : generic_emitter<T, R - 1>
{};

template <typename T>
struct generic_emitter<T, 0>
{};

//// generic streamable (AS TAGGED LITERAL)

template <typename T>
struct generic_emitter<T, 1, enable_if_t<is_streamable<std::ostream, T>::value>>
{
	inline static Emitter& emit(Emitter& emitter, const T& value)
	{
		return detail::emit_streamable(emitter << LocalTag(detail::typeid_name<T>()) << Literal,
									   value); //
	}
};

//// generic enums

template <typename T>
struct generic_emitter<T, 2, enable_if_t<std::is_enum<T>::value>>
{
	inline static Emitter& emit(Emitter& emitter, T value)
	{
		emitter << LocalTag(detail::typeid_name<decay_t<T>>());
		return emitter.WriteIntegralType(value);
	}
};

//// generic (maybe imcomplete) pointers

template <typename T>
struct generic_emitter<T, 2, enable_if_t<std::is_pointer<T>::value>>
{
	using P = decay_t<remove_pointer_t<T>>;

	inline static Emitter& emit(Emitter& emitter, T value)
	{
		emitter << LocalTag(detail::typeid_name<P*>()); // tag ptr typeid
		if (value == nullptr)
		{
			return emitter << _Null{};
		}

		return emitter.WriteIntegralType(value);
	}
};

//// generic complete pointers

template <typename T>
struct generic_emitter<T, 3,
					   enable_if_t<std::is_pointer<T>::value &&
								   detail::is_complete<remove_pointer_t<T>>::value>>
{
	using P = decay_t<remove_pointer_t<T>>;

	inline static Emitter& emit(Emitter& emitter, T value)
	{
		// emitter << LocalTag(detail::typeid_name<P*>()); // tag ptr typeid
		if (value == nullptr)
		{
			return emitter << _Null{};
		}

		return emitter << *value;
	}
};

} // namespace detail

//// use generic_emitter for generic types

template <typename T>
inline Emitter& operator<<(Emitter& emitter, const T& value)
{
	return detail::generic_emitter<T, 8>::emit(emitter, value);
}

//// extra pointers

inline Emitter& operator<<(Emitter& emitter, std::nullptr_t)
{
	return emitter << _Null{};
}

//// sequential

template <typename... Args>
struct Sequential
{
	/*const*/ std::tuple<Args...> values;

	template <typename S>
	inline void print(S& stream) const
	{
		detail::sequential_printer<std::tuple<Args...>, sizeof...(Args)>::print(stream, values);
	}
};

template <typename... CArgs>
inline Sequential<CArgs...> make_sequential(CArgs... args)
{
	return {std::move(std::make_tuple(std::forward<CArgs>(args)...))};
}

template <typename... Args>
inline Emitter& operator<<(Emitter& emitter, const Sequential<Args...>& value)
{
	value.print(emitter);
	return emitter;
}

} // namespace YAML

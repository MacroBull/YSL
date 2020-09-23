
#pragma once

#include <google/protobuf/message.h>

#include "emitter_extra.hpp"

namespace YAML
{
namespace detail
{

//// google::protobuf::Message

template <typename T>
struct generic_emitter<T, 5, enable_if_t<std::is_base_of<google::protobuf::Message, T>::value>>
{
	inline static Emitter& emit(Emitter& emitter, const T& value)
	{
		using namespace google::protobuf;

		const auto descriptor = value.GetDescriptor();
		GOOGLE_CHECK_NOTNULL(descriptor);

		const auto file_descriptor = descriptor->file();
		GOOGLE_CHECK_NOTNULL(file_descriptor);

		const auto text =
				std::string("{\n").append(value.DebugString()).append("}"); // add extra {}

#if GOOGLE_PROTOBUF_VERSION >= 3000000

		switch (file_descriptor->syntax())
		{
		case FileDescriptor::SYNTAX_PROTO3:
		{
			return emitter << LocalTag("pb3_message") << Literal << text;
		}
		case FileDescriptor::SYNTAX_PROTO2:
		case FileDescriptor::SYNTAX_UNKNOWN: // HINT: fallback to pb2 by default
		default:                             // HINT: fallback to pb2 by default
		{
			return emitter << LocalTag("pb2_message") << Literal << text;
		}
		}

#else

		return emitter << LocalTag("pb2_message") << Literal << text;

#endif
	}
};

} // namespace detail
} // namespace YAML

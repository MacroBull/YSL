/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <yaml-cpp/emitter.h>

#include <glog/logging.h>

#include "reconstructable.hpp"

//// YSL configs

#ifdef YSL_NAMESPACE // namespace can be redefined

#define YSL_NS_ YSL_NAMESPACE::

#else

#define YSL_NAMESPACE YSL

#endif

//// YSL interfaces

namespace YSL_NAMESPACE
{

// inherit YAML namespace
using namespace YAML;

// threaded global format control, see @ref YAML::Emitter
enum class LoggerFormat
{
	Indent,
	PreCommentIndent,
	PostCommentIndent,
	FloatPrecision,
	DoublePrecision,
	// NumLoggerFormats,
};

// threaded incremental frame manipulator, an extension of YAML document
struct ThreadFrame
{
	thread_local static std::size_t Index;

	const std::string name;
	const std::size_t fill_width{25};

	explicit ThreadFrame(std::string rv_name) noexcept;
	ThreadFrame(std::string rv_name, std::size_t rv_fill_width) noexcept;
};

// the YSL logger class
class StreamLogger
{
	class SkipEmptyLogMessage : public google::LogMessage
	{
	public:
		template <typename... CArgs>
		explicit SkipEmptyLogMessage(CArgs... args)
			: google::LogMessage{std::forward<CArgs>(args)...}
		{
			reset();
		}

		~SkipEmptyLogMessage();

		bool empty_line(); // const

	protected:
		void reset();

	private:
		size_t m_init_count{};
	};

public:
	// threaded global format control
	static bool set_thread_format(EMITTER_MANIP value);
	static bool set_thread_format(LoggerFormat value, std::size_t n);

	// forward constructor
	template <typename... CArgs>
	explicit StreamLogger(CArgs... args)
		: m_message{new Reconstructable<SkipEmptyLogMessage>{std::forward<CArgs>(args)...}}
	{
		reset();
	}

	~StreamLogger();

	StreamLogger(const StreamLogger&) = delete;
	StreamLogger& operator=(const StreamLogger&) = delete;

	// forward YAML-type value
	template <typename T>
	inline StreamLogger& operator<<(const T& value)
	{
		m_implicit_eol = true;
		thread_emitter() << value; // throw ?
		return *this;
	}

	// threaded format control
	StreamLogger& operator<<(EMITTER_MANIP value);
	// ThreadFrame
	StreamLogger& operator<<(const ThreadFrame& value);

	// whether the end-of-line is implicit set by YAML internally
	inline bool is_implicit_eol() const
	{
		return m_implicit_eol;
	}

	// self call
	inline StreamLogger& self()
	{
		return *this;
	}

	// new glog message for new line if necessary
	void change_message();

protected:
	// internal stubs
	static Emitter&      thread_emitter();
	static std::ostream& thread_stream();

	void reset();

private:
	// use pointer for explicit destructor call
	Reconstructable<SkipEmptyLogMessage>* m_message{nullptr};
	bool                                  m_implicit_eol{};
};

// voidifier, see @ref google::LogMessageVoidify
// inline void operator&(std::nullptr_t, const StreamLogger&) {}
struct LoggerVoidify
{
	inline void operator&(const StreamLogger& /*logger*/)
	{
	}
};

} // namespace YSL_NAMESPACE

//// YSL macros, see @ref "glog/logging.h"

#ifndef YSL_NS_

namespace YSL_ = YSL; // prevent recursive macro expansion

#define YSL_NS_ YSL_::

#endif

#define YSL(severity) YSL_NS_ StreamLogger{__FILE__, __LINE__, google::GLOG_##severity}.self()
#define YSL_AT_LEVEL(severity) YSL_NS_ StreamLogger(__FILE__, __LINE__, severity).self()
#define YSL_TO_STRING(severity, message)                                                       \
	YSL_NS_ StreamLogger{__FILE__, __LINE__, google::GLOG_##severity,                          \
						 static_cast<std::string*>(message)}                                   \
			.self()
#define YSL_STRING(severity, outvec)                                                           \
	YSL_NS_ StreamLogger{__FILE__, __LINE__, google::GLOG_##severity,                          \
						 static_cast<std::vector<std::string>*>(outvec)}                       \
			.self()
#define YSL_TO_SINK(sink, severity)                                                            \
	YSL_NS_ StreamLogger{__FILE__, __LINE__, google::GLOG_##severity,                          \
						 static_cast<google::LogSink*>(sink), true}                            \
			.self()
#define YSL_TO_SINK_BUT_NOT_TO_LOGFILE(sink, severity)                                         \
	YSL_NS_ StreamLogger{__FILE__, __LINE__, google::GLOG_##severity,                          \
						 static_cast<google::LogSink*>(sink), false}                           \
			.self()

#define YSL_IF(severity, condition)                                                            \
	!(condition) ? (void) 0 : YSL_NS_ LoggerVoidify() & YSL(severity)
#define VYSL(verboselevel) YSL_IF(INFO, VLOG_IS_ON(verboselevel))
#define VYSL_IF(verboselevel, condition) YSL_IF(INFO, (condition) && VLOG_IS_ON(verboselevel))

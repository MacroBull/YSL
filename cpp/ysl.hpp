/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <iosfwd>
#include <utility>

#include <glog/logging.h>

#include "yaml-cpp/emitter.h"

#include "emitter_extra.hpp"
#include "reconstructable.hpp"
#include "stack_storage.hpp"

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
	const std::string name;
	const std::size_t fill_width{25};

	explicit ThreadFrame(std::string rv_name) noexcept;
	ThreadFrame(std::string rv_name, std::size_t rv_fill_width) noexcept;

	std::size_t index() const;
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
		: m_message{std::forward<CArgs>(args)...}
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

	// overload sequential
	template <typename... Args>
	StreamLogger& operator<<(const Sequential<Args...>& value)
	{
		value.print(self());
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
	stack_storage<Reconstructable<SkipEmptyLogMessage>> m_message;
	bool                                                m_implicit_eol{};
};

// voidifier, see @ref google::LogMessageVoidify
// inline void operator&(std::nullptr_t, const StreamLogger&) {}
struct LoggerVoidify
{
	inline void operator&(const StreamLogger& /*logger*/) {}
};

// RAII mapping scope
template <typename... End>
class Scope
{
public:
	template <typename... Begin>
	Scope(const char* file, int line, google::LogSeverity severity,
		  const Sequential<Begin...>& begin, Sequential<End...> end, bool enabled = true)
		: m_end{std::move(end)}
		, m_file{file}
		, m_line{line}
		, m_severity{severity}
		, m_enabled{enabled}
	{
		if (m_enabled)
		{
			m_logger.construct(file, line, severity);
			*m_logger << begin;
			m_logger.try_destruct();
		}
	}

	Scope(Scope&& xvalue) noexcept
		: m_end{std::move(xvalue.m_end)}
		, m_enabled{xvalue.m_enabled}
	{}

	Scope& operator=(const Scope& /*rvalue*/) = default;

	~Scope()
	{
		if (m_enabled)
		{
			m_logger.construct(m_file, m_line, m_severity);
			*m_logger << m_end;
			m_logger.try_destruct();
		}
	}

private:
	stack_storage<StreamLogger> m_logger;
	const Sequential<End...>    m_end;
	const char* const           m_file{};
	const int                   m_line{};
	const google::LogSeverity   m_severity{};
	const bool                  m_enabled{};
};

template <typename... Begin, typename... End>
Scope<End...>
make_stream_logging_scope(const char* file, int line, google::LogSeverity severity,
						  const Sequential<Begin...>& begin, const Sequential<End...>& end,
						  bool enabled = true)
{
	return Scope<End...>{file, line, severity, begin, end, enabled};
}

} // namespace YSL_NAMESPACE

//// YSL macros, see @ref "glog/logging.h"

#ifndef YSL_NS_

namespace YSL_ = YSL; // prevent recursive macro expansion

#define YSL_NS_ YSL_::

#endif

#define YSL(severity) YSL_NS_ StreamLogger{__FILE__, __LINE__, google::GLOG_##severity}.self()
#define YSL_AT_LEVEL(severity) YSL_NS_ StreamLogger(__FILE__, __LINE__, severity).self()

#define YSL_TO_STRING(severity, message)                                                       \
	YSL_NS_ StreamLogger{                                                                      \
			__FILE__, __LINE__, google::GLOG_##severity, static_cast<std::string*>(message)}   \
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
	!(condition) ? (void)0 : YSL_NS_ LoggerVoidify() & YSL(severity)
#define VYSL(verboselevel) YSL_IF(INFO, VLOG_IS_ON(verboselevel))
#define VYSL_IF(verboselevel, condition) YSL_IF(INFO, ((condition) && VLOG_IS_ON(verboselevel)))

#define YSL_SCOPE_(severity, ...)                                                              \
	auto LOG_EVERY_N_VARNAME(scope_, __LINE__)                                                 \
	{                                                                                          \
		YSL_NS_ make_stream_logging_scope(__FILE__, __LINE__, google::GLOG_##severity,         \
										  YSL_NS_ make_sequential(__VA_ARGS__),                \
										  YSL_NS_ make_sequential(YSL_NS_ EndMap))             \
	}
#define YSL_SCOPE(severity) YSL_SCOPE_(severity, YSL_NS_ BeginMap)
#define YSL_FSCOPE(severity, name)                                                             \
	YSL_SCOPE_(severity, YSL_NS_ ThreadFrame(name), YSL_NS_ BeginMap)
#define YSL_MSCOPE(severity, name)                                                             \
	YSL_SCOPE_(severity, YSL_NS_ Key, name, YSL_NS_ Value, YSL_NS_ Block, YSL_NS_ BeginMap)
#define YSL_CSCOPE(severity, name)                                                             \
	YSL_SCOPE_(severity, YSL_NS_ Key, name, YSL_NS_ Value, YSL_NS_ Flow, YSL_NS_ BeginMap)

#define VYSL_SCOPE_(verboselevel, ...)                                                         \
	auto LOG_EVERY_N_VARNAME(scope_, __LINE__)                                                 \
	{                                                                                          \
		YSL_NS_ make_stream_logging_scope(                                                     \
				__FILE__, __LINE__, google::GLOG_INFO, YSL_NS_ make_sequential(__VA_ARGS__),   \
				YSL_NS_ make_sequential(YSL_NS_ EndMap), VLOG_IS_ON(verboselevel))             \
	}
#define VYSL_SCOPE(verboselevel) VYSL_SCOPE_(verboselevel, YSL_NS_ BeginMap)
#define VYSL_FSCOPE(verboselevel, name)                                                        \
	VYSL_SCOPE_(verboselevel, YSL_NS_ ThreadFrame(name), YSL_NS_ BeginMap)
#define VYSL_MSCOPE(verboselevel, name)                                                        \
	VYSL_SCOPE_(verboselevel, YSL_NS_ Key, name, YSL_NS_ Value, YSL_NS_ Block, YSL_NS_ BeginMap)
#define VYSL_CSCOPE(verboselevel, name)                                                        \
	VYSL_SCOPE_(verboselevel, YSL_NS_ Key, name, YSL_NS_ Value, YSL_NS_ Flow, YSL_NS_ BeginMap)

#define YSL_INDEXED_(name, id) (std::string{name} + '[' + std::to_string(id) + ']')
#define YSL_IFSCOPE(severity, name, id) YSL_FSCOPE(severity, YSL_INDEXED_(name, id))
#define YSL_IMSCOPE(severity, name, id) YSL_MSCOPE(severity, YSL_INDEXED_(name, id))
#define YSL_ICSCOPE(severity, name, id) YSL_CSCOPE(severity, YSL_INDEXED_(name, id))
#define VYSL_IFSCOPE(severity, name, id) VYSL_FSCOPE(severity, YSL_INDEXED_(name, id))
#define VYSL_IMSCOPE(severity, name, id) VYSL_MSCOPE(severity, YSL_INDEXED_(name, id))
#define VYSL_ICSCOPE(severity, name, id) VYSL_CSCOPE(severity, YSL_INDEXED_(name, id))

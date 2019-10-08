/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <yaml-cpp/emitter.h>

#include <glog/logging.h>

#include "reconstructable.hpp"

#ifndef YSL_NAMESPACE

#define YSL_NAMESPACE YSL

#endif

namespace YSL_NAMESPACE
{

using namespace YAML;

struct ThreadFrame
{
	thread_local static std::size_t Index;

	const std::string name;
	const std::size_t fill_width{25};

	explicit ThreadFrame(std::string lv_name) noexcept : name{std::move(lv_name)}
	{
	}

	ThreadFrame(std::string lv_name, std::size_t lv_fill_width) noexcept
		: name{std::move(lv_name)}, fill_width{lv_fill_width}
	{
	}
};

enum class LoggerFormat
{
	Indent,
	PreCommentIndent,
	PostCommentIndent,
	FloatPrecision,
	DoublePrecision,
	// NumLoggerFormats,
};

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
	static bool set_thread_format(EMITTER_MANIP value);
	static bool set_thread_format(LoggerFormat value, std::size_t n);

	template <typename... CArgs>
	explicit StreamLogger(CArgs... args)
		: m_message{new Reconstructable<SkipEmptyLogMessage>{std::forward<CArgs>(args)...}}
	{
		reset();
	}

	~StreamLogger();

	StreamLogger(const StreamLogger&) = delete;
	StreamLogger& operator=(const StreamLogger&) = delete;

	template <typename T>
	StreamLogger& operator<<(const T& value)
	{
		m_implicit_eol = true;
		thread_emitter() << value; // throw ?
		return *this;
	}

	StreamLogger& operator<<(EMITTER_MANIP value);
	StreamLogger& operator<<(const ThreadFrame& value);

	inline bool is_implicit_eol() const
	{
		return m_implicit_eol;
	}

	inline StreamLogger& self()
	{
		return *this;
	}

	void change_message();

protected:
	static Emitter& thread_emitter();

	void reset();

private:
	// use pointer for explicit destructor call required
	Reconstructable<SkipEmptyLogMessage>* m_message{nullptr};
	bool                                  m_implicit_eol{};
};

// void operator&(std::nullptr_t, const StreamLogger&) {}

struct LoggerVoidify
{
	inline void operator&(const StreamLogger& /*logger*/)
	{
	}
};

} // namespace YSL_NAMESPACE

#define YSL(severity)                                                                          \
	YSL_NAMESPACE::StreamLogger(__FILE__, __LINE__, google::GLOG_##severity).self()
#define YSL_AT_LEVEL(severity) YSL_NAMESPACE::StreamLogger(__FILE__, __LINE__, severity).self()
#define YSL_TO_STRING(severity, message)                                                       \
	YSL_NAMESPACE::StreamLogger(__FILE__, __LINE__, google::GLOG_##severity,                   \
								static_cast<std::string*>(message))                                 \
			.self()
#define YSL_STRING(severity, outvec)                                                           \
	YSL_NAMESPACE::StreamLogger(__FILE__, __LINE__, google::GLOG_##severity,                   \
								static_cast<std::vector<std::string>*>(outvec))                \
			.self()
#define YSL_TO_SINK(sink, severity)                                                            \
	YSL_NAMESPACE::StreamLogger(__FILE__, __LINE__, google::GLOG_##severity,                   \
								static_cast<google::LogSink*>(sink), true)                     \
			.self()
#define YSL_TO_SINK_BUT_NOT_TO_LOGFILE(sink, severity)                                         \
	YSL_NAMESPACE::StreamLogger(__FILE__, __LINE__, google::GLOG_##severity,                   \
								static_cast<google::LogSink*>(sink), false)                    \
			.self()

#define YSL_IF(severity, condition)                                                            \
	!(condition) ? (void) 0 : YSL_NAMESPACE::LoggerVoidify() & YSL(severity)
#define VYSL(verboselevel) YSL_IF(INFO, VLOG_IS_ON(verboselevel))
#define VYSL_IF(verboselevel, condition) YSL_IF(INFO, (condition) && VLOG_IS_ON(verboselevel))

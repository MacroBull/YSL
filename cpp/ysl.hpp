/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <iomanip>
#include <iosfwd>
#include <utility>

#include <glog/logging.h>

#include "yaml-cpp/emitter.h"

#include "emitter_extra.hpp"
#include "reconstructable.hpp"
#include "stack_storage.hpp"

//// YSL configs

#ifdef YSL_NAMESPACE // namespace can be customized

#define YSL_NS YSL_NAMESPACE

#else

#define YSL_NS YSL

#endif

//// YSL interfaces

namespace YSL_NS
{

// inherit YAML namespace
using namespace YAML;

// inherit and extent std::to_string
using std::to_string;

inline std::string to_string(const std::string& value)
{
	return value;
}

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
	const std::size_t fill_width{30};
	const bool        reset{false};

	static std::size_t index();

	explicit ThreadFrame(std::string rv_name) noexcept;
	ThreadFrame(std::string rv_name, std::size_t rv_fill_width, bool rv_reset) noexcept;
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
		thread_emitter() << value;
		return *this;
	}

	// overload sequential
	template <typename... Args>
	inline StreamLogger& operator<<(const Sequential<Args...>& value)
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
		if (sizeof...(Begin) > 0 && m_enabled)
		{
			m_logger.construct(m_file, m_line, m_severity);
			*m_logger << begin;
			m_logger.try_destruct();
		}
	}

	~Scope()
	{
		exit();
	}

	Scope(const Scope&) = delete; // force move

	Scope(Scope&& xvalue) noexcept
		: m_end{std::move(xvalue.m_end)}
		, m_enabled{xvalue.m_enabled}
	{}

	Scope& operator=(const Scope&) = delete; // force move

	// make explicit enter call for stack clean up

	template <typename... Args>
	inline void enter(Args&&... args)
	{
		if (sizeof...(Args) > 0 && m_enabled)
		{
			m_logger.construct(m_file, m_line, m_severity);
			*m_logger << make_sequential(std::forward<Args>(args)...);
			m_logger.try_destruct();
		}
	}

protected:
	inline void exit()
	{
		if (sizeof...(End) > 0 && m_enabled)
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
inline Scope<End...>
make_stream_logging_scope(const char* file, int line, google::LogSeverity severity,
						  const Sequential<Begin...>& begin, const Sequential<End...>& end,
						  bool enabled = true)
{
	return Scope<End...>{file, line, severity, begin, end, enabled};
}

} // namespace YSL_NS

//// YSL macros, see LOG in @ref "glog/logging.h"

namespace YSL_ = YSL_NS; // prevent recursive macro expansion

#ifdef YSL_NAMESPACE

namespace YSL = YSL_NS; // define YSL

#else

#define YSL_NAMESPACE YSL_NS // define YSL_NAMESPACE

#endif

// log as YAML comment

#define LOGC(severity) LOG(severity) << "# "
#define LOGI(severity, indent)                                                                 \
	LOG(severity) << "# " << std::setw((indent) + 2) << std::setfill(' ') << ""
#define VLOGC(verboselevel) VLOG(verboselevel) << "# "
#define VLOGI(verboselevel, indent)                                                            \
	VLOG(verboselevel) << "# " << std::setw((indent) + 2) << std::setfill(' ') << ""

// YSL

#define YSL(severity) YSL_::StreamLogger{__FILE__, __LINE__, google::GLOG_##severity}.self()
#define YSL_AT_LEVEL(severity) YSL_::StreamLogger(__FILE__, __LINE__, severity).self()

#define YSL_TO_STRING(severity, message)                                                       \
	YSL_::StreamLogger{                                                                        \
			__FILE__, __LINE__, google::GLOG_##severity, static_cast<std::string*>(message)}   \
			.self()
#define YSL_STRING(severity, outvec)                                                           \
	YSL_::StreamLogger{__FILE__, __LINE__, google::GLOG_##severity,                            \
					   static_cast<std::vector<std::string>*>(outvec)}                         \
			.self()
#define YSL_TO_SINK(sink, severity)                                                            \
	YSL_::StreamLogger{__FILE__, __LINE__, google::GLOG_##severity,                            \
					   static_cast<google::LogSink*>(sink), true}                              \
			.self()
#define YSL_TO_SINK_BUT_NOT_TO_LOGFILE(sink, severity)                                         \
	YSL_::StreamLogger{__FILE__, __LINE__, google::GLOG_##severity,                            \
					   static_cast<google::LogSink*>(sink), false}                             \
			.self()

// YSL_IF, VYSL

#define YSL_IF(severity, condition)                                                            \
	!(condition) ? (void)0 : YSL_::LoggerVoidify() & YSL(severity)
#define VYSL(verboselevel) YSL_IF(INFO, VLOG_IS_ON(verboselevel))
#define VYSL_IF(verboselevel, condition) YSL_IF(INFO, (condition) && VLOG_IS_ON(verboselevel))

// scopes:
// - SCOPE: value-only mapping scope
// - FSCOPE: thread frame + mapping scope
// - MSCOPE: named mapping scope
// - CSCOPE: named flow mapping scope

#define YSL_SCOPE_(severity, ...)                                                              \
	YSL_::make_stream_logging_scope(__FILE__, __LINE__, google::GLOG_##severity,               \
									YSL_::make_sequential(__VA_ARGS__),                        \
									YSL_::make_sequential(YSL_::EndMap))
#define YSL_SCOPE_DECL_VAR(severity, ...)                                                      \
	auto LOG_EVERY_N_VARNAME(ysl_scope_, __LINE__) = YSL_SCOPE_(severity, __VA_ARGS__)
#define YSL_SCOPE(severity) YSL_SCOPE_DECL_VAR(severity, YSL_::BeginMap)
#define YSL_SCOPED(severity)                                                                   \
	YSL_SCOPE_DECL_VAR(severity, YSL_::BeginMap);                                              \
	YSL(severity)
#define YSL_FSCOPE(severity, name)                                                             \
	YSL_SCOPE_DECL_VAR(severity, YSL_::ThreadFrame(name), YSL_::BeginMap)
#define YSL_MSCOPE(severity, name)                                                             \
	YSL_SCOPE_DECL_VAR(severity, YSL_::Key, name, YSL_::Value, YSL_::Block, YSL_::BeginMap)
#define YSL_CSCOPE(severity, name)                                                             \
	YSL_SCOPE_DECL_VAR(severity, YSL_::Key, name, YSL_::Value, YSL_::Flow, YSL_::BeginMap)

#define VYSL_SCOPE_(verboselevel, ...)                                                         \
	YSL_::make_stream_logging_scope(                                                           \
			__FILE__, __LINE__, google::GLOG_INFO, YSL_::make_sequential(__VA_ARGS__),         \
			YSL_::make_sequential(YSL_::EndMap), VLOG_IS_ON(verboselevel))
#define VYSL_SCOPE_DECL_VAR(verboselevel, ...)                                                 \
	auto LOG_EVERY_N_VARNAME(ysl_scope_, __LINE__) = VYSL_SCOPE_(verboselevel, __VA_ARGS__)
#define VYSL_SCOPE(verboselevel) VYSL_SCOPE_DECL_VAR(verboselevel, YSL_::BeginMap)
#define VYSL_SCOPED(verboselevel)                                                              \
	VYSL_SCOPE_DECL_VAR(verboselevel, YSL_::BeginMap);                                         \
	VYSL(verboselevel)
#define VYSL_FSCOPE(verboselevel, name)                                                        \
	VYSL_SCOPE_DECL_VAR(verboselevel, YSL_::ThreadFrame(name), YSL_::BeginMap)
#define VYSL_MSCOPE(verboselevel, name)                                                        \
	VYSL_SCOPE_DECL_VAR(verboselevel, YSL_::Key, name, YSL_::Value, YSL_::Block, YSL_::BeginMap)
#define VYSL_CSCOPE(verboselevel, name)                                                        \
	VYSL_SCOPE_DECL_VAR(verboselevel, YSL_::Key, name, YSL_::Value, YSL_::Flow, YSL_::BeginMap)

// IxSCOPE: named scopes with indexed key

#define YSL_INDEXED_(name, id)                                                                 \
	(std::string{name}.append("[").append(YSL_::to_string(id)).append("]"))
#define YSL_IFSCOPE(severity, name, id) YSL_FSCOPE(severity, YSL_INDEXED_(name, id))
#define YSL_IMSCOPE(severity, name, id) YSL_MSCOPE(severity, YSL_INDEXED_(name, id))
#define YSL_ICSCOPE(severity, name, id) YSL_CSCOPE(severity, YSL_INDEXED_(name, id))
#define VYSL_IFSCOPE(verboselevel, name, id) VYSL_FSCOPE(verboselevel, YSL_INDEXED_(name, id))
#define VYSL_IMSCOPE(verboselevel, name, id) VYSL_MSCOPE(verboselevel, YSL_INDEXED_(name, id))
#define VYSL_ICSCOPE(verboselevel, name, id) VYSL_CSCOPE(verboselevel, YSL_INDEXED_(name, id))

// key-value by local incremental counter(occurrence)

#define YSL_LIC_VARNAME() LOG_EVERY_N_VARNAME(ysl_lic_, __LINE__)
#define YSL_LIC_DECL_VAR() static size_t YSL_LIC_VARNAME(){0};
#define YSL_LIC(severity, key)                                                                 \
	YSL_LIC_DECL_VAR();                                                                        \
	YSL(severity) << YSL_::Key << (key) << YSL_::Value << YSL_LIC_VARNAME()++
#define YSL_LIC_IF(severity, key, condition)                                                   \
	YSL_LIC_DECL_VAR();                                                                        \
	!(condition) ? (void)0                                                                     \
				 : YSL_::LoggerVoidify() & YSL(severity) << YSL_::Key << (key) << YSL_::Value  \
														 << YSL_LIC_VARNAME()++
#define VYSL_LIC(verboselevel, key) YSL_LIC_IF(INFO, key, VLOG_IS_ON(verboselevel))
#define VYSL_LIC_IF(verboselevel, key, condition)                                              \
	YSL_LIC_IF(INFO, key, (condition) && VLOG_IS_ON(verboselevel))

// DYSL

#define DLOG_(func) !(DCHECK_IS_ON()) ? (void)0 : google::LogMessageVoidify() & func
#define DYSL_(func) !(DCHECK_IS_ON()) ? (void)0 : YSL_::LoggerVoidify() & func
#define DLOGC DLOG_(LOGC)
#define DLOGI DLOG_(LOGI)
#define DVLOGC DLOG_(VLOGC)
#define DVLOGI DLOG_(VLOGI)
#define DYSL DYSL_(YSL)
#define DYSL_AT_LEVEL DYSL_(YSL_AT_LEVEL)
#define DYSL_IF DYSL_(YSL_IF)
#define DVYSL DYSL_(VYSL)
#define DVYSL_IF DYSL_(VYSL_IF)

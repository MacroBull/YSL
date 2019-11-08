/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <iomanip>
#include <mutex>

#include "ysl.hpp"

#ifdef YSL_PRIVATE_IMPL

#define YSL_IMPL_NS
#define YSL_IMPL_NS_
#define YSL_IMPL_STORAGE

#else

#define YSL_IMPL_NS detail
#define YSL_IMPL_NS_ detail::
#define YSL_IMPL_STORAGE inline

#endif

//// YSL implementations

namespace YSL_NAMESPACE
{

namespace YSL_IMPL_NS
{

class FilterForwardOutStream;
class FilterForwardOutStreamBuf : public std::streambuf
{
	const FilterForwardOutStream& m_parent;
	std::streambuf*               m_target{nullptr};
	bool                          m_dirty{false}, m_end_with_eol{false};

public:
	explicit FilterForwardOutStreamBuf(const FilterForwardOutStream& stream) noexcept
		: m_parent{stream}
		, m_target{nullptr}
		, m_dirty{false}
		, m_end_with_eol{false}
	{}

	FilterForwardOutStreamBuf(const FilterForwardOutStreamBuf& /* rvalue*/) = default;

	FilterForwardOutStreamBuf& operator=(const FilterForwardOutStreamBuf& /* rvalue*/) = delete;

	inline bool end_with_eol() const
	{
		return m_end_with_eol;
	}

	inline void reset(std::streambuf* const streambuf)
	{
		m_target       = streambuf;
		m_dirty        = false;
		m_end_with_eol = false;
	}

protected:
	int             overflow(int c) override;
	std::streamsize xsputn(const char* s, std::streamsize n) override;
};

class FilterForwardOutStream : public std::ostream
{
	StreamLogger*             m_parent{nullptr};
	FilterForwardOutStreamBuf m_streambuf;

public:
	FilterForwardOutStream() noexcept
		: m_parent{nullptr}
		, m_streambuf{*this}
	{
		rdbuf(&m_streambuf);
	}

	FilterForwardOutStream(const FilterForwardOutStream& /* rvalue*/) = delete;

	FilterForwardOutStream& operator=(const FilterForwardOutStream& /* rvalue*/) = delete;

	inline StreamLogger* parent() const
	{
		return m_parent;
	}

	inline void reset(StreamLogger* const parent, std::ostream& stream)
	{
		m_parent = parent;
		m_streambuf.reset(stream.rdbuf());
	}
};

YSL_IMPL_STORAGE int FilterForwardOutStreamBuf::overflow(int c)
{
	m_end_with_eol = c == '\n';

	if (m_end_with_eol)
	{
		const auto logger = m_parent.parent();
		if (logger->is_implicit_eol())
		{
			if (m_dirty)
			{
				m_end_with_eol = false;
				logger->change_message();
			}

			m_dirty = true;
			return c;
		}
	}

	m_dirty = true;
	return m_target->sputc(static_cast<char>(c));
}

YSL_IMPL_STORAGE std::streamsize
				 FilterForwardOutStreamBuf::xsputn(const char* s, std::streamsize n)
{
	std::streamsize ret{0};
	const auto      logger = m_parent.parent();
	auto            c      = s;

	for (std::streamsize i = 0; i < n; ++i)
	{
		m_end_with_eol = *c++ == '\n';

		if (m_end_with_eol && logger->is_implicit_eol())
		{
			ret += m_target->sputn(s, c - s - 1) + 1;
			s = c;

			if (m_dirty)
			{
				m_end_with_eol = false;
				logger->change_message();
			}
		}

		m_dirty = true;
	}

	ret += m_target->sputn(s, c - s);
	return ret;
}

} // namespace YSL_IMPL_NS

namespace detail
{

// static stubs

using log_level_t = decltype(FLAGS_minloglevel);

inline YSL_IMPL_NS_ FilterForwardOutStream& thread_stream()
{
	// HINT: destruct until the thread ends
	static thread_local FilterForwardOutStream ret{};
	return ret;
}

inline Emitter& thread_emitter()
{
	// HINT: destruct until the thread ends
	static thread_local Emitter ret{thread_stream()};
	return ret;
}

inline std::mutex& minloglevel_mutex()
{
	// HINT: static variable lifetime
	static std::mutex ret{};
	return ret;
}

// constify minloglevel, call this after glog initialized
inline log_level_t min_log_level()
{
	static const log_level_t ret{[]() {
		auto& mutex = minloglevel_mutex();
		mutex.lock();
		auto minloglevel = FLAGS_minloglevel;
		mutex.unlock();
		return minloglevel;
	}()};
	return ret;
}

inline std::size_t& thread_frame_index()
{
	static thread_local size_t ret{0};
	return ret;
}

} // namespace detail

namespace YSL_IMPL_NS
{

inline void bypass_glog_flush()
{
	detail::minloglevel_mutex().lock();
	FLAGS_minloglevel = google::NUM_SEVERITIES;
}

inline void restore_glog_state()
{
	FLAGS_minloglevel = detail::min_log_level();
	detail::minloglevel_mutex().unlock();
}

} // namespace YSL_IMPL_NS

YSL_IMPL_STORAGE std::size_t ThreadFrame::index()
{
	return detail::thread_frame_index();
}

YSL_IMPL_STORAGE ThreadFrame::ThreadFrame(std::string rv_name) noexcept
	: name{std::move(rv_name)}
{}

YSL_IMPL_STORAGE
ThreadFrame::ThreadFrame(std::string rv_name, std::size_t rv_fill_width) noexcept
	: name{std::move(rv_name)}
	, fill_width{rv_fill_width}
{}

YSL_IMPL_STORAGE StreamLogger::SkipEmptyLogMessage::~SkipEmptyLogMessage()
{
	if (empty_line())
	{
		YSL_IMPL_NS_ bypass_glog_flush();
	}
}

YSL_IMPL_STORAGE bool StreamLogger::SkipEmptyLogMessage::empty_line() // const
{
	auto buf = static_cast<google::base_logging::LogStreamBuf*>(stream().rdbuf());
	if (buf->pcount() == m_init_count)
	{
		return true;
	}

	auto impl_buf = static_cast<YSL_IMPL_NS_ FilterForwardOutStreamBuf*>(
			detail::thread_stream().rdbuf());
	if (buf->pcount() == m_init_count + 1 && impl_buf->end_with_eol())
	{
		return true;
	}

	return false;
}

YSL_IMPL_STORAGE void StreamLogger::SkipEmptyLogMessage::reset()
{
	auto buf     = static_cast<google::base_logging::LogStreamBuf*>(stream().rdbuf());
	m_init_count = buf->pcount();
	detail::min_log_level(); // ensure min_log_level initialized
}

YSL_IMPL_STORAGE bool StreamLogger::set_thread_format(EMITTER_MANIP value)
{
	auto& emitter = thread_emitter();
	return emitter.SetOutputCharset(value) || emitter.SetOutputCharset(value) ||
		   emitter.SetStringFormat(value) || emitter.SetBoolFormat(value) ||
		   emitter.SetIntBase(value) || emitter.SetSeqFormat(value) ||
		   emitter.SetMapFormat(value);
}

YSL_IMPL_STORAGE bool StreamLogger::set_thread_format(LoggerFormat value, std::size_t n)
{
	auto& emitter = thread_emitter();
	switch (value)
	{
		case LoggerFormat::Indent:
		{
			return emitter.SetIndent(n);
		}
		case LoggerFormat::PreCommentIndent:
		{
			return emitter.SetPreCommentIndent(n);
		}
		case LoggerFormat::PostCommentIndent:
		{
			return emitter.SetPostCommentIndent(n);
		}
		case LoggerFormat::FloatPrecision:
		{
			return emitter.SetFloatPrecision(n);
		}
		case LoggerFormat::DoublePrecision:
		{
			return emitter.SetDoublePrecision(n);
		}
		default:
		{
			return false;
		}
	}
}

YSL_IMPL_STORAGE StreamLogger::~StreamLogger()
{
	self() << Newline;
	//	m_implicit_eol = true;
	//	thread_emitter() << Newline; // throw !?
	m_message.try_destruct();
	YSL_IMPL_NS_ restore_glog_state();
}

YSL_IMPL_STORAGE StreamLogger& StreamLogger::operator<<(EMITTER_MANIP value)
{
	m_implicit_eol = value != Newline;
	thread_emitter() << value; // throw !?
	return *this;
}

YSL_IMPL_STORAGE StreamLogger& StreamLogger::operator<<(const ThreadFrame& value)
{
	m_implicit_eol = false;
	// self() << EndDoc;

	auto&      stream = thread_stream();
	const auto text{std::string{" "}
							.append(value.name)
							.append(": ")
							.append(std::to_string(detail::thread_frame_index()++))
							.append(" ")};

	stream << "--- # ";
	stream << std::setfill('-')
		   << std::setw(static_cast<int>(value.fill_width + text.size() / 2));
	stream << text;
	stream << std::setfill('-')
		   << std::setw(static_cast<int>(value.fill_width - text.size() / 2));
	stream << " # "; // ---\n

	self() << BeginDoc;
	return *this;
}

YSL_IMPL_STORAGE void StreamLogger::change_message()
{
	m_message->reconstruct();
	YSL_IMPL_NS_ restore_glog_state();
	reset();
}

YSL_IMPL_STORAGE Emitter& StreamLogger::thread_emitter()
{
	return detail::thread_emitter();
}

YSL_IMPL_STORAGE std::ostream& StreamLogger::thread_stream()
{
	return detail::thread_stream();
}

YSL_IMPL_STORAGE void StreamLogger::reset()
{
	detail::thread_stream().reset(this, m_message->stream());
}

} // namespace YSL_NAMESPACE

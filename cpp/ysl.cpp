/*

Copyright (c) 2019 Macrobull

*/

#include <iomanip>

//// YSL configs

#ifdef YSL_NAMESPACE // if YAML

#include "yaml-cpp/contrib/ysl.hpp"

#else

#include "ysl.hpp"

#endif

//// YSL implementations

namespace YSL_NAMESPACE
{

namespace
{

class FilterForwardOutStream;
class FilterForwardOutStreamBuf : public std::streambuf
{
	const FilterForwardOutStream& m_parent;
	std::streambuf*               m_target{nullptr};
	bool                          m_dirty{false}, m_end_with_eol{false};

public:
	explicit FilterForwardOutStreamBuf(const FilterForwardOutStream& stream) noexcept
		: m_parent{stream}, m_target{nullptr}, m_dirty{false}, m_end_with_eol{false}
	{
	}

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
	FilterForwardOutStream() noexcept : m_parent{nullptr}, m_streambuf{*this}
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

int FilterForwardOutStreamBuf::overflow(int c)
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

std::streamsize FilterForwardOutStreamBuf::xsputn(const char* s, std::streamsize n)
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

// threaded static stubs
thread_local FilterForwardOutStream      ThreadStream;
thread_local Emitter                     ThreadEmitter(ThreadStream);
thread_local decltype(FLAGS_minloglevel) Minloglevel;

inline void bypass_glog_flush()
{
	Minloglevel       = FLAGS_minloglevel;
	FLAGS_minloglevel = google::NUM_SEVERITIES;
}

inline void restore_glog_state()
{
	FLAGS_minloglevel = Minloglevel;
}

} // namespace

thread_local std::size_t ThreadFrame::Index{0};

ThreadFrame::ThreadFrame(std::string rv_name) noexcept : name{std::move(rv_name)}
{
}

ThreadFrame::ThreadFrame(std::string rv_name, std::size_t rv_fill_width) noexcept
	: name{std::move(rv_name)}, fill_width{rv_fill_width}
{
}

StreamLogger::SkipEmptyLogMessage::~SkipEmptyLogMessage()
{
	if (empty_line())
	{
		bypass_glog_flush();
	}
}

bool StreamLogger::SkipEmptyLogMessage::empty_line() // const
{
	auto buf = static_cast<google::base_logging::LogStreamBuf*>(stream().rdbuf());
	if (buf->pcount() == m_init_count)
	{
		return true;
	}

	auto impl_buf = static_cast<FilterForwardOutStreamBuf*>(ThreadStream.rdbuf());
	if (buf->pcount() == m_init_count + 1 && impl_buf->end_with_eol())
	{
		return true;
	}

	return false;
}

void StreamLogger::SkipEmptyLogMessage::reset()
{
	auto buf     = static_cast<google::base_logging::LogStreamBuf*>(stream().rdbuf());
	m_init_count = buf->pcount();
}

bool StreamLogger::set_thread_format(EMITTER_MANIP value)
{
	return ThreadEmitter.SetOutputCharset(value) || ThreadEmitter.SetOutputCharset(value) ||
		   ThreadEmitter.SetStringFormat(value) || ThreadEmitter.SetBoolFormat(value) ||
		   ThreadEmitter.SetIntBase(value) || ThreadEmitter.SetSeqFormat(value) ||
		   ThreadEmitter.SetMapFormat(value);
}

bool StreamLogger::set_thread_format(LoggerFormat value, std::size_t n)
{
	switch (value)
	{
		case LoggerFormat::Indent:
		{
			return ThreadEmitter.SetIndent(n);
		}
		case LoggerFormat::PreCommentIndent:
		{
			return ThreadEmitter.SetPreCommentIndent(n);
		}
		case LoggerFormat::PostCommentIndent:
		{
			return ThreadEmitter.SetPostCommentIndent(n);
		}
		case LoggerFormat::FloatPrecision:
		{
			return ThreadEmitter.SetFloatPrecision(n);
		}
		case LoggerFormat::DoublePrecision:
		{
			return ThreadEmitter.SetDoublePrecision(n);
		}
		default:
		{
			return false;
		}
	}
}

StreamLogger::~StreamLogger()
{
	self() << Newline;
	//	m_implicit_eol = true;
	//	ThreadEmitter << Newline; // throw ?
	delete m_message;
	m_message = nullptr;
	restore_glog_state();
}

StreamLogger& StreamLogger::operator<<(EMITTER_MANIP value)
{
	m_implicit_eol = value != Newline;
	ThreadEmitter << value; // throw !?
	return *this;
}

StreamLogger& StreamLogger::operator<<(const ThreadFrame& value)
{
	m_implicit_eol = false;
	// self() << EndDoc;

	const auto text = " " + value.name + ": " + std::to_string(ThreadFrame::Index++) + " ";
	ThreadStream << "--- # ";
	ThreadStream << std::setfill('-')
				 << std::setw(static_cast<int>(value.fill_width + text.size() / 2));
	ThreadStream << text;
	ThreadStream << std::setfill('-')
				 << std::setw(static_cast<int>(value.fill_width - text.size() / 2));
	ThreadStream << " # "; // ---\n

	self() << BeginDoc;
	return *this;
}

void StreamLogger::change_message()
{
	m_message->reconstruct();
	restore_glog_state();
	reset();
}

Emitter& StreamLogger::thread_emitter()
{
	return ThreadEmitter;
}

std::ostream& StreamLogger::thread_stream()
{
	return ThreadStream;
}

void StreamLogger::reset()
{
	ThreadStream.reset(this, m_message->stream());
}

} // namespace YSL_NAMESPACE

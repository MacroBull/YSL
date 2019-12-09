
#pragma once

#include <opencv2/core.hpp>

#include "emitter_extra.hpp"

namespace YAML
{

//// cv::String

Emitter& operator<<(Emitter& emitter, const cv::String& value);

#ifndef YAML_EMITTER_NO_CV_FORMATTER

//// cv::Mat

Emitter& operator<<(Emitter& emitter, const cv::Mat& value);

#endif

//// cv::Mat_

template <typename T>
Emitter& operator<<(Emitter& emitter, const cv::Mat_<T>& value)
{
	emitter << LocalTag("tensor");

#ifndef YAML_EMITTER_NO_CV_FORMATTER

	auto formatter = cv::Formatter::get(cv::Formatter::FMT_PYTHON);
	formatter->setMultiline(true);

#ifdef YSL_NAMESPACE // extension

	formatter->set32fPrecision(static_cast<int>(emitter.GetFloatPrecision()));
	formatter->set64fPrecision(static_cast<int>(emitter.GetDoublePrecision()));

#endif

	return detail::emit_streamable(
			emitter << Literal, cv::Formatter::get(cv::Formatter::FMT_PYTHON)->format(value));

#else

	emitter << BeginSeq;
	for (int i = 0; i < value.rows; ++i)
	{
		emitter << Flow << BeginSeq;
		for (int j = 0; j < value.cols; ++j)
		{
			emitter << detail::as_numeric(value(i, j));
		}
		emitter << EndSeq;
	}
	return emitter << EndSeq;

#endif
}

//// cv::Matx

template <typename T, int M, int N>
Emitter& operator<<(Emitter& emitter, const cv::Matx<T, M, N>& value)
{
	if (value.rows > 1 && value.cols > 1) // simplify vector representation
	{
		emitter << LocalTag("tensor");
	}
	else
	{
		emitter << Flow;
	}

	emitter << BeginSeq;
	for (int i = 0; i < value.rows; ++i)
	{
		emitter << Flow << BeginSeq;
		for (int j = 0; j < value.cols; ++j)
		{
			emitter << detail::as_numeric(value(i, j));
		}
		emitter << EndSeq;
	}
	return emitter << EndSeq;
}

//// cv::Point_

template <typename T>
inline Emitter& operator<<(Emitter& emitter, const cv::Point_<T>& value)
{
	emitter << Flow << BeginSeq;
	emitter << detail::as_numeric(value.x) << detail::as_numeric(value.y);
	return emitter << EndSeq;
}

//// cv::Point3_

template <typename T>
inline Emitter& operator<<(Emitter& emitter, const cv::Point3_<T>& value)
{
	emitter << Flow << BeginSeq;
	emitter << detail::as_numeric(value.x) << detail::as_numeric(value.y)
			<< detail::as_numeric(value.z);
	return emitter << EndSeq;
}

//// cv::Vec

template <typename T, int N>
Emitter& operator<<(Emitter& emitter, const cv::Vec<T, N>& value)
{
	emitter << Flow << BeginSeq;
	for (int i = 0; i < value.channels; ++i)
	{
		emitter << detail::as_numeric(value[i]);
	}
	return emitter << EndSeq;
}

//// cv::Vec

template <typename T>
Emitter& operator<<(Emitter& emitter, const cv::Scalar_<T>& value)
{
	emitter << Flow << BeginSeq;
	for (int i = 0; i < value.channels; ++i)
	{
		emitter << detail::as_numeric(value[i]);
	}
	return emitter << EndSeq;
}

//// cv::Size_

template <typename T>
inline Emitter& operator<<(Emitter& emitter, const cv::Size_<T>& value)
{
	emitter << Flow << BeginSeq;
	emitter << detail::as_numeric(value.width) << detail::as_numeric(value.height);
	return emitter << EndSeq;
}

//// cv::Rect_

template <typename T>
inline Emitter& operator<<(Emitter& emitter, const cv::Rect_<T>& value)
{
	emitter << Flow << BeginSeq;
	emitter << detail::as_numeric(value.x) << detail::as_numeric(value.y);
	emitter << detail::as_numeric(value.width) << detail::as_numeric(value.height);
	return emitter << EndSeq;
}

//// cv::Range

Emitter& operator<<(Emitter& emitter, /*const*/ cv::Range/*&*/ value);

//// cv::Complex

template <typename T>
inline Emitter& operator<<(Emitter& emitter, const cv::Complex<T>& value)
{
	return detail::emit_complex(emitter, value.re, value.im);
}

//// implementations

inline Emitter& operator<<(Emitter& emitter, const cv::String& value)
{
	return emitter << value.c_str();
}

#ifndef YAML_EMITTER_NO_CV_FORMATTER

inline Emitter& operator<<(Emitter& emitter, const cv::Mat& value)
{
	auto formatter = cv::Formatter::get(cv::Formatter::FMT_PYTHON);
	formatter->setMultiline(true);

#ifdef YSL_NAMESPACE // extension

	formatter->set32fPrecision(static_cast<int>(emitter.GetFloatPrecision()));
	formatter->set64fPrecision(static_cast<int>(emitter.GetDoublePrecision()));

#endif

	return detail::emit_streamable(
			emitter << LocalTag("tensor") << Literal,
			cv::Formatter::get(cv::Formatter::FMT_PYTHON)->format(value));
}

#endif

inline Emitter& operator<<(Emitter& emitter, /*const*/ cv::Range/*&*/ value)
{
	return emitter << Flow << BeginSeq << value.start << value.end << EndSeq;
}

} // namespace YAML

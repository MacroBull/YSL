
#pragma once

#if !defined(YAML_EMITTER_NO_COMPLEX) || !defined(YAML_EMITTER_NO_CV_FORMATTER)

#include <sstream>

#endif

#include <opencv2/core.hpp>

#include <yaml-cpp/emitter.h>

#include "emitter_traits.h"

namespace YAML
{

//// cv::String

Emitter& operator<<(Emitter& emitter, const cv::String& value)
{
	return emitter << value.c_str();
}

#ifndef YAML_EMITTER_NO_CV_FORMATTER

//// cv::Formatted as string literal, YAML format is bypassed

Emitter& operator<<(Emitter& emitter, const cv::Ptr<cv::Formatted> value)
{
	std::stringstream ss;
	ss << value;
	return emitter << Literal << ss.str();
}

//// cv::Mat

Emitter& operator<<(Emitter& emitter, const cv::Mat& value)
{
	auto formatter = cv::Formatter::get(cv::Formatter::FMT_PYTHON);
	// formatter->set32fPrecision(static_cast<int>(emitter.GetFloatPrecision()));
	// formatter->set64fPrecision(static_cast<int>(emitter.GetDoublePrecision()));
	formatter->setMultiline(true);
	return emitter << LocalTag("tensor") << formatter->format(value);
}

#endif

//// cv::Mat_

template <typename T>
Emitter& operator<<(Emitter& emitter, const cv::Mat_<T>& value)
{
#ifndef YAML_EMITTER_NO_CV_FORMATTER

	return emitter << LocalTag("tensor")
				   << cv::Formatter::get(cv::Formatter::FMT_PYTHON)->format(value);

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

template <typename T, int m, int n>
Emitter& operator<<(Emitter& emitter, const cv::Matx<T, m, n>& value)
{
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
Emitter& operator<<(Emitter& emitter, const cv::Point_<T>& value)
{
	emitter << Flow << BeginSeq;
	emitter << detail::as_numeric(value.x) << detail::as_numeric(value.y);
	return emitter << EndSeq;
}

//// cv::Point3_

template <typename T>
Emitter& operator<<(Emitter& emitter, const cv::Point3_<T>& value)
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
	for (int i = 0; i < N; ++i)
	{
		emitter << detail::as_numeric(value[i]);
	}
	return emitter << EndSeq;
}

//// cv::Size_

template <typename T>
Emitter& operator<<(Emitter& emitter, const cv::Size_<T>& value)
{
	emitter << Flow << BeginSeq;
	emitter << detail::as_numeric(value.width) << detail::as_numeric(value.height);
	return emitter << EndSeq;
}

//// cv::Rect_

template <typename T>
Emitter& operator<<(Emitter& emitter, const cv::Rect_<T>& value)
{
	emitter << Flow << BeginSeq;
	emitter << detail::as_numeric(value.x) << detail::as_numeric(value.y);
	emitter << detail::as_numeric(value.width) << detail::as_numeric(value.height);
	return emitter << EndSeq;
}

//// cv::Range

Emitter& operator<<(Emitter& emitter, const cv::Range& value)
{
	return emitter << Flow << BeginSeq << value.start << value.end << EndSeq;
}

//// cv::Complex

template <typename T>
inline Emitter& operator<<(Emitter& emitter, const cv::Complex<T>& value)
{
#ifndef YAML_EMITTER_NO_COMPLEX

	std::stringstream ss;
	ss << detail::as_numeric(value.re) << '+' << detail::as_numeric(value.im) << 'j';
	return emitter << LocalTag("complex") << ss.str();

#else

	return emitter << Flow << BeginSeq << value.re << value.im << EndSeq;

#endif
}

} // namespace YAML

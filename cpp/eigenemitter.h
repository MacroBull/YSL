
#pragma once

#if defined(YAML_EMITTER_USE_EIGEN_FORMATTER)

#include <sstream>

#endif

#include <Eigen/Core>

#include <yaml-cpp/emitter.h>

#include "emitter_traits.h"

namespace YAML
{

#ifdef YAML_EMITTER_USE_EIGEN_FORMATTER

//// Eigen::WithFormat

template <typename T>
Emitter& operator<<(Emitter& emitter, const Eigen::WithFormat<T>& value)
{
	std::stringstream ss;
	ss << value;
	return emitter << Literal << ss.str();
}

#endif

//// Eigen::DenseBase

template <typename T>
Emitter& operator<<(Emitter& emitter, const Eigen::DenseBase<T>& value)
{
#ifdef YAML_EMITTER_USE_EIGEN_FORMATTER

	Eigen::IOFormat format{Eigen::StreamPrecision, 0, ", ", "\n", "[", "]"};
	return emitter << LocalTag("tensor") << value.format(format);

#else

	const typename T::Nested matrix = value.eval();

	if (matrix.cols() <= 1)
	{
		emitter << Flow;
	}

	emitter << BeginSeq;
	for (Eigen::Index i = 0; i < matrix.rows(); ++i)
	{
		emitter << Flow << BeginSeq;
		for (Eigen::Index j = 0; j < matrix.cols(); ++j)
		{
			emitter << detail::as_numeric(matrix.coeff(i, j));
		}
		emitter << EndSeq;
	}
	return emitter << EndSeq;

#endif
}

} // namespace YAML

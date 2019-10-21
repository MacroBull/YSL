
#pragma once

#include <Eigen/Core>

#include "emitter_extra.hpp"

namespace YAML
{

//// Eigen::DenseBase

template <typename T>
Emitter& operator<<(Emitter& emitter, const Eigen::DenseBase<T>& value)
{
#ifdef YAML_EMITTER_USE_EIGEN_FORMATTER

	Eigen::IOFormat   format{Eigen::StreamPrecision, 0, ", ", "\n", "[", "]"};
	std::stringstream ss;

#ifdef YSL_NAMESPACE // extension

	emitter.SetStreamablePrecision<typename T::Scalar>(ss);

#endif

	return detail::emit_streamable(
			emitter << LocalTag("tensor") << Literal, value.format(format), &ss);

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
			emitter << detail::as_numeric(matrix(i, j));
		}
		emitter << EndSeq;
	}
	return emitter << EndSeq;

#endif
}

} // namespace YAML

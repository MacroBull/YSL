
#pragma once

#include <Eigen/Core>

#include "emitter_extra.hpp"

//// define YAML_DEF_EMIT_WITH_EIGEN_FORMATTER to enable Eigen formatter
////   see @ref https://eigen.tuxfamily.org/dox/structEigen_1_1IOFormat.html

// #define YAML_DEF_EMIT_WITH_EIGEN_FORMATTER

namespace YAML
{
namespace detail
{

//// Eigen::DenseCoeffsBase(readonly)

template <typename T>
struct generic_emitter<T, 4,
					   enable_if_t<std::is_base_of<
							   Eigen::DenseCoeffsBase<T, Eigen::ReadOnlyAccessors>, T>::value>>
{
	inline static Emitter& emit(Emitter& emitter, const T& value)
	{
#ifdef YAML_DEF_EMIT_WITH_EIGEN_FORMATTER

		Eigen::IOFormat   format(Eigen::StreamPrecision, 0, ", ", "\n", "[", "]");
		std::stringstream ss;

#ifdef YSL_NAMESPACE // extension

		emitter.SetStreamablePrecision<typename T::Scalar>(ss);

#endif

		return detail::emit_streamable(
				emitter << LocalTag("tensor") << Literal, value.format(format), &ss);

#else

		const auto matrix = value.eval();

		if (matrix.rows() > 1 && matrix.cols() > 1) // simplify vector representation
		{
			emitter << LocalTag("tensor");
		}
		else
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
};

} // namespace detail
} // namespace YAML

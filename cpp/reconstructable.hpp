/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <cassert>
#include <tuple>
#include <type_traits>

//// hepler traits

template <bool P, typename T = void>
using enable_if_t = typename std::enable_if<P, T>::type;

//// reconstruct function

template <typename T, typename... CArgs>
inline T* reconstruct(T& target, CArgs... args)
{
	target.~T();
	return new (&target) T{std::forward<CArgs>(args)...};
}

//// ReconstructorBase: base class

template <typename T>
struct ReconstructorBase
{
	virtual ~ReconstructorBase()     = default;
	virtual T* reconstruct(T&) const = 0;
};

//// ReconstructorImpl: @ref ReconstructorBase implementation with tupled parameters

template <typename T, typename... Params>
struct ReconstructorImpl : ReconstructorBase<T>
{
	/*const*/ std::tuple<Params...> params;

	template <typename... CArgs>
	explicit ReconstructorImpl(CArgs... args) noexcept
		: params{std::move(std::make_tuple(std::forward<CArgs>(args)...))}
	{
	}

	explicit ReconstructorImpl(std::tuple<Params...> rv_params) noexcept
		: params{std::move(rv_params)}
	{
	}

	inline T* reconstruct(T& target) const override
	{
		return reconstruct_impl(target);
	}

protected:
	inline T* reconstruct_impl(T& target, Params... args) const
	{
		return ::reconstruct(target, std::forward<Params>(args)...);
	}

	template <typename... Args>
	inline enable_if_t<sizeof...(Args) != sizeof...(Params), T*>
	reconstruct_impl(T& target, Args&&... args) const
	{
		return reconstruct_impl(
				target, std::forward<Args>(args)..., std::get<sizeof...(Args)>(params));
	}
};

//// ReconstructableImpl: attach reconstructor to a class

template <typename T>
class ReconstructableImpl : public T
{
public:
	template <typename... CArgs>
	explicit ReconstructableImpl(bool reset, CArgs... args)
		: T{std::forward<CArgs>(args)...}
		, m_reconstructor{reset ? nullptr
								: new ReconstructorImpl<ReconstructableImpl, bool, CArgs...>{
										  true, std::forward<CArgs>(args)...}}
	{
	}

	~ReconstructableImpl()
	{
		delete m_reconstructor;
		m_reconstructor = nullptr;
	}

	ReconstructableImpl(const ReconstructableImpl& /*rvalue*/) = default;

	ReconstructableImpl& operator=(const ReconstructableImpl& /*rvalue*/) = default;

	inline void reconstruct()
	{
		assert(m_reconstructor != nullptr);
		auto reconstructor = m_reconstructor;
		m_reconstructor    = nullptr;
		reconstructor->reconstruct(*this);
		assert(m_reconstructor == nullptr);
		m_reconstructor = reconstructor;
	}

private:
	const ReconstructorBase<ReconstructableImpl>* m_reconstructor{nullptr};
};

//// Reconstructable: purify arguments of @ref ReconstructableImpl

template <typename T>
struct Reconstructable : ReconstructableImpl<T>
{
	template <typename... CArgs>
	explicit Reconstructable(CArgs... args)
		: ReconstructableImpl<T>{false, std::forward<CArgs>(args)...}
	{
	}
};

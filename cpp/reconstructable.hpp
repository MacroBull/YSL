/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <tuple>
#include <utility>

#include <cassert>

//// hepler traits

template <bool P, typename T = void>
using enable_if_t = typename std::enable_if<P, T>::type;

//// reconstruct function

template <typename T, typename... CArgs>
inline T& reconstruct(T& target, CArgs... args)
{
	target.~T();
	new (&target) T{std::forward<CArgs>(args)...};
	return target;
}

//// ReconstructorBase: base class

template <typename T>
struct ReconstructorBase
{
	ReconstructorBase()          = default;
	virtual ~ReconstructorBase() = default;

	ReconstructorBase(const ReconstructorBase&) = default;

	ReconstructorBase& operator=(const ReconstructorBase&) = default;

	virtual T& construct(T&) const   = 0;
	virtual T& reconstruct(T&) const = 0;
};

//// ReconstructorImpl: @ref ReconstructorBase implementation with tupled parameters

template <typename T, typename... Params>
struct ReconstructorImpl : ReconstructorBase<T>
{
	/*const*/ std::tuple<Params...> params; // editable

	template <typename... CArgs>
	explicit ReconstructorImpl(CArgs... args) noexcept
		: params{std::move(std::make_tuple(std::forward<CArgs>(args)...))}
	{}

	explicit ReconstructorImpl(std::tuple<Params...> rv_params) noexcept
		: params{std::move(rv_params)}
	{}

	inline T& construct(T& target) const override
	{
		return construct_impl(target);
	}

	inline T& reconstruct(T& target) const override
	{
		target.~T();
		return construct_impl(target);
	}

protected:
	inline T& construct_impl(T& target, Params... args) const
	{
		new (&target) T{std::forward<Params>(args)...};
		return target;
	}

	template <typename... Args>
	inline enable_if_t<sizeof...(Args) != sizeof...(Params), T&>
	construct_impl(T& target, Args&&... args) const
	{
		return construct_impl(
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
	{}

	~ReconstructableImpl()
	{
		delete m_reconstructor;
		m_reconstructor = nullptr;
	}

	ReconstructableImpl(const ReconstructableImpl&) = delete; // default = delete

	ReconstructableImpl& operator=(const ReconstructableImpl&) = delete; // default = delete

	inline void reconstruct();

private:
	const ReconstructorBase<ReconstructableImpl>* m_reconstructor{nullptr};
};

//// Reconstructable: purify arguments of @ref ReconstructableImpl

template <typename T>
struct Reconstructable final : ReconstructableImpl<T> // final inheritance
{
	template <typename... CArgs>
	explicit Reconstructable(CArgs... args)
		: ReconstructableImpl<T>{false, std::forward<CArgs>(args)...}
	{}
};

template <typename T>
void ReconstructableImpl<T>::reconstruct()
{
	assert(m_reconstructor != nullptr && "reconstruct without reconstructor");

	auto reconstructor = m_reconstructor;
	m_reconstructor    = nullptr;
	reconstructor->reconstruct(*this);
	assert(m_reconstructor == nullptr);

	m_reconstructor = reconstructor;
}

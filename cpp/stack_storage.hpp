/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <memory>
#include <utility>

#include <cassert>

////

template <typename T>
class StackStorage
{
public:
	// StackStorage() {}
	StackStorage() = default;

	template <typename... CArgs>
	explicit StackStorage(CArgs... args)
	{
		construct(std::forward<CArgs>(args)...);
	}

	template <typename U>
	explicit StackStorage(std::initializer_list<U>&& args)
	{
		construct(std::forward<std::initializer_list<U>>(args));
	}

	virtual ~StackStorage()
	{
		try_destruct();
	}

	StackStorage(const StackStorage&) = default;

	StackStorage& operator=(const StackStorage&) = default;

	inline T& operator*()
	{
		check_inited();
		return *m_pointer;
	}

	inline T* operator->()
	{
		check_inited();
		return m_pointer;
	}

	inline bool inited() const noexcept
	{
		return m_pointer != nullptr;
	}

	template <typename... CArgs>
	inline T& construct(CArgs... args)
	{
		try_destruct();
		new (m_storage) T(std::move(args)...);
		m_pointer = reinterpret_cast<T*>(m_storage);
		return *m_pointer;
	}

	inline bool try_destruct() noexcept
	{
		if (m_pointer != nullptr)
		{
			m_pointer->~T();
			m_pointer = nullptr;
			return true;
		}

		return false;
	}

protected:
	inline void check_inited()
	{
		assert(m_pointer != nullptr && "accessing unconstructed storage");
	}

private:
	//	union
	//	{
	//		T m_payload;
	//	};
	alignas(alignof(T)) char m_storage[sizeof(T)]; // {} leave uninitialized
	T* m_pointer{nullptr};
};

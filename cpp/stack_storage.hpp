/*

Copyright (c) 2019 Macrobull

*/

#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include <cassert>

////

template <typename T>
class stack_storage
{
public:
	stack_storage() {}

	template <typename... CArgs>
	explicit stack_storage(CArgs... args)
	{
		construct(std::forward<CArgs>(args)...);
	}

	stack_storage(const stack_storage&) = default;

	virtual ~stack_storage()
	{
		try_destruct();
	}

	stack_storage& operator=(const stack_storage&) = default;

	inline T& operator*()
	{
		return get_payload();
	}

	inline T* operator->()
	{
		return &get_payload();
	}

	inline bool inited() const
	{
		return m_pointer != nullptr;
	}

	template <typename... CArgs>
	inline T& construct(CArgs... args)
	{
		try_destruct();
		new (m_storage) T{std::move(args)...};
		m_pointer = reinterpret_cast<T*>(m_storage);
		return *m_pointer;
	}

	inline bool try_destruct()
	{
		if (m_pointer != nullptr)
		{
			m_pointer->~T(); // noexcept
			m_pointer = nullptr;
			return true;
		}

		return false;
	}

protected:
	inline T& get_payload()
	{
		assert(inited() && "accessing unconstructed storage");

		return *m_pointer;
	}

private:
	union
	{
		// T m_payload;
		char m_storage[sizeof(T)];
	};

	T* m_pointer{nullptr};
};

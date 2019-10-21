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

	stack_storage(const stack_storage& rvalue) = default;

	~stack_storage()
	{
		try_destruct();
	}

	stack_storage& operator=(const stack_storage& rvalue) = default;

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
		return m_inited;
	}

	template <typename... CArgs>
	inline T& construct(CArgs... args)
	{
		try_destruct();
		new (std::addressof(m_payload)) T{std::move(args)...};
		m_inited = true;
		return m_payload;
	}

	inline bool try_destruct()
	{
		if (m_inited)
		{
			m_payload.~T(); // noexcept
			m_inited = false;
			return true;
		}

		return false;
	}

protected:
	inline T& get_payload()
	{
		assert(m_inited && "accessing unconstructed storage");

		return m_payload;
	}

private:
	union
	{
		T m_payload;
	};

	bool m_inited{false};
};

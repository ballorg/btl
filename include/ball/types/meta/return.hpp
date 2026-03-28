#ifndef _INCLUDE_BALL_TYPES_META_RETURN_HPP_
#	define _INCLUDE_BALL_TYPES_META_RETURN_HPP_

#	pragma once

template < typename R >
struct MReturn
{
	static constexpr R Default()
	{
		return R();
	}
};

template <>
struct MReturn< void >
{
	static constexpr void Default() noexcept
	{
	}
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_RETURN_HPP_ )

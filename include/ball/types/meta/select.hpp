#ifndef _INCLUDE_BALL_TYPES_META_SELECT_HPP_
#	define _INCLUDE_BALL_TYPES_META_SELECT_HPP_

// Select between aliases that extract either their first or second parameter
template < bool >
struct MSelect
{
	template < class T, class T2 >
	using Apply_t = T;
};
template <>
struct MSelect< false >
{
	template < class T, class T2 >
	using Apply_t = T2;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_SELECT_HPP_ )

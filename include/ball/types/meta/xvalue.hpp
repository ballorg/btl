#ifndef _INCLUDE_BALL_TYPES_XVALUE_HPP_
#	define _INCLUDE_BALL_TYPES_XVALUE_HPP_

#	include "removereference.hpp"

template < class T >
constexpr MRemoveReference< T >::Type &&Move( T &&obj ) noexcept
{
	return static_cast< MRemoveReference< T >::Type &&>( obj );
}

template < class T >
constexpr T &&Forward( typename MRemoveReference< T >::Type &obj ) noexcept
{
	return static_cast< T && >( obj );
}

template < class T >
constexpr T &&Forward( typename MRemoveReference< T >::Type &&obj ) noexcept
{
	return static_cast< T && >( obj );
}

// Swap two of anything.
template < class T >
constexpr void Swap( T &x, T &y ) noexcept
{
	T temp = x;

	x = y;
	y = Move( temp );
}

template < class T, size_t N >
constexpr void Swap( T ( &x )[ N ], T ( &y )[ N ] ) noexcept
{
	T temp[ N ] = x;

	x = y;
	y = Move( temp );
}

#endif // !defined( _INCLUDE_BALL_TYPES_XVALUE_HPP_ )

#ifndef _INCLUDE_BALL_TYPES_XVALUE_HPP_
#	define _INCLUDE_BALL_TYPES_XVALUE_HPP_

#	include "meta/removereference.hpp"

template < class T >
inline MRemoveReference< T >::Type &&Move( T &&obj ) noexcept
{
	return static_cast< MRemoveReference< T >::Type &&>( obj );
}

template < class T >
inline T &&Forward( typename MRemoveReference< T >::Type &obj ) noexcept
{
	return static_cast< T && >( obj );
}

template < class T >
inline T &&Forward( typename MRemoveReference< T >::Type &&obj ) noexcept
{
	return static_cast< T && >( obj );
}

#endif // !defined( _INCLUDE_BALL_TYPES_XVALUE_HPP_ )

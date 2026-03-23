#ifndef _INCLUDE_BALL_TYPES_META_REMOVEREFERENCE_HPP_
#	define _INCLUDE_BALL_TYPES_META_REMOVEREFERENCE_HPP_

#	pragma once

template < typename T > struct MRemoveReference { using Type = T; };
template < typename T > struct MRemoveReference< T & > { using Type = T; };
template < typename T > struct MRemoveReference< T && > { using Type = T; };

template < typename T > using RemoveReference_t = MRemoveReference< T >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_REMOVEREFERENCE_HPP_ )

#ifndef _INCLUDE_BALL_TYPES_META_REMOVEREFERENCE_HPP_
#	define _INCLUDE_BALL_TYPES_META_REMOVEREFERENCE_HPP_

template < class T > struct MRemoveReference { using Type = T; };
template < class T > struct MRemoveReference< T & > { using Type = T; };
template < class T > struct MRemoveReference< T && > { using Type = T; };

#endif // !defined( _INCLUDE_BALL_TYPES_META_REMOVEREFERENCE_HPP_ )

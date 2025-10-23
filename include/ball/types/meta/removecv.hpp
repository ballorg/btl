#ifndef _INCLUDE_BALL_TYPES_META_REMOVECV_HPP_
#	define _INCLUDE_BALL_TYPES_META_REMOVECV_HPP_

#define BALL_META_SPECIFIER( name, ... ) \
	template < typename T > \
	struct name \
	{ \
		using Type = T; \
		__VA_ARGS__; \
	};

// RemoveCV_t.
#define BALL_META_SPECIFIER_REMOVE_CV() \
	BALL_META_SPECIFIER( MRemoveCV, template < template < class > class F > using Apply_t = F< T > )
#define BALL_META_SPECIFIER_REMOVE_CV_( topmost ) \
	BALL_META_SPECIFIER( MRemoveCV< topmost T >, template < template < class > class F > using Apply_t = topmost F< T > )

BALL_META_SPECIFIER_REMOVE_CV();
BALL_META_SPECIFIER_REMOVE_CV_( const );
BALL_META_SPECIFIER_REMOVE_CV_( volatile );
BALL_META_SPECIFIER_REMOVE_CV_( const volatile );
template < typename T > using RemoveCV_t = typename MRemoveCV< T >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_REMOVECV_HPP_ )

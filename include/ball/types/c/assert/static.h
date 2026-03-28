#ifndef _INCLUDE_BALL_TYPES_C_ASSERT_STATIC_H_
#	define _INCLUDE_BALL_TYPES_C_ASSERT_STATIC_H_

#	define BALL_STATIC_ASSERT_JOIN_IMPL( a, b ) a##b
#	define BALL_STATIC_ASSERT_JOIN( a, b ) BALL_STATIC_ASSERT_JOIN_IMPL( a, b )

#	ifndef BALL_STATIC_ASSERT
#		if defined( __cplusplus ) && ( __cplusplus >= 201103L || ( defined( _MSVC_LANG ) && _MSVC_LANG >= 201103L ) )
#			define BALL_STATIC_ASSERT( expression, msg ) static_assert( expression, msg )
#		elif defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 201112L
#			define BALL_STATIC_ASSERT( expression, msg ) _Static_assert( expression, msg )
#		else
#			define BALL_STATIC_ASSERT( expression, msg ) typedef char BALL_STATIC_ASSERT_JOIN( static_assert_, __LINE__ )[ ( expression ) ? 1 : -1 ]
#		endif
#	endif

#	undef BALL_STATIC_ASSERT_JOIN
#	undef BALL_STATIC_ASSERT_JOIN_IMPL

#endif // !defined( _INCLUDE_BALL_TYPES_C_ASSERT_STATIC_H_ )

#ifndef _INCLUDE_BALL_TYPES_C_ASSERT_HPP_
#	define _INCLUDE_BALL_TYPES_C_ASSERT_HPP_

#	include "debugbreak.h"
#	include "macros.h"

#	ifndef BALL_ENABLE_ASSERT
#		if defined( NDEBUG )
#			define BALL_ENABLE_ASSERT 0
#		else // !defined( NDEBUG )
#			define BALL_ENABLE_ASSERT 1
#		endif // defined( NDEBUG )
#	endif // !defined( BALL_ENABLE_ASSERT )

#	if BALL_ENABLE_ASSERT
#		define BALL_ASSERT_IMPL( func, expr, message, name, filename, line, column ) \
			{ \
				if ( !( expr ) ) \
				{ \
					func( #expr, message, name, filename, ( unsigned int )( line ), ( unsigned int )( column ) ); \
					BALL_DEBUGBREAK(); \
				} \
			}
#	else // !BALL_ENABLE_ASSERT
#		define BALL_ASSERT_IMPL( func, expr, message, name, filename, line, column ) ( ( void )0 );
#	endif // BALL_ENABLE_ASSERT
#	define BALL_ASSERT_MESSAGE( expr, message ) BALL_ASSERT_IMPL( Ball_AssertFail, expr, message, __FUNCTION__, __FILE__, __LINE__, 0 )
#	define BALL_ASSERT( expr ) BALL_ASSERT_MESSAGE( expr, BALL_NULL )
#	define BALL_ASSERT_IF_MESSAGE( expr, message ) BALL_ASSERT_IMPL( Ball_AssertFail, !( expr ), message, __FUNCTION__, __FILE__, __LINE__, 0 ) if( expr )
#	define BALL_ASSERT_IF( expr ) BALL_ASSERT_IF_MESSAGE( expr, BALL_NULL )

//TODO
#	define BALL_FATAL_MESSAGE( expr, message ) ( ( void )0 )
#	define BALL_FATAL( expr ) ( ( void )0 )

// BALL_ASSUME(x)
//  - Optimization hint to the compiler that x is always true in Release.
//  - In GCC/Clang we route false-case into UB to enable more aggressive opts.
#if defined( _MSC_VER )
#	define BALL_ASSUME( x ) __assume( x )
#elif defined( __GNUC__ ) || defined( __clang__ )
#	define BALL_ASSUME( x ) do { if ( !(x) ) __builtin_unreachable(); } while ( 0 )
#else
#	define BALL_ASSUME( x ) ( ( void )0 )
#endif

BALL_EXTERN_C void Ball_AssertFail(
	const char *pszExpression,
	const char *pszMessage,
	const char *pszName,
	const char *pszFile,
	unsigned int nLine,
	unsigned int nColumn
);

#endif // !defined( _INCLUDE_BALL_TYPES_C_ASSERT_HPP_ )

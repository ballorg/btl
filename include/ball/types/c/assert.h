#ifndef _INCLUDE_BALL_TYPES_C_ASSERT_H_
#	define _INCLUDE_BALL_TYPES_C_ASSERT_H_

#	include "debugbreak.h"
#	include "macros.h"
#	include "unreachable.h"

#	include "assert/static.h"

#	ifndef BALL_ENABLE_ASSERT
#		if defined( NDEBUG )
#			define BALL_ENABLE_ASSERT 0
#		else // !defined( NDEBUG )
#			define BALL_ENABLE_ASSERT 1
#		endif // defined( NDEBUG )
#	endif // !defined( BALL_ENABLE_ASSERT )

#	define BALL_THROW_IMPL( func, sig, expr, message, name, filename, line, column ) \
		{ \
			if ( !( expr ) ) \
			{ \
				func( #expr, message, name, filename, ( unsigned int )( line ), ( unsigned int )( column ) ); \
				sig(); \
			} \
		}

#	if BALL_ENABLE_ASSERT
#		define BALL_ASSERT_IMPL( expr, message, name, filename, line, column ) \
		BALL_THROW_IMPL( Ball_AssertFail, BALL_DEBUGBREAK, expr, message, name, filename, line, column );
#	else // !BALL_ENABLE_ASSERT
#		define BALL_ASSERT_IMPL( expr, message, name, filename, line, column ) ( ( void )0 );
#	endif // BALL_ENABLE_ASSERT

#	define BALL_ASSERT_MESSAGE( expr, message ) BALL_ASSERT_IMPL( expr, message, __FUNCTION__, __FILE__, __LINE__, 0 )
#	define BALL_ASSERT( expr ) BALL_ASSERT_MESSAGE( expr, BALL_NULL )
#	define BALL_ASSERT_IF_MESSAGE( expr, message ) BALL_ASSERT_IMPL( !( expr ), message, __FUNCTION__, __FILE__, __LINE__, 0 ) if( expr )
#	define BALL_ASSERT_IF( expr ) BALL_ASSERT_IF_MESSAGE( expr, BALL_NULL )

#	define BALL_FATAL_IMPL( expr, message, name, filename, line, column ) \
		BALL_THROW_IMPL( Ball_AssertFail, BALL_UNREACHABLE, expr, message, name, filename, line, column );
#	define BALL_FATAL_MESSAGE( expr, message ) BALL_FATAL_IMPL( expr, message, __FUNCTION__, __FILE__, __LINE__, 0 )
#	define BALL_FATAL( expr ) BALL_FATAL_MESSAGE( expr, BALL_NULL )

BALL_EXTERN_C void Ball_AssertFail(
	const char *pszExpression,
	const char *pszMessage,
	const char *pszName,
	const char *pszFile,
	unsigned int nLine,
	unsigned int nColumn
);

#endif // !defined( _INCLUDE_BALL_TYPES_C_ASSERT_H_ )

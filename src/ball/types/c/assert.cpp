#include <ball/types/c/assert.h>
#include <ball/types/c/debugbreak.h>
#include <ball/types/c/io.h>

#ifdef BALL_ENABLE_MODULES
import Ball.New;
import Ball.Types;
#else // !defined( BALL_ENABLE_MODULES )
#	include <ball/new.hpp>
#	include <ball/types.hpp>
#endif // defined( BALL_ENABLE_MODULES )

using namespace Ball::Types;

void Ball_AssertFail(
	const char *pszExpression,
	const char *pszMessage,
	const char *pszName,
	const char *pszFile,
	unsigned int nLine,
	unsigned int nColumn
)
{
	BufferString_t< 1000 > sMessage;

	if ( pszExpression  != nullptr )
	{
		sMessage.AppendMultiple( "Assertion failed: ", pszExpression, "\n" );
	}

	// Message (if available)
	if ( pszMessage != nullptr )
	{
		sMessage.AppendMultiple( "Message: ", pszMessage, "\n" );
	}

	// Defintion name (if available)
	if ( pszName != nullptr )
	{
		sMessage.AppendMultiple( "Name: ", pszName, "\n" );
	}

	// Location (file:line)
	if ( pszFile != nullptr )
	{
		sMessage.AppendMultiple( "Localtion: ", pszFile );

		if ( nLine )
		{
			sMessage.AppendMultiple( ":", nLine );
		}

		if ( nColumn )
		{
			sMessage.AppendMultiple( ":", nColumn );
		}

		sMessage += '\n';
	}

	sMessage += '\0';

	// Emit and break
	const char *pszOut = sMessage.String();

	puts( pszOut );
	BALL_DEBUGBREAK();
}

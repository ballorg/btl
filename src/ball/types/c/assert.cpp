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

using namespace BTL;

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
		sMessage.AppendMultiple( "Expression failed: ", StringView_t( pszExpression ), "\n" );
	}

	// Message (if available)
	if ( pszMessage != nullptr )
	{
		sMessage.AppendMultiple( "Message: ", StringView_t( pszMessage ), "\n" );
	}

	// Definition name (if available)
	if ( pszName != nullptr )
	{
		sMessage.AppendMultiple( "Name: ", StringView_t( pszName ), "\n" );
	}

	// Location (file:line)
	if ( pszFile != nullptr )
	{
		sMessage.AppendMultiple( "Localtion: ", StringView_t( pszFile ) );

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
}

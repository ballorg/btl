#ifndef _INCLUDE_BALL_TYPES_BASE_FIXED_H_
#	define _INCLUDE_BALL_TYPES_BASE_FIXED_H_

#if defined( _WIN64 )
#	include "fixed/win64.h"
#elif defined( _WIN32 )
#	include "fixed/win32.h"
#elif defined( _LP32 ) || defined( __LP32__ )
#	include "fixed/lp32.h"
#elif defined( _ILP32 ) || defined( __ILP32__ )
#	include "fixed/ilp32.h"
#elif defined( _LP64 ) || defined( __LP64__ )
#	include "fixed/lp64.h"
#elif defined( _LLP64 ) || defined( __LLP64__ )
#	include "fixed/llp64.h"
#endif

#endif // !defined( _INCLUDE_BALL_TYPES_BASE_FIXED_H_ )

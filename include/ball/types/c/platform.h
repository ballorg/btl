#ifndef _INCLUDE_BALL_TYPES_C_PLATFORM_H_
#	define _INCLUDE_BALL_TYPES_C_PLATFORM_H_

#	ifdef __cplusplus
#		define BALL_CXX 1
#	endif

#	if defined( __GNUC__ )
#		define BALL_GNUC 1
#	endif

#	if defined( __clang__ )
#		define BALL_CLANG 1
#	endif

#	if defined( __apple_build_version__ )
#		define BALL_APPLE_CLANG 1
#	endif

#	if defined( __MINGW32__ ) || defined( __MINGW64__ )
#		define BALL_MINGW 1
#	elif defined( _MSC_VER )
#		define BALL_MSVC 1
#	elif defined( BALL_GNUC ) && !defined( BALL_CLANG )
#		define BALL_GCC 1
#	endif

#	if defined( __arm__ ) || defined( __aarch64__ ) || defined( _M_ARM ) || defined( _M_ARM64 )
#		define BALL_ARM 1
#		if defined( __aarch64__ ) || defined( _M_ARM64 )
#			define BALL_ARM64 1
#		endif
#	elif defined( __i386__ ) || defined( __x86_64__ ) || defined( _M_IX86 ) || defined( _M_X64 ) || defined( _M_AMD64 )
#		define BALL_X86 1
#	endif

#	if defined( __i386__ ) || defined( _M_IX86 )
#		define BALL_32BITS 1
#	endif

#	if defined( __x86_64__ ) || defined( _WIN64 )
#		define BALL_64BITS 1
#	endif

#	if defined( _WIN32 ) || defined( _WIN64 )
#		define BALL_WIN 1
#	endif

#	if defined( __unix__ ) || defined( __unix )
#		define BALL_UNIX 1
#	endif

#	if defined( __APPLE__ ) && defined( __MACH__ )
#		define BALL_APPLE 1
#	elif defined( __linux__ ) || defined( __linux )
#		define BALL_LINUX 1
#	endif

#	if defined( __MCST__ )
#		define BALL_MCST 1
#	endif

#	ifdef BALL_CXX
#		define BALL_LANGUAGE_STR "C++"
#		define BALL_EXTERN_C extern "C"
#		define BALL_EXTERN_CPP extern "C++"
#	else // !defined( BALL_CXX )
#		define BALL_LANGUAGE_STR "C"
#		define BALL_EXTERN_C extern
#		define BALL_EXTERN_CPP extern
#	endif // defined( BALL_CXX )

#endif // !defined( _INCLUDE_BALL_TYPES_C_PLATFORM_H_ )

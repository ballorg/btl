#ifndef _INCLUDE_BALL_TYPES_C_THREAD_H_
#	define _INCLUDE_BALL_TYPES_C_THREAD_H_

#	include "macros.h"

///-----------------------------------------------------------------------------
/// @brief Hints the processor that the current loop is a spin-wait.
///
/// @note Purely a scheduling/power hint with no ordering effect: it shortens the
///       memory-order-violation penalty on x86 and lets an SMT sibling run. A
///       target without such an instruction expands to nothing.
///-----------------------------------------------------------------------------
#	if defined( BALL_MSVC ) && !defined( BALL_CLANG )
#		if defined( BALL_ARM )
#			if !defined( __INTRIN0_H )
BALL_EXTERN_C void __yield( void );
#			endif // !defined( __INTRIN0_H )
#			pragma intrinsic( __yield )
#			define BALL_CPU_RELAX() __yield()
#		elif defined( BALL_X86 )
#			if !defined( __INTRIN0_H )
BALL_EXTERN_C void _mm_pause( void );
#			endif // !defined( __INTRIN0_H )
#			pragma intrinsic( _mm_pause )
#			define BALL_CPU_RELAX() _mm_pause()
#		else // !defined( BALL_ARM ) && !defined( BALL_X86 )
#			define BALL_CPU_RELAX() ( ( void )0 )
#		endif // defined( BALL_ARM )
#	elif defined( BALL_GNUC ) || defined( BALL_CLANG )
#		if defined( BALL_X86 )
#			define BALL_CPU_RELAX() __builtin_ia32_pause()
#		elif defined( BALL_ARM )
#			define BALL_CPU_RELAX() __asm__ __volatile__( "yield" ::: "memory" )
#		else // !defined( BALL_X86 ) && !defined( BALL_ARM )
#			define BALL_CPU_RELAX() ( ( void )0 )
#		endif // defined( BALL_X86 )
#	else // !defined( BALL_MSVC ) && !defined( BALL_GNUC ) && !defined( BALL_CLANG )
#		define BALL_CPU_RELAX() ( ( void )0 )
#	endif // defined( BALL_MSVC ) && !defined( BALL_CLANG )

///-----------------------------------------------------------------------------
/// @brief Hands the rest of the current time slice back to the scheduler.
///
/// @note Declared here the way Ball declares every other platform entry point it
///       needs (see `c/mmap.h`), so no system header is pulled in. `SwitchToThread`
///       lives in kernel32, which every Windows link already resolves;
///       `sched_yield` lives in libc.
///-----------------------------------------------------------------------------
#	if defined( BALL_WIN )
BALL_DLL_IMPORT_C int BALL_WINAPI SwitchToThread( void );
#		define BALL_YIELD_THREAD() ( ( void )SwitchToThread() )
#	else // !defined( BALL_WIN )
BALL_DLL_IMPORT_C int sched_yield( void );
#		define BALL_YIELD_THREAD() ( ( void )sched_yield() )
#	endif // defined( BALL_WIN )

#endif // !defined( _INCLUDE_BALL_TYPES_C_THREAD_H_ )

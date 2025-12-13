#include <ball/time.h>

#if defined( _WIN32 )

/// ---------------------------------------------------------------------------
/// Minimal WinAPI subset (without including <windows.h>).
///
/// Only the required definitions for thread-time retrieval are declared here
/// to avoid pulling the full Windows header set and reduce compilation weight.
/// ---------------------------------------------------------------------------

typedef void *HANDLE;
typedef int BOOL;
typedef unsigned long DWORD;

/// @brief WinAPI FILETIME structure (64-bit timestamp split into two 32-bit words).
/// FILETIME stores time in 100-nanosecond ticks since an arbitrary epoch.
typedef struct FILETIME
{
	DWORD dwLowDateTime;    ///< Lower 32 bits of the 64-bit time value
	DWORD dwHighDateTime;   ///< Upper 32 bits of the 64-bit time value
} FILETIME;

/// @brief Returns a pseudo-handle for the calling thread.
BALL_DLL_IMPORT HANDLE BALL_WINAPI GetCurrentThread( void );

/// @brief Retrieves thread creation/exit times and CPU usage (kernel/user).
/// All returned timestamps are provided as FILETIME (100-ns units).
BALL_DLL_IMPORT BOOL BALL_WINAPI GetThreadTimes(
	HANDLE     hThread,
	FILETIME * lpCreationTime,
	FILETIME * lpExitTime,
	FILETIME * lpKernelTime,
	FILETIME * lpUserTime
);


/// ---------------------------------------------------------------------------
/// @brief Converts a FILETIME structure to nanoseconds.
///
/// FILETIME stores time in 100-ns ticks.
/// Conversion: nanoseconds = ticks * 100.
/// ---------------------------------------------------------------------------
static timens_t FileTimeToNS( const FILETIME *ft )
{
	if( ft == BALL_NULL )
		return 0ULL;

	ullong_t v = ( ( ullong_t )ft->dwHighDateTime << 32 ) |
	             ( ullong_t )ft->dwLowDateTime;  /* combine into 64-bit tick count */

	return ( timens_t )( v * 100ULL );  /* 100-ns ticks -> nanoseconds */
}


/// ---------------------------------------------------------------------------
/// @brief Retrieves CPU time consumed by the current thread (Windows).
///
/// Returns:
///   User-time + Kernel-time accumulated by the OS scheduler, in nanoseconds.
///
/// Notes:
///   - This is CPU time, not wall-clock time.
///   - If GetThreadTimes() fails, the function returns 0.
/// ---------------------------------------------------------------------------
timens_t GetThreadCPUTime( void )
{
	FILETIME createTime;
	FILETIME exitTime;
	FILETIME kernelTime;
	FILETIME userTime;

	if( !GetThreadTimes( GetCurrentThread(), &createTime, &exitTime, &kernelTime, &userTime ) )
		return 0LL;

	return FileTimeToNS( &userTime ) + FileTimeToNS( &kernelTime );
}

#elif defined( __linux__ )

/// ---------------------------------------------------------------------------
/// Minimal POSIX subset (without including <time.h>).
///
/// Only clock_gettime() and a local timespec definition are declared.
/// ---------------------------------------------------------------------------

struct timespec_t
{
	long_t nTVSeconds;    ///< Seconds component
	long_t nTVNSeconds;   ///< Nanoseconds component
};

/// @brief Imported clock_gettime() signature.
BALL_DLL_IMPORT int clock_gettime( int nClockId, struct timespec_t *ts );

/// @brief Linux constant for per-thread CPU time.
#	define CLOCK_THREAD_CPUTIME_ID 3


/// ---------------------------------------------------------------------------
/// @brief Retrieves CPU time consumed by the current thread (Linux).
///
/// Returns:
///   CPU time spent by the calling thread in nanoseconds.
///
/// Notes:
///   - clock_gettime( CLOCK_THREAD_CPUTIME_ID ) returns CPU time, not real time.
///   - If the system call fails, the function returns 0.
/// ---------------------------------------------------------------------------
timens_t GetThreadCPUTime( void )
{
	struct timespec_t ts;

	if( clock_gettime( CLOCK_THREAD_CPUTIME_ID, &ts ) != 0 )
		return 0LL;

	return ( timens_t )ts.nTVSeconds * 1000000000LL +
	       ( timens_t )ts.nTVNSeconds;
}

#else

/// ---------------------------------------------------------------------------
/// @brief Stub for unsupported platforms. Always returns 0.
/// ---------------------------------------------------------------------------
timens_t GetThreadCPUTime( void )
{
	return 0LL;
}

#endif

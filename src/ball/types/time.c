#include <ball/time.h>

#if defined( _WIN32 )

/// ---------------------------------------------------------------------------
/// Minimal WinAPI subset (without including <windows.h>).
///
/// Only the required definitions for high-resolution timer retrieval are declared here
/// to avoid pulling the full Windows header set and reduce compilation weight.
/// ---------------------------------------------------------------------------

typedef int BOOL;
typedef struct BallLargeInteger_t
{
	llong_t nQuadPart;
} BallLargeInteger_t;

BALL_DLL_IMPORT BOOL BALL_WINAPI QueryPerformanceCounter( BallLargeInteger_t *lpPerformanceCount );
BALL_DLL_IMPORT BOOL BALL_WINAPI QueryPerformanceFrequency( BallLargeInteger_t *lpFrequency );


/// ---------------------------------------------------------------------------
/// @brief Retrieves high-resolution monotonic time in nanoseconds (Windows).
///
/// Returns:
///   Current performance-counter timestamp converted to nanoseconds.
///
/// Notes:
///   - Backed by QueryPerformanceCounter (high precision, monotonic).
///   - If the counter/frequency query fails, the function returns 0.
/// ---------------------------------------------------------------------------
timens_t GetTimeNS( void )
{
	static ullong_t nFrequency = 0ULL;
	BallLargeInteger_t counter;

	if( nFrequency == 0ULL )
	{
		BallLargeInteger_t frequency;

		if( !QueryPerformanceFrequency( &frequency ) || frequency.nQuadPart <= 0 )
			return 0ULL;

		nFrequency = ( ullong_t )frequency.nQuadPart;
	}

	if( !QueryPerformanceCounter( &counter ) || counter.nQuadPart < 0 )
		return 0ULL;

	{
		const ullong_t nCounter = ( ullong_t )counter.nQuadPart;
		const ullong_t nSeconds = nCounter / nFrequency;
		const ullong_t nRemainder = nCounter % nFrequency;

		return ( timens_t )( nSeconds * 1000000000ULL +
			( nRemainder * 1000000000ULL ) / nFrequency );
	}
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

/// @brief Linux clock ids used for high-resolution monotonic time.
#	ifndef CLOCK_MONOTONIC
#		define CLOCK_MONOTONIC 1
#	endif
#	ifndef CLOCK_MONOTONIC_RAW
#		define CLOCK_MONOTONIC_RAW 4
#	endif


/// ---------------------------------------------------------------------------
/// @brief Retrieves high-resolution monotonic time in nanoseconds (Linux).
///
/// Returns:
///   Monotonic timestamp in nanoseconds.
///
/// Notes:
///   - Prefers CLOCK_MONOTONIC_RAW when available.
///   - Falls back to CLOCK_MONOTONIC.
///   - If the system call fails, the function returns 0.
/// ---------------------------------------------------------------------------
timens_t GetTimeNS( void )
{
	struct timespec_t ts;

	if( clock_gettime( CLOCK_MONOTONIC_RAW, &ts ) != 0 &&
		clock_gettime( CLOCK_MONOTONIC, &ts ) != 0 )
	{
		return 0ULL;
	}

	return ( timens_t )ts.nTVSeconds * 1000000000ULL +
	       ( timens_t )ts.nTVNSeconds;
}

#else

/// ---------------------------------------------------------------------------
/// @brief Stub for unsupported platforms. Always returns 0.
/// ---------------------------------------------------------------------------
timens_t GetTimeNS( void )
{
	return 0ULL;
}

#endif

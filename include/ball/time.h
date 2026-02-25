#ifndef _INCLUDE_BALL_TIME_H_
#define _INCLUDE_BALL_TIME_H_

#include "types/base/arch.h"
#include "types/base/fixed.h"
#include "types/c/macros.h"

///-----------------------------------------------------------------------------
/// @brief Core time type used across the Ball runtime.
///
/// This type always represents:
///   - nanoseconds since the Unix epoch (when used as an absolute timestamp), or
///   - nanoseconds as a duration (when used as an interval).
///
/// The unit is fixed to nanoseconds to maintain consistency and avoid implicit
/// conversions or precision loss across all timing subsystems.
///-----------------------------------------------------------------------------
typedef ullong_t timens_t;

///-----------------------------------------------------------------------------
/// @brief Decomposed Unix timestamp to UTC.
///
/// Represents a calendar date/time derived from nanoseconds since Unix epoch.
/// Contains standard Gregorian components:
///   - year, month, day
///   - hours, minutes, seconds
///
/// All fields are stored in 32-bit integers, sufficient for representing the
/// practical range of Unix timestamps.
///-----------------------------------------------------------------------------
struct Date_t
{
	uint64_t nYears;        ///< Gregorian year (e.g., 2025)
	uint8_t  nMonths;       ///< Month of year (1–12)
	uint8_t  nDays;         ///< Day of month (1–31)
	uint8_t  nHours;        ///< Hour of day (0–23)
	uint8_t  nMinutes;      ///< Minute of hour (0–59)
	uint8_t  nSeconds;      ///< Second of minute (0–59)
	uint16_t nMilliseconds; ///< Milliseconds of second (0-1000)
	uint16_t nNanoseconds;  ///< Nanoseconds of millisecond (0-1000)
};

///-----------------------------------------------------------------------------
/// @brief Retrieves CPU time consumed by the current thread (user + kernel).
///
/// Returns:
///   - Nanoseconds of CPU time consumed by the calling thread.
///
/// The function is implemented in platform-specific C translation units,
/// providing OS-native measurement based on:
///   - Windows: GetThreadTimes() → FILETIME (100 ns units)
///   - Linux:   clock_gettime() with CLOCK_THREAD_CPUTIME_ID
///
/// This routine does *not* return wall-clock time. It reflects actual CPU
/// execution time spent by the thread.
///-----------------------------------------------------------------------------
BALL_EXTERN_C timens_t GetTimeNS();

#endif // _INCLUDE_BALL_TIME_H_

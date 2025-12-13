#ifndef _INCLUDE_BALL_TIME_HPP_
#	define _INCLUDE_BALL_TIME_HPP_

// Simple profiling helpers:
//   - BALL_PROF_BEGIN( tag ): capture start CPU time of current thread
//   - BALL_PROF_END( tag ):   create CTimeNS with elapsed CPU time since BEGIN
// Usage (supply a unique tag per measurement):
//   BALL_PROF_BEGIN( Case01 );
//   ... work ...
//   auto elapsed = BALL_PROF_END( Case01 ); // CTimeNS holding elapsed nanoseconds
#	define BALL_PROF_CAT_IMPL( x, y ) x##y
#	define BALL_PROF_CAT( x, y ) BALL_PROF_CAT_IMPL( x, y )
#	define BALL_PROF_BEGIN( tag ) const BTL::timens_t BALL_PROF_CAT( __ballProfBeginNS_, tag ) = BTL::GetThreadCPUTime()
#	define BALL_PROF_END( tag ) BTL::CTimeNS( BTL::GetThreadCPUTime() - BALL_PROF_CAT( __ballProfBeginNS_, tag ) )

namespace BTL
{
#	include "types/base/arch.h"
#	include "time.h"

	//-----------------------------------------------------------------------------
	// @brief CTimeNS – lightweight POD-style class for time conversion,
	//        arithmetic, and calendar decomposition without relying on STL,
	//        <ctime>, or platform headers. All values are stored in nanoseconds.
	//-----------------------------------------------------------------------------
	class CTimeNS
	{
	public:
		//-------------------------------------------------------------------------
		// @brief Constructs zero-initialized time value.
		//-------------------------------------------------------------------------
		CTimeNS() noexcept : m_nNS( 0 ) {}

		//-------------------------------------------------------------------------
		// @brief Constructs time from a raw nanosecond value.
		//-------------------------------------------------------------------------
		explicit CTimeNS( const timens_t nNS ) noexcept : m_nNS( nNS ) {}

	public:
		void SetNS( const timens_t nNS ) noexcept { m_nNS = nNS; }

		//-------------------------------------------------------------------------
		// @brief Adds a nanosecond delta to the current value.
		//-------------------------------------------------------------------------
		void AddNS( const timens_t nNS ) noexcept { m_nNS += nNS; }

		//-------------------------------------------------------------------------
		// @brief Returns raw nanoseconds.
		//-------------------------------------------------------------------------
		timens_t GetNSs() const noexcept { return m_nNS; }

	public:
		//-------------------------------------------------------------------------
		// @brief Returns microseconds (integer truncation).
		//-------------------------------------------------------------------------
		timens_t AsMicros() const noexcept { return m_nNS / 1'000LL; }

		//-------------------------------------------------------------------------
		// @brief Returns milliseconds (integer truncation).
		//-------------------------------------------------------------------------
		timens_t AsMillis() const noexcept { return m_nNS / 1'000'000LL; }

		//-------------------------------------------------------------------------
		// @brief Returns seconds (integer truncation).
		//-------------------------------------------------------------------------
		timens_t AsSeconds() const noexcept { return m_nNS / 1'000'000'000LL; }

		//-------------------------------------------------------------------------
		// @brief Returns minutes (integer truncation).
		//-------------------------------------------------------------------------
		timens_t AsMinutes() const noexcept { return AsSeconds() / 60LL; }

		//-------------------------------------------------------------------------
		// @brief Returns hours (integer truncation).
		//-------------------------------------------------------------------------
		timens_t AsHours() const noexcept { return AsSeconds() / 3600LL; }

	public:
		//-------------------------------------------------------------------------
		// @brief Returns microseconds as floating-point (double_t precision).
		//-------------------------------------------------------------------------
		double_t AsMicrosF() const noexcept { return static_cast< double_t >( m_nNS ) / 1'000.0; }

		//-------------------------------------------------------------------------
		// @brief Returns milliseconds as floating-point (double_t precision).
		//-------------------------------------------------------------------------
		double_t AsMillisF() const noexcept { return static_cast< double_t >( m_nNS ) / 1'000'000.0; }

		//-------------------------------------------------------------------------
		// @brief Returns seconds as floating-point (double_t precision).
		//-------------------------------------------------------------------------
		double_t AsSecondsF() const noexcept { return static_cast< double_t >( m_nNS ) / 1'000'000'000.0; }

		//-------------------------------------------------------------------------
		// @brief Returns minutes as floating-point.
		//-------------------------------------------------------------------------
		double_t AsMinutesF() const noexcept { return static_cast< double_t >( m_nNS ) / ( 60.0 * 1'000'000'000.0 ); }

		//-------------------------------------------------------------------------
		// @brief Returns hours as floating-point.
		//-------------------------------------------------------------------------
		double_t AsHoursF() const noexcept { return static_cast< double_t >( m_nNS ) / ( 3600.0 * 1'000'000'000.0 ); }

	private:
		timens_t m_nNS;
	}; // class CTimeNS
}; // namespace BTL

#endif // !defined( _INCLUDE_BALL_TIME_HPP_ )

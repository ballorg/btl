#ifndef _INCLUDE_BALL_TYPES_LOCK_HPP_
#	define _INCLUDE_BALL_TYPES_LOCK_HPP_

#	pragma once

/// @brief Tag selecting the constructor that leaves a lock object unlocked.
struct MDeferLock {};
/// @brief Tag selecting the constructor that takes over an already-held lock.
struct MAdoptLock {};

inline constexpr MDeferLock DEFER_LOCK{};
inline constexpr MAdoptLock ADOPT_LOCK{};

///-----------------------------------------------------------------------------
/// @brief Escalating wait for a contended lock word.
///
/// @details Starts by spinning with the processor's spin-wait hint, doubling the
/// spin count each round, and falls back to handing the time slice back once the
/// wait has clearly outlived a short critical section. Keeping the fast path in
/// user space is what makes an uncontended acquire a single atomic instruction.
///
/// @complexity O(1) per call; the spin length is bounded by `SPIN_ROUNDS`.
///-----------------------------------------------------------------------------
class CSpinBackoff
{
public:
	/// @brief Rounds spun before the wait starts yielding instead.
	static constexpr uint32_t SPIN_ROUNDS = 6u;

	void Pause() noexcept
	{
		if ( m_nRound < SPIN_ROUNDS )
		{
			const uint32_t nSpins = uint32_t( 1u ) << m_nRound;

			for ( uint32_t i = 0; i < nSpins; ++i )
				BALL_CPU_RELAX();

			++m_nRound;

			return;
		}

		BALL_YIELD_THREAD();
	}

	void Reset() noexcept { m_nRound = 0; }

private:
	uint32_t m_nRound = 0;
}; // class CSpinBackoff

///-----------------------------------------------------------------------------
/// @brief Exclusive mutex: one holder at a time.
///
/// @details A single atomic word taken with an acquire compare-exchange and
/// released with a release store, so every write made under the lock is visible
/// to the next holder. Waiting escalates through `CSpinBackoff`.
///
/// @note The shared-access members are aliases of the exclusive ones, so this
/// mutex satisfies the same contract as `CSharedMutex` and both can drive the
/// same lock templates -- readers simply do not run concurrently here.
///
/// @note Not recursive: a thread that locks twice deadlocks. Not movable or
/// copyable, since a waiter holds its address.
///
/// @complexity O(1) uncontended; a contended acquire is bounded by how long the
/// holder keeps the lock.
///-----------------------------------------------------------------------------
class CMutex
{
public:
	constexpr CMutex() noexcept = default;

	CMutex( const CMutex & ) = delete;
	CMutex &operator=( const CMutex & ) = delete;

	/// @complexity O(1).
	bool TryLock() noexcept
	{
		uint32_t nExpected = 0u;

		return m_nLocked.CompareExchange( nExpected, 1u, EMemoryOrder::ACQUIRE, EMemoryOrder::RELAXED );
	}

	void Lock() noexcept
	{
		CSpinBackoff backoff;

		while ( !TryLock() )
		{
			// Re-read relaxed until the word looks free again; retrying the
			// compare-exchange directly would keep the cache line bouncing.
			while ( m_nLocked.Load( EMemoryOrder::RELAXED ) )
				backoff.Pause();
		}
	}

	void Unlock() noexcept
	{
		BALL_ASSERT_MESSAGE( m_nLocked.Load( EMemoryOrder::RELAXED ), "Unlock requires a held mutex" );

		m_nLocked.Store( 0u, EMemoryOrder::RELEASE );
	}

	bool TryLockShared() noexcept { return TryLock(); }
	void LockShared() noexcept { Lock(); }
	void UnlockShared() noexcept { Unlock(); }

private:
	CAtomic< uint32_t > m_nLocked{ 0u };
}; // class CMutex

///-----------------------------------------------------------------------------
/// @brief Reader/writer mutex: many concurrent readers, or one writer.
///
/// @details One atomic word carries the whole state -- the top bit marks a writer
/// in possession, the next bit marks a writer waiting, and the remaining bits count
/// the readers currently inside. The waiting bit is what stops a stream of readers
/// from starving a writer: once it is set, an arriving reader queues behind the
/// writer instead of joining the current batch.
///
/// @note Not recursive, and not upgradable: a thread holding shared access must
/// release it before asking for exclusive access. Not movable or copyable.
///
/// @complexity O(1) uncontended for both modes.
///-----------------------------------------------------------------------------
class CSharedMutex
{
public:
	/// @brief A writer owns the mutex.
	static constexpr uint32_t WRITER = uint32_t( 1u ) << 31;
	/// @brief A writer is waiting; new readers must not enter.
	static constexpr uint32_t WRITER_PENDING = uint32_t( 1u ) << 30;
	/// @brief Bits counting the readers inside.
	static constexpr uint32_t READER_MASK = WRITER_PENDING - 1u;

	constexpr CSharedMutex() noexcept = default;

	CSharedMutex( const CSharedMutex & ) = delete;
	CSharedMutex &operator=( const CSharedMutex & ) = delete;

	/// @complexity O(1).
	bool TryLock() noexcept
	{
		uint32_t nExpected = 0u;

		return m_nState.CompareExchange( nExpected, WRITER, EMemoryOrder::ACQUIRE, EMemoryOrder::RELAXED );
	}

	void Lock() noexcept
	{
		CSpinBackoff backoff;

		for ( ;; )
		{
			uint32_t nState = m_nState.Load( EMemoryOrder::RELAXED );

			if ( !( nState & WRITER ) && !( nState & READER_MASK ) )
			{
				// Free apart from a pending flag, which this acquisition consumes.
				if ( m_nState.CompareExchange( nState, WRITER, EMemoryOrder::ACQUIRE, EMemoryOrder::RELAXED ) )
					return;

				continue;
			}

			// Close the door behind the readers already inside, then wait for them.
			if ( !( nState & WRITER_PENDING ) )
				( void )m_nState.CompareExchange( nState, nState | WRITER_PENDING, EMemoryOrder::RELAXED, EMemoryOrder::RELAXED );

			backoff.Pause();
		}
	}

	void Unlock() noexcept
	{
		BALL_ASSERT_MESSAGE( m_nState.Load( EMemoryOrder::RELAXED ) & WRITER, "Unlock requires exclusive ownership" );

		m_nState.Store( 0u, EMemoryOrder::RELEASE );
	}

	/// @complexity O(1).
	bool TryLockShared() noexcept
	{
		uint32_t nState = m_nState.Load( EMemoryOrder::RELAXED );

		if ( nState & ( WRITER | WRITER_PENDING ) )
			return false;

		BALL_ASSERT_MESSAGE( ( nState & READER_MASK ) != READER_MASK, "Shared reader count overflowed" );

		return m_nState.CompareExchange( nState, nState + 1u, EMemoryOrder::ACQUIRE, EMemoryOrder::RELAXED );
	}

	void LockShared() noexcept
	{
		CSpinBackoff backoff;

		for ( ;; )
		{
			uint32_t nState = m_nState.Load( EMemoryOrder::RELAXED );

			if ( nState & ( WRITER | WRITER_PENDING ) )
			{
				backoff.Pause();

				continue;
			}

			BALL_ASSERT_MESSAGE( ( nState & READER_MASK ) != READER_MASK, "Shared reader count overflowed" );

			if ( m_nState.CompareExchange( nState, nState + 1u, EMemoryOrder::ACQUIRE, EMemoryOrder::RELAXED ) )
				return;
		}
	}

	void UnlockShared() noexcept
	{
		BALL_ASSERT_MESSAGE( m_nState.Load( EMemoryOrder::RELAXED ) & READER_MASK, "UnlockShared requires shared ownership" );

		// Release so a writer that observes the count reach zero sees this reader's
		// accesses as complete; the pending flag is left for the writer to consume.
		( void )m_nState.FetchSub( 1u, EMemoryOrder::RELEASE );
	}

private:
	CAtomic< uint32_t > m_nState{ 0u };
}; // class CSharedMutex

///-----------------------------------------------------------------------------
/// @brief Mutex that locks nothing, for a container synchronized some other way.
///
/// @details Satisfies the same contract as `CMutex` and `CSharedMutex` with every
/// operation compiled away, so a policy can opt out of locking entirely without a
/// second code path. Choosing it moves the whole burden onto the caller: it is only
/// sound when no thread performs a structural modification while another is inside
/// the container.
///
/// @complexity O(1); every member is empty.
///-----------------------------------------------------------------------------
class CNullMutex
{
public:
	constexpr CNullMutex() noexcept = default;

	CNullMutex( const CNullMutex & ) = delete;
	CNullMutex &operator=( const CNullMutex & ) = delete;

	static constexpr bool TryLock() noexcept { return true; }
	static constexpr void Lock() noexcept {}
	static constexpr void Unlock() noexcept {}
	static constexpr bool TryLockShared() noexcept { return true; }
	static constexpr void LockShared() noexcept {}
	static constexpr void UnlockShared() noexcept {}
}; // class CNullMutex

///-----------------------------------------------------------------------------
/// @brief RAII exclusive ownership of one mutex.
///
/// @details Movable so an access object can carry its lock out of the function
/// that took it; the moved-from lock owns nothing.
///
/// @complexity O(1) plus the wrapped acquire.
///-----------------------------------------------------------------------------
template < typename TMutex >
class CUniqueLock
{
public:
	using Mutex_t = TMutex;

	explicit CUniqueLock( Mutex_t &mutex ) noexcept : m_pMutex( &mutex ), m_bOwns( true ) { mutex.Lock(); }
	CUniqueLock( Mutex_t &mutex, MDeferLock ) noexcept : m_pMutex( &mutex ), m_bOwns( false ) {}
	CUniqueLock( Mutex_t &mutex, MAdoptLock ) noexcept : m_pMutex( &mutex ), m_bOwns( true ) {}

	~CUniqueLock() noexcept
	{
		if ( m_bOwns )
			m_pMutex->Unlock();
	}

	CUniqueLock( const CUniqueLock & ) = delete;
	CUniqueLock &operator=( const CUniqueLock & ) = delete;

	CUniqueLock( CUniqueLock &&other ) noexcept : m_pMutex( other.m_pMutex ), m_bOwns( other.m_bOwns ) { other.m_bOwns = false; }

	CUniqueLock &operator=( CUniqueLock &&other ) noexcept
	{
		if ( this != &other )
		{
			if ( m_bOwns )
				m_pMutex->Unlock();

			m_pMutex = other.m_pMutex;
			m_bOwns = other.m_bOwns;
			other.m_bOwns = false;
		}

		return *this;
	}

	void Lock() noexcept { m_pMutex->Lock(); m_bOwns = true; }
	bool TryLock() noexcept { m_bOwns = m_pMutex->TryLock(); return m_bOwns; }
	void Unlock() noexcept { m_pMutex->Unlock(); m_bOwns = false; }
	bool Owns() const noexcept { return m_bOwns; }

private:
	Mutex_t *m_pMutex;
	bool m_bOwns;
}; // class CUniqueLock

///-----------------------------------------------------------------------------
/// @brief RAII shared ownership of one mutex.
///
/// @complexity O(1) plus the wrapped acquire.
///-----------------------------------------------------------------------------
template < typename TMutex >
class CSharedLock
{
public:
	using Mutex_t = TMutex;

	explicit CSharedLock( Mutex_t &mutex ) noexcept : m_pMutex( &mutex ), m_bOwns( true ) { mutex.LockShared(); }
	CSharedLock( Mutex_t &mutex, MDeferLock ) noexcept : m_pMutex( &mutex ), m_bOwns( false ) {}
	CSharedLock( Mutex_t &mutex, MAdoptLock ) noexcept : m_pMutex( &mutex ), m_bOwns( true ) {}

	~CSharedLock() noexcept
	{
		if ( m_bOwns )
			m_pMutex->UnlockShared();
	}

	CSharedLock( const CSharedLock & ) = delete;
	CSharedLock &operator=( const CSharedLock & ) = delete;

	CSharedLock( CSharedLock &&other ) noexcept : m_pMutex( other.m_pMutex ), m_bOwns( other.m_bOwns ) { other.m_bOwns = false; }

	CSharedLock &operator=( CSharedLock &&other ) noexcept
	{
		if ( this != &other )
		{
			if ( m_bOwns )
				m_pMutex->UnlockShared();

			m_pMutex = other.m_pMutex;
			m_bOwns = other.m_bOwns;
			other.m_bOwns = false;
		}

		return *this;
	}

	void Lock() noexcept { m_pMutex->LockShared(); m_bOwns = true; }
	bool TryLock() noexcept { m_bOwns = m_pMutex->TryLockShared(); return m_bOwns; }
	void Unlock() noexcept { m_pMutex->UnlockShared(); m_bOwns = false; }
	bool Owns() const noexcept { return m_bOwns; }

private:
	Mutex_t *m_pMutex;
	bool m_bOwns;
}; // class CSharedLock

///-----------------------------------------------------------------------------
/// @brief RAII exclusive ownership of two mutexes, taken in a deadlock-free order.
///
/// @details Both are acquired lowest address first, so two threads locking the
/// same pair from opposite sides still agree on the order. Locking one mutex with
/// itself (self-assignment of a container) is detected and taken once.
///
/// @complexity O(1) plus the two acquires.
///-----------------------------------------------------------------------------
template < typename TMutex >
class CScopedLock
{
public:
	using Mutex_t = TMutex;

	CScopedLock( Mutex_t &first, Mutex_t &second ) noexcept :
		m_pFirst( &first < &second ? &first : &second ),
		m_pSecond( &first < &second ? &second : &first )
	{
		m_pFirst->Lock();

		if ( m_pFirst != m_pSecond )
			m_pSecond->Lock();
	}

	~CScopedLock() noexcept
	{
		if ( m_pFirst != m_pSecond )
			m_pSecond->Unlock();

		m_pFirst->Unlock();
	}

	CScopedLock( const CScopedLock & ) = delete;
	CScopedLock &operator=( const CScopedLock & ) = delete;

private:
	Mutex_t *m_pFirst;
	Mutex_t *m_pSecond;
}; // class CScopedLock

#endif // !defined( _INCLUDE_BALL_TYPES_LOCK_HPP_ )

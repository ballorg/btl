#ifndef _INCLUDE_BALL_TYPES_ATOMIC_HPP_
#	define _INCLUDE_BALL_TYPES_ATOMIC_HPP_

#	pragma once

///-----------------------------------------------------------------------------
/// @brief Ordering constraint an atomic operation imposes on the surrounding
/// non-atomic accesses.
///
/// @details Same six-point lattice the C++ memory model defines. A backend is
/// always allowed to be *stronger* than requested, and the MSVC one is: it maps
/// every read-modify-write onto a full-barrier `_Interlocked*` intrinsic.
///-----------------------------------------------------------------------------
enum class EMemoryOrder : uint8_t
{
	RELAXED = 0,
	CONSUME,
	ACQUIRE,
	RELEASE,
	ACQ_REL,
	SEQ_CST
};

#	if defined( BALL_MSVC ) && !defined( BALL_CLANG )

/// @brief Whether an order needs a barrier before the access (a release edge).
constexpr bool MemoryOrder_IsRelease( EMemoryOrder eOrder ) noexcept
{
	return eOrder == EMemoryOrder::RELEASE || eOrder == EMemoryOrder::ACQ_REL || eOrder == EMemoryOrder::SEQ_CST;
}

/// @brief Whether an order needs a barrier after the access (an acquire edge).
constexpr bool MemoryOrder_IsAcquire( EMemoryOrder eOrder ) noexcept
{
	return eOrder == EMemoryOrder::CONSUME || eOrder == EMemoryOrder::ACQUIRE || eOrder == EMemoryOrder::ACQ_REL || eOrder == EMemoryOrder::SEQ_CST;
}

#	else // !( defined( BALL_MSVC ) && !defined( BALL_CLANG ) )

/// @brief Translates an order into the GCC/Clang `__ATOMIC_*` constant.
constexpr int MemoryOrder_Builtin( EMemoryOrder eOrder ) noexcept
{
	switch ( eOrder )
	{
		case EMemoryOrder::RELAXED: return __ATOMIC_RELAXED;
		case EMemoryOrder::CONSUME: return __ATOMIC_CONSUME;
		case EMemoryOrder::ACQUIRE: return __ATOMIC_ACQUIRE;
		case EMemoryOrder::RELEASE: return __ATOMIC_RELEASE;
		case EMemoryOrder::ACQ_REL: return __ATOMIC_ACQ_REL;
		default: return __ATOMIC_SEQ_CST;
	}
}

/// @brief Order a failed compare-exchange may use: no release edge, since nothing was written.
constexpr int MemoryOrder_BuiltinFailure( EMemoryOrder eOrder ) noexcept
{
	switch ( eOrder )
	{
		case EMemoryOrder::RELAXED: return __ATOMIC_RELAXED;
		case EMemoryOrder::RELEASE: return __ATOMIC_RELAXED;
		case EMemoryOrder::CONSUME: return __ATOMIC_CONSUME;
		case EMemoryOrder::ACQUIRE: return __ATOMIC_ACQUIRE;
		case EMemoryOrder::ACQ_REL: return __ATOMIC_ACQUIRE;
		default: return __ATOMIC_SEQ_CST;
	}
}

#	endif // defined( BALL_MSVC ) && !defined( BALL_CLANG )

///-----------------------------------------------------------------------------
/// @brief Unsigned word an atomic of @p SIZE bytes is carried in, together with
/// the signed spelling MSVC's intrinsics take.
///
/// @details Every operation is performed on this word: a value of the user's type
/// is bit-cast into it and back, so one backend serves integers, enumerations,
/// pointers and floating-point values alike.
///-----------------------------------------------------------------------------
template < size_t SIZE > struct MAtomicWord;

template <> struct MAtomicWord< 1 > { using Type = uint8_t; using Intrinsic_t = char; };
template <> struct MAtomicWord< 2 > { using Type = uint16_t; using Intrinsic_t = short; };
template <> struct MAtomicWord< 4 > { using Type = uint32_t; using Intrinsic_t = long; };
template <> struct MAtomicWord< 8 > { using Type = uint64_t; using Intrinsic_t = llong_t; };

///-----------------------------------------------------------------------------
/// @brief Backend for one atomic word width.
///
/// @details GCC and Clang forward straight to the `__atomic_*` builtins. MSVC has
/// no equivalent family, so loads and stores are plain accesses fenced by
/// `BALL_ATOMIC_FENCE` (a compiler barrier on x86-TSO, a `DMB ISH` on ARM) and
/// every read-modify-write is the matching full-barrier `_Interlocked*`.
///-----------------------------------------------------------------------------
template < size_t SIZE >
struct MAtomicOps
{
	using Word_t = typename MAtomicWord< SIZE >::Type;
	using Intrinsic_t = typename MAtomicWord< SIZE >::Intrinsic_t;

#	if defined( BALL_MSVC ) && !defined( BALL_CLANG )

	static Intrinsic_t volatile *Target( Word_t *pValue ) noexcept { return reinterpret_cast< Intrinsic_t volatile * >( pValue ); }
	static Intrinsic_t const volatile *Target( const Word_t *pValue ) noexcept { return reinterpret_cast< Intrinsic_t const volatile * >( pValue ); }

	static Word_t Load( const Word_t *pValue, EMemoryOrder eOrder ) noexcept
	{
		const Word_t nValue = static_cast< Word_t >( *Target( pValue ) );

		if ( MemoryOrder_IsAcquire( eOrder ) )
			BALL_ATOMIC_FENCE();

		return nValue;
	}

	static void Store( Word_t *pValue, Word_t nDesired, EMemoryOrder eOrder ) noexcept
	{
		// A sequentially consistent store must also order against later loads, which
		// on x86-TSO takes an actual locked instruction rather than a barrier.
		if ( eOrder == EMemoryOrder::SEQ_CST )
		{
			( void )Exchange( pValue, nDesired, eOrder );

			return;
		}

		if ( MemoryOrder_IsRelease( eOrder ) )
			BALL_ATOMIC_FENCE();

		*Target( pValue ) = static_cast< Intrinsic_t >( nDesired );
	}

	static Word_t Exchange( Word_t *pValue, Word_t nDesired, EMemoryOrder ) noexcept
	{
		if constexpr ( SIZE == 1 )
			return static_cast< Word_t >( _InterlockedExchange8( Target( pValue ), static_cast< Intrinsic_t >( nDesired ) ) );
		else if constexpr ( SIZE == 2 )
			return static_cast< Word_t >( _InterlockedExchange16( Target( pValue ), static_cast< Intrinsic_t >( nDesired ) ) );
		else if constexpr ( SIZE == 4 )
			return static_cast< Word_t >( _InterlockedExchange( Target( pValue ), static_cast< Intrinsic_t >( nDesired ) ) );
		else
			return static_cast< Word_t >( _InterlockedExchange64( reinterpret_cast< __int64 volatile * >( pValue ), static_cast< __int64 >( nDesired ) ) );
	}

	static bool CompareExchange( Word_t *pValue, Word_t &nExpected, Word_t nDesired, EMemoryOrder, EMemoryOrder ) noexcept
	{
		Word_t nPrevious;

		if constexpr ( SIZE == 1 )
			nPrevious = static_cast< Word_t >( _InterlockedCompareExchange8( Target( pValue ), static_cast< Intrinsic_t >( nDesired ), static_cast< Intrinsic_t >( nExpected ) ) );
		else if constexpr ( SIZE == 2 )
			nPrevious = static_cast< Word_t >( _InterlockedCompareExchange16( Target( pValue ), static_cast< Intrinsic_t >( nDesired ), static_cast< Intrinsic_t >( nExpected ) ) );
		else if constexpr ( SIZE == 4 )
			nPrevious = static_cast< Word_t >( _InterlockedCompareExchange( Target( pValue ), static_cast< Intrinsic_t >( nDesired ), static_cast< Intrinsic_t >( nExpected ) ) );
		else
			nPrevious = static_cast< Word_t >( _InterlockedCompareExchange64( reinterpret_cast< __int64 volatile * >( pValue ), static_cast< __int64 >( nDesired ), static_cast< __int64 >( nExpected ) ) );

		if ( nPrevious == nExpected )
			return true;

		nExpected = nPrevious;

		return false;
	}

#		define BALL_ATOMIC_MSVC_RMW( NAME, INTRINSIC ) \
		static Word_t NAME( Word_t *pValue, Word_t nOperand, EMemoryOrder ) noexcept \
		{ \
			if constexpr ( SIZE == 1 ) \
				return static_cast< Word_t >( INTRINSIC##8( Target( pValue ), static_cast< Intrinsic_t >( nOperand ) ) ); \
			else if constexpr ( SIZE == 2 ) \
				return static_cast< Word_t >( INTRINSIC##16( Target( pValue ), static_cast< Intrinsic_t >( nOperand ) ) ); \
			else if constexpr ( SIZE == 4 ) \
				return static_cast< Word_t >( INTRINSIC( Target( pValue ), static_cast< Intrinsic_t >( nOperand ) ) ); \
			else \
				return static_cast< Word_t >( INTRINSIC##64( reinterpret_cast< __int64 volatile * >( pValue ), static_cast< __int64 >( nOperand ) ) ); \
		}

	BALL_ATOMIC_MSVC_RMW( FetchAdd, _InterlockedExchangeAdd )
	BALL_ATOMIC_MSVC_RMW( FetchAnd, _InterlockedAnd )
	BALL_ATOMIC_MSVC_RMW( FetchOr, _InterlockedOr )
	BALL_ATOMIC_MSVC_RMW( FetchXor, _InterlockedXor )

#		undef BALL_ATOMIC_MSVC_RMW

	static Word_t FetchSub( Word_t *pValue, Word_t nOperand, EMemoryOrder eOrder ) noexcept
	{
		// Two's complement: subtracting is adding the negation of the operand.
		return FetchAdd( pValue, static_cast< Word_t >( Word_t( 0 ) - nOperand ), eOrder );
	}

#	else // !( defined( BALL_MSVC ) && !defined( BALL_CLANG ) )

	static Word_t Load( const Word_t *pValue, EMemoryOrder eOrder ) noexcept { return __atomic_load_n( pValue, MemoryOrder_Builtin( eOrder ) ); }
	static void Store( Word_t *pValue, Word_t nDesired, EMemoryOrder eOrder ) noexcept { __atomic_store_n( pValue, nDesired, MemoryOrder_Builtin( eOrder ) ); }
	static Word_t Exchange( Word_t *pValue, Word_t nDesired, EMemoryOrder eOrder ) noexcept { return __atomic_exchange_n( pValue, nDesired, MemoryOrder_Builtin( eOrder ) ); }

	static bool CompareExchange( Word_t *pValue, Word_t &nExpected, Word_t nDesired, EMemoryOrder eSuccess, EMemoryOrder eFailure ) noexcept
	{
		return __atomic_compare_exchange_n( pValue, &nExpected, nDesired, false, MemoryOrder_Builtin( eSuccess ), MemoryOrder_BuiltinFailure( eFailure ) );
	}

	static Word_t FetchAdd( Word_t *pValue, Word_t nOperand, EMemoryOrder eOrder ) noexcept { return __atomic_fetch_add( pValue, nOperand, MemoryOrder_Builtin( eOrder ) ); }
	static Word_t FetchSub( Word_t *pValue, Word_t nOperand, EMemoryOrder eOrder ) noexcept { return __atomic_fetch_sub( pValue, nOperand, MemoryOrder_Builtin( eOrder ) ); }
	static Word_t FetchAnd( Word_t *pValue, Word_t nOperand, EMemoryOrder eOrder ) noexcept { return __atomic_fetch_and( pValue, nOperand, MemoryOrder_Builtin( eOrder ) ); }
	static Word_t FetchOr( Word_t *pValue, Word_t nOperand, EMemoryOrder eOrder ) noexcept { return __atomic_fetch_or( pValue, nOperand, MemoryOrder_Builtin( eOrder ) ); }
	static Word_t FetchXor( Word_t *pValue, Word_t nOperand, EMemoryOrder eOrder ) noexcept { return __atomic_fetch_xor( pValue, nOperand, MemoryOrder_Builtin( eOrder ) ); }

#	endif // defined( BALL_MSVC ) && !defined( BALL_CLANG )
};

/// @brief Standalone fence, ordering the surrounding non-atomic accesses.
inline void AtomicThreadFence( EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept
{
#	if defined( BALL_MSVC ) && !defined( BALL_CLANG )
	( void )eOrder;
	BALL_ATOMIC_FENCE();
#	else // !( defined( BALL_MSVC ) && !defined( BALL_CLANG ) )
	__atomic_thread_fence( MemoryOrder_Builtin( eOrder ) );
#	endif // defined( BALL_MSVC ) && !defined( BALL_CLANG )
}

///-----------------------------------------------------------------------------
/// @brief Atomic storage, chosen by argument count.
///
/// @details One name spells two things. `CAtomic< T >` is a single lock-free
/// atomic value -- the partial specialization right below. `CAtomic< I, Ts... >`,
/// an index type followed by at least one column, is the unsynchronized
/// concurrent SoA container, a second partial specialization that lives in
/// `types/concurrent.hpp`. The primary template itself is never defined.
///
/// @note The first parameter therefore changes meaning with the argument count:
/// `CAtomic< uint32_t >` is an atomic 32-bit value, while
/// `CAtomic< uint32_t, uint64_t >` is a one-column SoA indexed by `uint32_t`.
///-----------------------------------------------------------------------------
template < typename I, typename... Ts > class CAtomic;

///-----------------------------------------------------------------------------
/// @brief Lock-free atomic value of a 1-, 2-, 4- or 8-byte trivially copyable type.
///
/// @details The value is held as an unsigned word of the same width and bit-cast
/// to and from @p T on every access, so integers, enumerations, pointers and
/// floating-point values share one implementation. Read-modify-write arithmetic
/// and bitwise operations are offered for integral and enumeration types only.
///
/// @note Unlike `std::atomic`, `CAtomic` is **copyable and trivially copyable**,
/// because an SoA column must be relocatable: growing, inserting into or erasing
/// from a `CVector` relocates rows with a raw byte copy. Those copies are *not*
/// atomic. A `CAtomic` may therefore be copied only while no other thread can
/// touch it -- inside a container that means under exclusive access, which is
/// exactly what a structural modification already holds.
///
/// @complexity O(1) for every operation; each maps to a single instruction (or a
/// short retry loop on load-linked/store-conditional targets).
///-----------------------------------------------------------------------------
template < typename T >
class CAtomic< T >
{
public:
	using Value_t = T;

private:
	BALL_STATIC_ASSERT( sizeof( T ) == 1 || sizeof( T ) == 2 || sizeof( T ) == 4 || sizeof( T ) == 8, "CAtomic supports 1, 2, 4 and 8 byte values" );
	BALL_STATIC_ASSERT( IS_TRIVIALLY_COPYABLE< T >, "CAtomic requires a trivially copyable value type" );

	using Ops_t = MAtomicOps< sizeof( T ) >;
	using Word_t = typename Ops_t::Word_t;

	// Bit-casting rather than converting keeps one backend for every value kind and
	// preserves the exact object representation of a float or a pointer.
	static constexpr Word_t ToWord( T value ) noexcept { return __builtin_bit_cast( Word_t, value ); }
	static constexpr T FromWord( Word_t nValue ) noexcept { return __builtin_bit_cast( T, nValue ); }

	// Natural alignment is what makes the access single-copy atomic on every target
	// Ball builds for; the SoA substrate aligns a column to alignof( CAtomic< T > ).
	alignas( sizeof( T ) ) Word_t m_nValue;

public:
	/// @complexity O(1). The default constructor value-initializes, so a freshly
	/// grown SoA row starts at a defined zero rather than `std::atomic`'s indeterminate.
	constexpr CAtomic() noexcept : m_nValue( Word_t( 0 ) ) {}
	constexpr CAtomic( T value ) noexcept : m_nValue( ToWord( value ) ) {}

	// Non-atomic by construction; see the note on the class.
	constexpr CAtomic( const CAtomic & ) noexcept = default;
	constexpr CAtomic &operator=( const CAtomic & ) noexcept = default;

	/// @complexity O(1).
	T Load( EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) const noexcept { return FromWord( Ops_t::Load( &m_nValue, eOrder ) ); }
	void Store( T value, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept { Ops_t::Store( &m_nValue, ToWord( value ), eOrder ); }
	T Exchange( T value, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept { return FromWord( Ops_t::Exchange( &m_nValue, ToWord( value ), eOrder ) ); }

	///-----------------------------------------------------------------------------
	/// @brief Replaces the value with @p desired when it still equals @p expected.
	///
	/// @param expected Read back with the value actually found when the swap fails.
	///
	/// @return true when the swap happened.
	///
	/// @complexity O(1) per attempt.
	///-----------------------------------------------------------------------------
	bool CompareExchange( T &expected, T desired, EMemoryOrder eSuccess = EMemoryOrder::SEQ_CST, EMemoryOrder eFailure = EMemoryOrder::SEQ_CST ) noexcept
	{
		Word_t nExpected = ToWord( expected );
		const bool bExchanged = Ops_t::CompareExchange( &m_nValue, nExpected, ToWord( desired ), eSuccess, eFailure );

		if ( !bExchanged )
			expected = FromWord( nExpected );

		return bExchanged;
	}

	/// @complexity O(1). Integral and enumeration values only: arithmetic on a
	/// bit-cast float or pointer word would be meaningless.
	T FetchAdd( T operand, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept requires ( IS_INTEGRAL< T > )
	{
		return FromWord( Ops_t::FetchAdd( &m_nValue, ToWord( operand ), eOrder ) );
	}

	T FetchSub( T operand, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept requires ( IS_INTEGRAL< T > )
	{
		return FromWord( Ops_t::FetchSub( &m_nValue, ToWord( operand ), eOrder ) );
	}

	T FetchAnd( T operand, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept requires ( IS_INTEGRAL< T > )
	{
		return FromWord( Ops_t::FetchAnd( &m_nValue, ToWord( operand ), eOrder ) );
	}

	T FetchOr( T operand, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept requires ( IS_INTEGRAL< T > )
	{
		return FromWord( Ops_t::FetchOr( &m_nValue, ToWord( operand ), eOrder ) );
	}

	T FetchXor( T operand, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept requires ( IS_INTEGRAL< T > )
	{
		return FromWord( Ops_t::FetchXor( &m_nValue, ToWord( operand ), eOrder ) );
	}

	/// @complexity O(1). Sequentially consistent shorthands.
	operator T() const noexcept { return Load(); }
	CAtomic &operator=( T value ) noexcept { Store( value ); return *this; }
	T operator++() noexcept requires ( IS_INTEGRAL< T > ) { return static_cast< T >( FetchAdd( T( 1 ) ) + T( 1 ) ); }
	T operator++( int ) noexcept requires ( IS_INTEGRAL< T > ) { return FetchAdd( T( 1 ) ); }
	T operator--() noexcept requires ( IS_INTEGRAL< T > ) { return static_cast< T >( FetchSub( T( 1 ) ) - T( 1 ) ); }
	T operator--( int ) noexcept requires ( IS_INTEGRAL< T > ) { return FetchSub( T( 1 ) ); }
}; // class CAtomic

/// @brief Detects a `CAtomic` column, so a container can offer per-element atomic
/// operations on exactly the fields that support them.
template < typename T > struct MIsAtomic { static constexpr bool VALUE = false; };
template < typename T > struct MIsAtomic< CAtomic< T > > { static constexpr bool VALUE = true; };

template < typename T > inline constexpr bool IS_ATOMIC = MIsAtomic< T >::VALUE;

/// @brief Value an atomic field carries, or the field's own type when it is plain.
template < typename T > struct MAtomic { using Type = T; };
template < typename T > struct MAtomic< CAtomic< T > > { using Type = T; };

template < typename T > using AtomicValue_t = typename MAtomic< T >::Type;

using AtomicBool_t = CAtomic< bool_t >;
using AtomicChar_t = CAtomic< uchar_t >;
using AtomicUInt16_t = CAtomic< uint16_t >;
using AtomicUInt32_t = CAtomic< uint32_t >;
using AtomicUInt64_t = CAtomic< uint64_t >;
using AtomicSize_t = CAtomic< size_t >;

#endif // !defined( _INCLUDE_BALL_TYPES_ATOMIC_HPP_ )

#ifndef _INCLUDE_BALL_TYPES_HASHMAP_HPP_
#	define _INCLUDE_BALL_TYPES_HASHMAP_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "bits.hpp"
#	include "c/assert.h"
#	include "c/nouniqueaddress.h"
#	include "elements.hpp"
#	include "fixed.hpp"
#	include "hash.hpp"
#	include "meta/fixed.hpp"
#	include "meta/get.hpp"
#	include "meta/indexsequence.hpp"
#	include "meta/indextype.hpp"
#	include "meta/xvalue.hpp"
#	include "vector.hpp"
#	include "reflect.hpp"
#	include "slotiterator.hpp"

///-----------------------------------------------------------------------------
/// @brief Iterates @p map over its occupied slots in storage order, binding each
/// slot index to @p it.
///
/// @details Storage order is bucket order, permuted by hashing -- not key order.
/// @p it holds the slot `Index_t`; read columns with `Get< TN >( it )` (or
/// `Key`/`Value` on `CHashMap`). Safe on an empty map (the body never runs).
///
/// @complexity O(capacity) for the whole loop: one O(1) step per bucket.
///-----------------------------------------------------------------------------
#	define BALL_HASHMAP_FOREACH( map, it ) \
		for ( auto it = ( map ).FirstIndex(); it != ( map ).EndIndex(); it = ( map ).NextIndex( it ) )

///-----------------------------------------------------------------------------
/// @brief Per-slot occupancy state of an open-addressing hash table.
///
/// @details Only two states are needed: a `FREE` slot terminates a probe (the key
/// is absent) and an `OCCUPIED` slot holds a live key/value. Removal keeps the
/// probe chains gap-free by backward-shift deletion (@ref CHashMapImpl::Remove)
/// rather than leaving tombstones, so no third `DELETED` state is required. Stored
/// as a single packed bit per slot; value-initialization (all-zero) yields `FREE`,
/// so a freshly grown table starts all-free for free.
///-----------------------------------------------------------------------------
enum class EHashSlotState : uint1_t
{
	FREE = 0,
	OCCUPIED = 1
};
BALL_FIXED_UNSIGNED_ENUM_TRAIT( EHashSlotState, 1 );

// The key routinely shares a type with a value column (`CHashMap< I, size_t, size_t >`),
// and the SoA substrate lays out same-typed heap columns as one aliasing region.
// Tagging the key gives it a distinct type so it stays its own column, using the
// same reflect-tag idiom `CRBTree` applies to its links. Value columns are
// stored as supplied -- give identically-typed values distinct types yourself
// (e.g. via `BALL_REFLECT_TAGGED`), exactly as `CRBTree` requires.
BALL_REFLECT_TAGGED_TEMPLATE( HashKeyColumn );

///-----------------------------------------------------------------------------
/// @brief Structure-of-arrays open-addressing hash table core (storage, probing,
/// growth), independent of the public facade layered on top.
///
/// @details The payload is the key column @p K followed by an arbitrary value
/// pack @p Ts, each in its own SoA column alongside a per-slot `EHashSlotState`
/// metadata column, all held by a variadic `CBufferVector` sized to the bucket count
/// (a power of two). Collisions are resolved by linear probing; the home bucket
/// of a key is `Hasher_t::Make( key )` mapped through the Fibonacci multiply-shift of the
/// hash policy @p C. The policy is an empty base (zero-cost via EBO; a
/// `BALL_NO_UNIQUE_ADDRESS` member is the alternative when composition is
/// preferred). @p TI indexes the SoA columns; payload columns are reached by
/// compile-time index through `Column_t< TN >` (column 0 is the key), mirroring
/// multi-column `CVector` and `CRBTree`.
///
/// @complexity Lookups/insertions/removals are O(1) expected, O(capacity) worst
/// case under adversarial clustering; a growth rehash is O(capacity).
///-----------------------------------------------------------------------------
template < typename I, I N, typename TI, typename C, typename K, typename... Ts >
class CHashMapBase : public C, public CBufferVector< I, N, EHashSlotState, HashKeyColumn_t< K >, Ts... >
{
public:
	BALL_STATIC_ASSERT( !N || ( N >= 8 && !( N & ( N - 1 ) ) ), "Buffered hash-table capacity must be a power of two >= 8" );

	// The live table stores inline up to @p N rows and then overflows to the heap.
	// Rehash scratch has the same columns but always uses CVector's heap-backed form,
	// so a large inline capacity is never duplicated inside a rehash stack frame;
	// its metadata column simply rides along unused.
	using Base_t = CBufferVector< I, N, EHashSlotState, HashKeyColumn_t< K >, Ts... >;
	using Storage_t = Base_t;
	using ScratchStorage_t = CVector< I, EHashSlotState, HashKeyColumn_t< K >, Ts... >;
	using State_t = EHashSlotState;
	using Hasher_t = C;
	using Key_t = K;
	using Index_t = I;
	using TypeIndex_t = TI;
	using Fixed_t = MFixed< Index_t >;

	/// @brief Type of the @p TN -th payload column (column 0 is the key @p K).
	template < TI TN > using Column_t = typename MIndexType< TI, TN, K, Ts... >::Type;
	using Value_t = Column_t< 0 >;

	/// @brief Total payload columns: the key column plus @p Ts.
	static constexpr size_t COLUMN_COUNT = 1 + sizeof...( Ts );
	/// @brief Payload columns follow the single `EHashSlotState` metadata column.
	static constexpr TI PAYLOAD_COLUMN_OFFSET = static_cast< TI >( Storage_t::TYPE_COUNT - COLUMN_COUNT );
	using ColumnSequence_t = MakeIndexSequence_t< size_t, COLUMN_COUNT >;

	/// @brief Out-of-band "no slot" index (-1); never a valid bucket.
	static constexpr Index_t NIL_INDEX = Fixed_t::INVALID;
	static constexpr Index_t INVALID_INDEX = NIL_INDEX;
	static constexpr Index_t FIRST_INDEX = 0;

	/// @brief Grow once the live load reaches NUM/DEN of capacity.
	static constexpr I LOAD_FACTOR_NUM = 3;
	static constexpr I LOAD_FACTOR_DEN = 4;
	/// @brief Probe length that makes an insert suspect the table is loaded.
	///
	/// @details With no size field, checking the load factor exactly costs a
	/// popcount of the whole state column (@ref Count) -- too dear to pay on every
	/// insert. An insert's own probe walk is a free local load sample: linear
	/// probing at the 3/4 target load has a mean insert probe of about
	/// `(1 + 1/(1-a)^2)/2 = 8.5` slots, so a probe reaching 8 is the cue to run the
	/// exact check. Short probes -- the overwhelming case below the target load --
	/// skip the check entirely.
	static constexpr I PROBE_GROW_THRESHOLD = 8;

	// Access to the hash-policy empty-base subobject, mirroring CRBTree's
	// Comparator(): routing every use through here keeps call sites from naming or
	// casting to the base and gives one place to carry the policy across siblings.
	constexpr Hasher_t &Hasher() noexcept { return *this; }
	constexpr const Hasher_t &Hasher() const noexcept { return *this; }

	constexpr Storage_t &Nodes() noexcept { return *this; }
	constexpr const Storage_t &Nodes() const noexcept { return *this; }

	/// @brief Bucket count is the SoA row count (a power of two, so a whole number
	/// of state bytes).
	/// @complexity O(1).
	constexpr Index_t BucketCount() const noexcept { return Nodes().Count(); }

	///-----------------------------------------------------------------------------
	/// @brief Live element count, recovered from the packed state column.
	///
	/// @details No size field is kept: `OCCUPIED == 1`/`FREE == 0` in the 1-bit
	/// state column, so the number of live entries is exactly the set-bit population
	/// of that bit array (@ref PopCount). The bucket count is a power of two >= 8,
	/// hence a whole number of bytes, so the tail never needs masking.
	///
	/// @complexity O(bucket count / 64): one hardware popcount per 64 buckets.
	///-----------------------------------------------------------------------------
	constexpr Index_t Count() const noexcept
	{
		constexpr size_t BATCH_SIZE = sizeof( ullong_t );
		size_t nBytes = Nodes().template SizeBy< State_t >();

		if ( !nBytes )
			return 0;

		const uchar_t *pStateBits = Nodes().template Packed_BaseBy< State_t >();
		ullong_t nLive = 0;

		do
		{
			const size_t nBatch = nBytes < BATCH_SIZE ? nBytes : BATCH_SIZE;
			ullong_t nWord = 0;

			memcpy( &nWord, pStateBits, nBatch );
			nLive += PopCount( nWord );
			pStateBits += nBatch;
			nBytes -= nBatch;
		}
		while ( nBytes );

		return static_cast< Index_t >( nLive );
	}
	constexpr bool IsEmpty() const noexcept { return !Count(); }

	/// @complexity O(1). Payload column base-pointers and per-slot accessors.
	template < TI TN > constexpr auto PayloadColumn() noexcept { return Get< PAYLOAD_COLUMN_OFFSET + TN >( Nodes() ); }
	template < TI TN > constexpr auto PayloadColumn() const noexcept { return Get< PAYLOAD_COLUMN_OFFSET + TN >( Nodes() ); }

	constexpr State_t State( Index_t i ) const noexcept { return Get< State_t >( Nodes(), i ); }
	constexpr void SetState( Index_t i, State_t eState ) noexcept { Get< State_t >( Nodes(), i ) = eState; }
	constexpr bool IsOccupied( Index_t i ) const noexcept { return State( i ) == State_t::OCCUPIED; }

	// The key column is reflect-tagged, the value columns are raw; ReflectAccess
	// unwraps the former and passes the latter straight through, so one form serves both.
	constexpr Key_t &Key( Index_t i ) noexcept { return ReflectAccess( PayloadColumn< 0 >()[ i ] ); }
	constexpr const Key_t &Key( Index_t i ) const noexcept { return ReflectAccess( PayloadColumn< 0 >()[ i ] ); }
	template < TI TN > constexpr Column_t< TN > &Column( Index_t i ) noexcept { return ReflectAccess( PayloadColumn< TN >()[ i ] ); }
	template < TI TN > constexpr const Column_t< TN > &Column( Index_t i ) const noexcept { return ReflectAccess( PayloadColumn< TN >()[ i ] ); }

	/// @complexity O(N) for a valid buffered table (constructs its inline bucket
	/// rows without allocation); O(1) for the heap-backed form.
	constexpr CHashMapBase() noexcept : CHashMapBase( Hasher_t() ) {}

	constexpr explicit CHashMapBase( const Hasher_t &hasher ) noexcept :
		Hasher_t( hasher ),
		Storage_t()
	{
		// CBufferVector already owns storage for N rows. Make that storage the live
		// bucket array once during construction, so the first Insert neither resets
		// the table nor walks through intermediate capacities. SetCount clears the
		// newly exposed packed state bits to FREE and performs no allocation for N.
		if constexpr ( N )
			Nodes().SetCount( N );
	}

	~CHashMapBase() noexcept = default;

protected:
	// The bucket count (the SoA row count) is a power of two, so IndexForCapacity
	// derives the multiply-shift width (log2) from it directly -- no cached
	// capacity-bits member is kept.
	/// @complexity O(1): hash the key, then one multiply-shift onto the table.
	constexpr Index_t HomeBucket( const Key_t &key, Index_t nCapacity ) const noexcept
	{
		const auto nHash = Hasher_t::Make( key );

		// The usual CBufferHashMap lookup stays inside its compile-time N buckets.
		// Feed Index a constant width in that case: this removes capacity log2 from
		// every Find without storing another field. After heap overflow, fall back to
		// the run-time-capacity path.
		if constexpr ( N )
		{
			if ( nCapacity == N )
			{
				constexpr bits_t INLINE_INDEX_BITS = static_cast< bits_t >( BitWidth_Const( N ) );

				return Hasher_t::template Index< Index_t >( nHash, INLINE_INDEX_BITS );
			}
		}

		return Hasher_t::template IndexForCapacity< Index_t >( nHash, nCapacity );
	}

	///-----------------------------------------------------------------------------
	/// @brief Drops the table to @p nNewCapacity empty buckets (a power of two).
	///
	/// @details Grows/shrinks the SoA storage (its row count becomes the bucket
	/// count), marks every slot `FREE`, and resets the element count. Stale key/value
	/// cells left by a grow are ignored while their slot reads `FREE`.
	///
	/// @complexity O(nNewCapacity).
	///-----------------------------------------------------------------------------
	void ResetTable( Index_t nNewCapacity )
	{
		Nodes().SetCount( nNewCapacity );
		memset( Nodes().template Packed_BaseBy< State_t >(), 0, Nodes().template SizeBy< State_t >() );
	}

	static constexpr bool IsOccupiedBit( const uchar_t *pStateBits, Index_t i ) noexcept
	{
		return pStateBits[ i >> 3 ] & static_cast< uchar_t >( 1u << ( i & 7 ) );
	}

	template < typename TKeyColumn >
	constexpr Index_t FindInTable( const Key_t &key, Index_t nCapacity, Index_t iHome, const uchar_t *pStateBits, const TKeyColumn *pKeys ) const noexcept
	{
		const Index_t nMask = static_cast< Index_t >( nCapacity - 1 );
		Index_t i = iHome;

		for ( ;; )
		{
			if ( !IsOccupiedBit( pStateBits, i ) )
				return NIL_INDEX;

			if ( ReflectAccess( pKeys[ i ] ) == key )
				return i;

			i = static_cast< Index_t >( ( i + 1 ) & nMask );

			if ( i == iHome )
				return NIL_INDEX;
		}
	}

	///-----------------------------------------------------------------------------
	/// @brief Locates the slot a key belongs in, distinguishing present from absent.
	///
	/// @details Linear probe from the home bucket: stops at the first `FREE` slot
	/// (absent -- the key is inserted there) or at the matching `OCCUPIED` key
	/// (present). With no tombstones there is no separate reuse slot to track.
	/// @p nProbeLength reports how far the walk went (0 = landed on the home
	/// bucket); the insert path reads it as a free local load sample (see
	/// @ref PROBE_GROW_THRESHOLD). `NIL_INDEX` with `bExists == false` means the
	/// full cycle found neither the key nor a free slot: the table is completely
	/// full and must grow before this key can be placed.
	///
	/// @complexity O(1) expected, O(capacity) worst case.
	///-----------------------------------------------------------------------------
	constexpr Index_t LocateForInsert( const Key_t &key, bool &bExists, Index_t &nProbeLength ) const noexcept
	{
		const Index_t nCapacity = BucketCount();
		const Index_t nMask = static_cast< Index_t >( nCapacity - 1 );
		const Index_t iHome = HomeBucket( key, nCapacity );
		const uchar_t *pStateBits = Nodes().template Packed_BaseBy< State_t >();
		const auto *pKeys = PayloadColumn< 0 >();
		Index_t i = iHome;

		nProbeLength = 0;

		for ( ;; )
		{
			if ( !IsOccupiedBit( pStateBits, i ) )
			{
				bExists = false;

				return i;
			}

			if ( ReflectAccess( pKeys[ i ] ) == key )
			{
				bExists = true;

				return i;
			}

			i = static_cast< Index_t >( ( i + 1 ) & nMask );
			++nProbeLength;

			if ( i == iHome )
				break;
		}

		bExists = false;

		return NIL_INDEX;
	}

	///-----------------------------------------------------------------------------
	/// @brief Returns the occupied slot holding @p key, or `NIL_INDEX` if absent.
	///
	/// @complexity O(1) expected, O(capacity) worst case.
	///-----------------------------------------------------------------------------
	constexpr Index_t FindSlot( const Key_t &key ) const noexcept
	{
		const Index_t nCapacity = BucketCount();

		if ( !nCapacity )
			return NIL_INDEX;

		if constexpr ( N )
		{
			if ( nCapacity == N )
			{
				constexpr bits_t INLINE_INDEX_BITS = static_cast< bits_t >( BitWidth_Const( N ) );
				const Index_t iHome = Hasher_t::template Index< Index_t >( Hasher_t::Make( key ), INLINE_INDEX_BITS );
				const uchar_t *pStateBits = Nodes().template Packed_FixedDataBy< State_t >();
				const auto *pKeys = Nodes().template FixedDataBy< HashKeyColumn_t< K > >();

				return FindInTable( key, nCapacity, iHome, pStateBits, pKeys );
			}
		}

		const Index_t iHome = HomeBucket( key, nCapacity );
		const uchar_t *pStateBits = Nodes().template Packed_BaseBy< State_t >();
		const auto *pKeys = PayloadColumn< 0 >();

		return FindInTable( key, nCapacity, iHome, pStateBits, pKeys );
	}

	// Assign a freshly-supplied row into slot i column by column. The slots are
	// pre-constructed (a grow value-initializes every cell), so this assigns rather
	// than constructing; the tagged key column takes the raw key through CReflect's
	// value assignment, the value columns take theirs directly.
	template < TI COLUMN = 0, typename U0, typename... URest >
	constexpr void AssignFrom( Index_t i, U0 &&u0, URest &&...urest )
	{
		PayloadColumn< COLUMN >()[ i ] = Forward< U0 >( u0 );

		if constexpr ( sizeof...( URest ) > 0 )
			AssignFrom< static_cast< TI >( COLUMN + 1 ) >( i, Forward< URest >( urest )... );
	}

	///-----------------------------------------------------------------------------
	/// @brief Writes a new (absent) key/value row into the located slot @p i.
	///
	/// @complexity O(1) (fixed column count).
	///-----------------------------------------------------------------------------
	template < typename... Us >
	constexpr void PlaceElement( Index_t i, Us &&...values )
	{
		AssignFrom< 0 >( i, Forward< Us >( values )... );
		SetState( i, State_t::OCCUPIED );
	}

	// Move-assign every payload column of slot @p iFrom onto slot @p iTo (both live
	// rows); used by backward-shift deletion to slide an entry back into a hole.
	template < size_t... Is >
	constexpr void MoveRow( Index_t iFrom, Index_t iTo, MIndexSequence< size_t, Is... > )
	{
		( ( PayloadColumn< static_cast< TI >( Is ) >()[ iTo ] = Move( PayloadColumn< static_cast< TI >( Is ) >()[ iFrom ] ) ), ... );
	}

	///-----------------------------------------------------------------------------
	/// @brief Removes the entry at @p iRemove and closes the probe-chain gap.
	///
	/// @details Backward-shift (Knuth 6.4R) deletion: walk forward from the hole and
	/// slide back each following entry whose home bucket lets it legally occupy the
	/// hole, until a `FREE` slot ends the cluster. This leaves every probe chain
	/// contiguous, so `FREE`/`OCCUPIED` alone suffice -- no tombstone is written.
	///
	/// @complexity O(cluster length); O(1) expected under the load factor.
	///-----------------------------------------------------------------------------
	constexpr void EraseSlot( Index_t iRemove )
	{
		const Index_t nCapacity = BucketCount();
		const Index_t nMask = static_cast< Index_t >( nCapacity - 1 );
		const uchar_t *pStateBits = Nodes().template Packed_BaseBy< State_t >();
		const auto *pKeys = PayloadColumn< 0 >();
		Index_t iHole = iRemove;

		for ( ;; )
		{
			Index_t i = iHole;

			for ( ;; )
			{
				i = static_cast< Index_t >( ( i + 1 ) & nMask );

				if ( !IsOccupiedBit( pStateBits, i ) )
				{
					SetState( iHole, State_t::FREE );

					return;
				}

				const Index_t iCurHome = HomeBucket( ReflectAccess( pKeys[ i ] ), nCapacity );

				// Move the entry at i into the hole iff the hole lies on its probe
				// path, i.e. its home is NOT cyclically within the open range (iHole, i].
				const bool bStay = iHole < i ? ( iHole < iCurHome && iCurHome <= i ) : ( iCurHome > iHole || iCurHome <= i );

				if ( !bStay )
					break;
			}

			MoveRow( i, iHole, ColumnSequence_t() );
			iHole = i;
		}
	}

	// Move one live row's payload from slot @p i into the raw scratch row @p w
	// (rehash step 1). The scratch's payload rows are reserved as raw memory
	// (SetCountRaw), so each cell is move-CONSTRUCTED, not assigned; the scratch
	// mirrors the table's payload columns (tagged key, raw values), so each moves
	// straight into the matching column. The scratch's metadata column stays raw and
	// unread (EHashSlotState is trivially destructible, so it needs no construction).
	template < size_t... Is >
	void CollectRow( ScratchStorage_t &temp, Index_t w, Index_t i, MIndexSequence< size_t, Is... > )
	{
		( ConstructElement( &Get< PAYLOAD_COLUMN_OFFSET + static_cast< TI >( Is ) >( temp )[ w ], Move( PayloadColumn< static_cast< TI >( Is ) >()[ i ] ) ), ... );
	}

	// Re-insert one collected row into the freshly sized empty table (rehash step 3).
	// ReflectAccess unwraps the tagged key and passes raw values through unchanged;
	// payload columns sit past the shared metadata column at PAYLOAD_COLUMN_OFFSET.
	template < size_t... Is >
	void ReinsertRow( ScratchStorage_t &temp, Index_t r, MIndexSequence< size_t, Is... > )
	{
		bool bExists = false;
		Index_t nProbeLength = 0; // Unused: the fresh table is under the target load by construction.
		const Index_t iSlot = LocateForInsert( ReflectAccess( Get< PAYLOAD_COLUMN_OFFSET >( temp )[ r ] ), bExists, nProbeLength );

		PlaceElement( iSlot, Move( ReflectAccess( Get< PAYLOAD_COLUMN_OFFSET + static_cast< TI >( Is ) >( temp )[ r ] ) )... );
	}

	///-----------------------------------------------------------------------------
	/// @brief Rebuilds the table at @p nNewCapacity buckets, re-probing every key.
	///
	/// @details Live rows are snapshotted into a scratch table whose rows are
	/// reserved raw in a single `EnsureCapacity` allocation sized to the exact
	/// live-row count (`SetCountRaw`), then move-constructed into place, so
	/// collecting neither re-grows the scratch nor value-initializes it. The table
	/// is reset to the new (power-of-two) size and every row is re-probed into its
	/// new home. Re-hashing recomputes bucket indices from the keys; nothing caches
	/// a stale hash. Uses heap scratch, so this is a run-time-only path.
	///
	/// @complexity O(capacity + nNewCapacity).
	///-----------------------------------------------------------------------------
	void Rehash( Index_t nNewCapacity )
	{
		const Index_t nRows = Count();

		ScratchStorage_t temp;

		temp.SetCountRaw( nRows );

		const Index_t nBuckets = BucketCount();
		const uchar_t *pStateBits = Nodes().template Packed_BaseBy< State_t >();

		for ( Index_t i = 0, w = 0; i < nBuckets; ++i )
		{
			if ( IsOccupiedBit( pStateBits, i ) )
				CollectRow( temp, w++, i, ColumnSequence_t() );
		}

		ResetTable( nNewCapacity );

		for ( Index_t r = 0; r < nRows; ++r )
			ReinsertRow( temp, r, ColumnSequence_t() );
	}

	///-----------------------------------------------------------------------------
	/// @brief Exact load check: is the table at or past the NUM/DEN target load?
	///
	/// @details Runs the full popcount (@ref Count), so the insert path only calls
	/// it when a long probe has already hinted at high load -- never on the fast
	/// path (see @ref PROBE_GROW_THRESHOLD).
	///
	/// @complexity O(capacity / 64): one popcount of the state column.
	///-----------------------------------------------------------------------------
	bool ShouldGrow() const noexcept
	{
		// Widen the load arithmetic so small index types (e.g. size8_t) do not
		// overflow the capacity * NUM product.
		const size_t nUsed = static_cast< size_t >( Count() ) + 1u;
		const size_t nCapacity = static_cast< size_t >( BucketCount() );

		return nUsed * LOAD_FACTOR_DEN >= nCapacity * LOAD_FACTOR_NUM;
	}

	/// @brief Doubles the table (rehashing every key), guarding the index type.
	///
	/// @complexity O(capacity).
	void GrowTable()
	{
		const size_t nWide = static_cast< size_t >( BucketCount() ) * 2u;
		const Index_t nNewCapacity = static_cast< Index_t >( nWide );

		BALL_ASSERT_MESSAGE( nNewCapacity && static_cast< size_t >( nNewCapacity ) == nWide, "Hash table capacity overflowed its index type" );
		Rehash( nNewCapacity );
	}
};

///-----------------------------------------------------------------------------
/// @brief Public unique-key facade over `CHashMapBase`.
///
/// @details Insert probes for the key (rejecting duplicates), places it in the
/// first free slot, and grows past the load factor; find/remove probe the same
/// way, with remove closing the probe-chain gap by backward-shift deletion (no
/// tombstones). Cross-capacity copy/move rebuild the table by re-inserting every
/// row, mirroring `CRBTreeImpl`.
///-----------------------------------------------------------------------------
template < typename I, I N, typename TI, typename C, typename K, typename... Ts >
class CHashMapImpl : public CHashMapBase< I, N, TI, C, K, Ts... >
{
private:
	using Base_t = CHashMapBase< I, N, TI, C, K, Ts... >;
	using ColumnSequence_t = MakeIndexSequence_t< size_t, 1 + sizeof...( Ts ) >;

	using Base_t::LocateForInsert;
	using Base_t::PlaceElement;
	using Base_t::FindSlot;
	using Base_t::ShouldGrow;
	using Base_t::GrowTable;
	using Base_t::ResetTable;
	using Base_t::EraseSlot;
	using Base_t::State;
	using Base_t::SetState;
	using Base_t::Nodes;

public:
	using typename Base_t::Index_t;
	using typename Base_t::TypeIndex_t;
	using typename Base_t::Key_t;
	using typename Base_t::Value_t;
	using typename Base_t::Hasher_t;
	using typename Base_t::State_t;
	using Base_t::Base_t;
	using Base_t::NIL_INDEX; using Base_t::INVALID_INDEX; using Base_t::FIRST_INDEX;
	using Base_t::Count; using Base_t::IsEmpty;
	using Base_t::BucketCount;
	using Base_t::IsOccupied;
	using Base_t::Key;
	using Base_t::Hasher;
	using Base_t::COLUMN_COUNT;

	template < TI TN > using Column_t = typename Base_t::template Column_t< TN >;

	/// @brief Per-column row access by column index (column 0 is the key).
	///
	/// @complexity O(1).
	template < TI TN > constexpr Column_t< TN > &Get( Index_t i ) { return Base_t::template Column< TN >( i ); }
	template < TI TN > constexpr const Column_t< TN > &Get( Index_t i ) const { return Base_t::template Column< TN >( i ); }

	// Forward iterator over occupied slots, shared with the red-black tree
	// (@ref CSlotIterator). This map owner supplies the traversal contract --
	// Index_t/Key_t, NIL_INDEX, Key(), IsOccupied() and NextIndex() (the next
	// occupied slot). It is forward-only, so it provides no PrevIndex and never
	// instantiates the iterator's operator--.
	using iterator = CSlotIterator< CHashMapImpl, false >;
	using const_iterator = CSlotIterator< CHashMapImpl, true >;

	/// @complexity O(1). Iteration boundary helpers over occupied slots.
	constexpr Index_t EndIndex() const noexcept { return NIL_INDEX; }

	constexpr Index_t FirstIndex() const noexcept
	{
		const Index_t nBuckets = BucketCount();

		for ( Index_t i = 0; i < nBuckets; ++i )
		{
			if ( State( i ) == State_t::OCCUPIED )
				return i;
		}

		return NIL_INDEX;
	}

	constexpr Index_t NextIndex( Index_t i ) const noexcept
	{
		const Index_t nBuckets = BucketCount();

	for ( ++i; i < nBuckets; ++i )
		{
			if ( State( i ) == State_t::OCCUPIED )
				return i;
		}

		return NIL_INDEX;
	}

	constexpr iterator Iterator( Index_t iSlot ) noexcept { return iterator( iSlot, this ); }
	constexpr const_iterator Iterator( Index_t iSlot ) const noexcept { return const_iterator( iSlot, this ); }

	constexpr iterator begin() noexcept { return iterator( FirstIndex(), this ); }
	constexpr iterator end() noexcept { return iterator( EndIndex(), this ); }
	constexpr const_iterator begin() const noexcept { return const_iterator( FirstIndex(), this ); }
	constexpr const_iterator end() const noexcept { return const_iterator( EndIndex(), this ); }
	constexpr const_iterator cbegin() const noexcept { return const_iterator( FirstIndex(), this ); }
	constexpr const_iterator cend() const noexcept { return const_iterator( EndIndex(), this ); }

	///-----------------------------------------------------------------------------
	/// @brief Empties the table without releasing the bucket storage.
	///
	/// @complexity O(capacity / 8) at run time; O(capacity) during constant evaluation.
	///-----------------------------------------------------------------------------
	constexpr void Clear()
	{
		const Index_t nBuckets = BucketCount();

		if ( IsConstantEvaluated() )
		{
			for ( Index_t i = 0; i < nBuckets; ++i )
				SetState( i, State_t::FREE );
		}
		else if ( nBuckets )
		{
			memset( Nodes().template Packed_BaseBy< State_t >(), 0, Nodes().template SizeBy< State_t >() );
		}
	}

	///-----------------------------------------------------------------------------
	/// @brief Inserts a unique key with its values; a duplicate key is rejected.
	///
	/// @return The slot index of the new row, or `NIL_INDEX` if @p key was present.
	///
	/// @details The fast path is check-free: a short probe proves the neighborhood
	/// is sparse and the row is placed at once. Only a probe reaching
	/// `PROBE_GROW_THRESHOLD` -- the local symptom of high load -- pays for the
	/// exact popcount load check (@ref Base_t::ShouldGrow), and only a confirmed
	/// loaded table grows and re-probes. A long probe on an under-loaded table
	/// (e.g. one adversarial cluster) places without growing, since doubling would
	/// not break such a cluster apart.
	///
	/// @complexity O(1) expected, amortized over the growth rehashes.
	///-----------------------------------------------------------------------------
	constexpr Index_t Insert( K key, Ts... values )
	{
		if ( !BucketCount() )
			// Eight one-bit states occupy exactly one byte: this is the smallest
			// table supported by Count's whole-byte packed-state invariant.
			ResetTable( static_cast< Index_t >( sizeof( uchar_t ) * 8u ) );

		for ( ;; )
		{
			bool bExists = false;
			Index_t nProbeLength = 0;
			const Index_t iSlot = LocateForInsert( key, bExists, nProbeLength );

			if ( bExists )
				return NIL_INDEX;

			// NIL means completely full (grow unconditionally); otherwise grow only
			// when the long probe is confirmed by the exact load check.
			if ( iSlot != NIL_INDEX && ( nProbeLength < Base_t::PROBE_GROW_THRESHOLD || !ShouldGrow() ) )
			{
				PlaceElement( iSlot, Move( key ), Move( values )... );

				return iSlot;
			}

			GrowTable();
		}
	}
	constexpr iterator InsertIterator( K key, Ts... values ) { return Iterator( Insert( Move( key ), Move( values )... ) ); }

	/// @complexity O(1) expected for Find/Contains and their iterator forms.
	constexpr Index_t Find( const Key_t &key ) const noexcept { return FindSlot( key ); }
	constexpr bool Contains( const Key_t &key ) const noexcept { return FindSlot( key ) != NIL_INDEX; }
	constexpr iterator FindIterator( const Key_t &key ) noexcept { return Iterator( Find( key ) ); }
	constexpr const_iterator FindIterator( const Key_t &key ) const noexcept { return Iterator( Find( key ) ); }

	///-----------------------------------------------------------------------------
	/// @brief Removes @p key if present, closing the probe-chain gap it leaves.
	///
	/// @return true when a row was removed.
	///
	/// @details Backward-shift deletion (@ref Base_t::EraseSlot) slides the trailing
	/// cluster entries back over the hole, so the table needs no tombstones.
	///
	/// @complexity O(1) expected, O(cluster length) worst case.
	///-----------------------------------------------------------------------------
	constexpr bool Remove( const Key_t &key )
	{
		const Index_t iSlot = FindSlot( key );

		if ( iSlot == NIL_INDEX )
			return false;

		EraseSlot( iSlot );

		return true;
	}

private:
	template < typename Other, size_t... Is >
	constexpr void CopyRowFrom( const Other &other, Index_t i, MIndexSequence< size_t, Is... > )
	{
		Insert( other.template Get< static_cast< TI >( Is ) >( i )... );
	}

	template < typename Other, size_t... Is >
	constexpr void MoveRowFrom( Other &other, Index_t i, MIndexSequence< size_t, Is... > )
	{
		Insert( Move( other.template Get< static_cast< TI >( Is ) >( i ) )... );
	}

public:
	/// @complexity O(m + n): clear m current rows, then insert the n source rows.
	template < I ON >
	constexpr CHashMapImpl &CopyFrom( const CHashMapImpl< I, ON, TI, C, K, Ts... > &other )
	{
		if ( reinterpret_cast< const void * >( this ) == reinterpret_cast< const void * >( &other ) )
			return *this;

		Hasher() = other.Hasher();
		Clear();

		for ( Index_t i = other.FirstIndex(); i != other.EndIndex(); i = other.NextIndex( i ) )
			CopyRowFrom( other, i, ColumnSequence_t() );

		return *this;
	}

	/// @complexity O(m + n): a structural rebuild, not a pointer steal.
	template < I ON >
	constexpr CHashMapImpl &MoveFrom( CHashMapImpl< I, ON, TI, C, K, Ts... > &&other )
	{
		if ( reinterpret_cast< const void * >( this ) == reinterpret_cast< const void * >( &other ) )
			return *this;

		Hasher() = Move( other.Hasher() );
		Clear();

		for ( Index_t i = other.FirstOccupied(); i != other.EndIndex(); i = other.NextOccupied( i ) )
			MoveRowFrom( other, i, ColumnSequence_t() );

		other.Clear();

		return *this;
	}

	template < I ON > constexpr CHashMapImpl( const CHashMapImpl< I, ON, TI, C, K, Ts... > &other ) { CopyFrom( other ); }
	template < I ON > constexpr CHashMapImpl &operator=( const CHashMapImpl< I, ON, TI, C, K, Ts... > &other ) { return CopyFrom( other ); }
	template < I ON > constexpr CHashMapImpl( CHashMapImpl< I, ON, TI, C, K, Ts... > &&other ) { MoveFrom( Move( other ) ); }
	template < I ON > constexpr CHashMapImpl &operator=( CHashMapImpl< I, ON, TI, C, K, Ts... > &&other ) { return MoveFrom( Move( other ) ); }

	// The SoA storage base declares a move, which deletes its implicit copy; define
	// the same-type special members explicitly so this class (and the wrappers that
	// derive from it) stay copyable/movable, rebuilding through CopyFrom/MoveFrom
	// rather than a raw member-wise copy.
	constexpr CHashMapImpl( const CHashMapImpl &other ) { CopyFrom( other ); }
	constexpr CHashMapImpl &operator=( const CHashMapImpl &other ) { return CopyFrom( other ); }
	constexpr CHashMapImpl( CHashMapImpl &&other ) { MoveFrom( Move( other ) ); }
	constexpr CHashMapImpl &operator=( CHashMapImpl &&other ) { return MoveFrom( Move( other ) ); }
};

template < typename I, I N, typename K, typename C, typename... Ts > class CBufferHashMap;

///-----------------------------------------------------------------------------
/// @brief Heap-backed multi-column hash table: key column @p K plus value columns
/// @p Ts (a hash set when @p Ts is empty, an SoA multi-map otherwise).
///-----------------------------------------------------------------------------
template < typename I = size32_t, typename K = I, typename C = CFibonacciHash< I >, typename... Ts >
class CHashMap : public CHashMapImpl< I, 0, size8_t, C, K, Ts... >
{
public:
	using Base_t = CHashMapImpl< I, 0, size8_t, C, K, Ts... >;
	using Base_t::Base_t;
	using Base_t::Get;
	using Base_t::CopyFrom; using Base_t::MoveFrom;
	using typename Base_t::Index_t;
	using typename Base_t::Key_t;

	constexpr Key_t &Key( Index_t i ) { return Base_t::template Get< 0 >( i ); }
	constexpr const Key_t &Key( Index_t i ) const { return Base_t::template Get< 0 >( i ); }

	/// @complexity O(m + n): cross-form copy/move conversions rebuild the table.
	template < I N > constexpr CHashMap( const CBufferHashMap< I, N, K, C, Ts... > &other ) { CopyFrom( other ); }
	template < I N > constexpr CHashMap &operator=( const CBufferHashMap< I, N, K, C, Ts... > &other ) { return CopyFrom( other ); }
	template < I N > constexpr CHashMap( CBufferHashMap< I, N, K, C, Ts... > &&other ) { MoveFrom( Move( other ) ); }
	template < I N > constexpr CHashMap &operator=( CBufferHashMap< I, N, K, C, Ts... > &&other ) { return MoveFrom( Move( other ) ); }
};

/// @brief Fixed-capacity (inline buffer) counterpart of `CHashMap`. @p N is
/// the inline bucket capacity and must be a power of two >= 8.
template < typename I, I N, typename K = I, typename C = CFibonacciHash< I >, typename... Ts >
class CBufferHashMap : public CHashMapImpl< I, N, size8_t, C, K, Ts... >
{
public:
	using Base_t = CHashMapImpl< I, N, size8_t, C, K, Ts... >;
	using Base_t::Base_t;
	using Base_t::CopyFrom; using Base_t::MoveFrom;

	constexpr CBufferHashMap( const CHashMap< I, K, C, Ts... > &other ) { CopyFrom( other ); }
	constexpr CBufferHashMap &operator=( const CHashMap< I, K, C, Ts... > &other ) { return CopyFrom( other ); }
	constexpr CBufferHashMap( CHashMap< I, K, C, Ts... > &&other ) { MoveFrom( Move( other ) ); }
	constexpr CBufferHashMap &operator=( CHashMap< I, K, C, Ts... > &&other ) { return MoveFrom( Move( other ) ); }
};

/// Convenience spellings with the hash policy fixed to `CFibonacciHash< I >`
/// (use the classes directly for a custom policy).
///
/// Key @p K plus value columns @p Ts (no @p Ts is a hash set).
template < typename I = size32_t, typename K = I, typename... Ts > using HashMap_t = CHashMap< I, K, CFibonacciHash< I >, Ts... >;
template < typename K, typename... Ts > using HashMap8_t = CHashMap< size8_t, K, CFibonacciHash< size8_t >, Ts... >;
template < typename K, typename... Ts > using HashMap16_t = CHashMap< size16_t, K, CFibonacciHash< size16_t >, Ts... >;
template < typename K, typename... Ts > using HashMap32_t = CHashMap< size32_t, K, CFibonacciHash< size32_t >, Ts... >;
template < typename K, typename... Ts > using HashMap64_t = CHashMap< size64_t, K, CFibonacciHash< size64_t >, Ts... >;

template < size_t N, typename K = size_t, typename... Ts > using BufferHashMap_t = CBufferHashMap< size_t, N, K, CFibonacciHash< size_t >, Ts... >;
template < size8_t N, typename K = size8_t, typename... Ts > using BufferHashMap8_t = CBufferHashMap< size8_t, N, K, CFibonacciHash< size8_t >, Ts... >;
template < size16_t N, typename K = size16_t, typename... Ts > using BufferHashMap16_t = CBufferHashMap< size16_t, N, K, CFibonacciHash< size16_t >, Ts... >;
template < size32_t N, typename K = size32_t, typename... Ts > using BufferHashMap32_t = CBufferHashMap< size32_t, N, K, CFibonacciHash< size32_t >, Ts... >;
template < size64_t N, typename K = size64_t, typename... Ts > using BufferHashMap64_t = CBufferHashMap< size64_t, N, K, CFibonacciHash< size64_t >, Ts... >;

#endif // !defined( _INCLUDE_BALL_TYPES_HASHMAP_HPP_ )

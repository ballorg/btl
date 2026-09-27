#ifndef _INCLUDE_BALL_TYPES_CONCURRENT_HPP_
#	define _INCLUDE_BALL_TYPES_CONCURRENT_HPP_

#	pragma once

///-----------------------------------------------------------------------------
/// @brief Synchronization policy: one exclusive mutex, readers included.
///
/// @details Every access -- read or write -- serializes. Cheapest state and the
/// simplest reasoning; pick it when reads are rare or critical sections are tiny.
///-----------------------------------------------------------------------------
struct CMutexSynchronization
{
	using Mutex_t = CMutex;

	/// @brief Whether read access actually runs concurrently.
	static constexpr bool SHARED_READS = false;
};

///-----------------------------------------------------------------------------
/// @brief Synchronization policy: concurrent readers, exclusive writers.
///
/// @details The default. Readers share the SoA; a writer excludes everyone.
///-----------------------------------------------------------------------------
struct CSharedMutexSynchronization
{
	using Mutex_t = CSharedMutex;

	static constexpr bool SHARED_READS = true;
};

///-----------------------------------------------------------------------------
/// @brief Synchronization policy: no locking at all.
///
/// @details For a container whose fields are all `CAtomic< U >` and whose shape
/// never changes while it is shared: element operations carry their own ordering,
/// so no lock is left to take. The caller owns the remaining obligation -- a structural
/// modification (push, erase, resize, sort, swap, clear) must not run while any
/// other thread is inside the container, because it relocates rows.
///-----------------------------------------------------------------------------
struct CAtomicSynchronization
{
	using Mutex_t = CNullMutex;

	static constexpr bool SHARED_READS = true;
};

///-----------------------------------------------------------------------------
/// @brief Thread-safe structure-of-arrays container over one shared `CBufferVector`.
///
/// @details The object is exactly two things: the SoA storage and the one
/// synchronization primitive that guards it. Nothing is cached, mirrored or
/// derived into a second field -- the row count, the emptiness, the capacity and
/// every element come out of the storage itself, read under a lock. There is no
/// parallel state model to keep reconciled, and no per-element or per-column lock:
/// one domain covers the whole SoA, so no thread can observe one column updated
/// and another not.
///
/// Access comes in two shapes. The one-shot members (`PushBack`, `Size`, `Erase`
/// and friends) take and release the lock themselves, which is enough when the
/// operation is self-contained. Anything whose outcome depends on state it just
/// read -- the classic check-then-act -- must instead take an access object
/// (@ref ReadAccess, @ref WriteAccess) and perform the whole sequence inside its
/// lifetime, which is one synchronization transaction.
///
/// Fields declared `CAtomic< U >` additionally support per-element atomic
/// operations. Those hold *shared* access for their duration: the shared lock is
/// what pins the rows in place, and the atomic instruction is what orders the
/// element against other threads. They do not make a multi-field update atomic --
/// see @ref SetRow for that.
///
/// @note `Ts...` are the SoA columns and must be pairwise distinct types, the same
/// rule every Ball SoA container follows: the substrate addresses columns by type,
/// so two columns of one type would alias. Wrap duplicates with
/// `BALL_REFLECT_TAGGED(_TEMPLATE)`.
///
/// @complexity See the individual operations. Every one of them adds exactly one
/// lock acquire and release to the cost of the underlying SoA operation.
///-----------------------------------------------------------------------------
template < typename TSync, typename I, I N, typename... Ts >
class CConcurrentBase
{
public:
	/// @brief The single SoA storage; the only authoritative state in the object.
	using Storage_t = CBufferVector< I, N, Ts... >;
	using Index_t = I;
	using Sync_t = TSync;
	using Mutex_t = typename TSync::Mutex_t;
	using TypeIndex_t = size8_t;

	/// @brief Number of SoA columns.
	static constexpr TypeIndex_t FIELD_COUNT = static_cast< TypeIndex_t >( sizeof...( Ts ) );

	/// @brief Type stored in the @p FIELD -th column.
	template < TypeIndex_t FIELD > using Field_t = typename MIndexType< TypeIndex_t, FIELD, Ts... >::Type;

	/// @brief Value the @p FIELD -th column carries: an atomic column unwraps to its payload.
	template < TypeIndex_t FIELD > using FieldValue_t = AtomicValue_t< Field_t< FIELD > >;

	/// @brief Whether the @p FIELD -th column supports per-element atomic operations.
	template < TypeIndex_t FIELD > static constexpr bool FIELD_IS_ATOMIC = IS_ATOMIC< Field_t< FIELD > >;

	/// @brief Whether read access is genuinely concurrent under the chosen policy.
	static constexpr bool SHARED_READS = TSync::SHARED_READS;

	/// @brief Out-of-band "no row" index.
	static constexpr Index_t INVALID_INDEX = MFixed< I >::INVALID;

private:
	BALL_STATIC_ASSERT( sizeof...( Ts ) > 0, "A concurrent SoA needs at least one column" );

	using FieldSequence_t = MakeIndexSequence_t< size_t, sizeof...( Ts ) >;

	/// @brief Whether a column is bit-packed rather than a plain array.
	template < typename T > static constexpr bool IS_PACKED_FIELD = MFixedMetadata< RemoveCV_t< T > >::IS_PACKED;

	///-----------------------------------------------------------------------------
	/// @brief Reference (or packed proxy) to one cell, addressed by column index.
	///
	/// @details Column index rather than column type: positional addressing is what
	/// keeps two same-typed columns apart, exactly as `CVector`'s own row assignment
	/// does. A packed column has no addressable element, so it routes through the
	/// substrate's proxy instead.
	///
	/// @complexity O(1).
	///-----------------------------------------------------------------------------
	template < TypeIndex_t FIELD, typename TStorage >
	static constexpr decltype( auto ) FieldAt( TStorage &data, Index_t i ) noexcept
	{
		using Column_t = Field_t< FIELD >;

		if constexpr ( IS_PACKED_FIELD< Column_t > )
			return data.template Get< Column_t >( i );
		else
			return data.template BaseBy< FIELD >()[ i ];
	}

	template < TypeIndex_t FIELD >
	static void SetFieldAt( Storage_t &data, Index_t i, const Field_t< FIELD > &value ) noexcept
	{
		using Column_t = Field_t< FIELD >;

		if constexpr ( IS_PACKED_FIELD< Column_t > )
			data.template SetTo< Column_t >( i, value );
		else
			data.template BaseBy< FIELD >()[ i ] = value;
	}

	/// @brief Exchanges every column of two rows, so the SoA invariant is preserved.
	template < size_t... FIELDS >
	static void SwapRowFields( Storage_t &data, Index_t left, Index_t right, MIndexSequence< size_t, FIELDS... > ) noexcept
	{
		( SwapOneField< static_cast< TypeIndex_t >( FIELDS ) >( data, left, right ), ... );
	}

	template < TypeIndex_t FIELD >
	static void SwapOneField( Storage_t &data, Index_t left, Index_t right ) noexcept
	{
		using Column_t = Field_t< FIELD >;

		if constexpr ( IS_PACKED_FIELD< Column_t > )
		{
			const Column_t valueLeft = static_cast< Column_t >( data.template Get< Column_t >( left ) );
			const Column_t valueRight = static_cast< Column_t >( data.template Get< Column_t >( right ) );

			data.template SetTo< Column_t >( left, valueRight );
			data.template SetTo< Column_t >( right, valueLeft );
		}
		else
		{
			// A plain byte-level exchange: an atomic column is swapped non-atomically
			// here, which is sound precisely because a structural change holds the
			// exclusive lock and no other thread can be inside the container.
			Column_t *pColumn = data.template BaseBy< FIELD >();

			::BTL::Swap( pColumn[ left ], pColumn[ right ] );
		}
	}

	template < typename TDestination, typename TSource, size_t... FIELDS >
	static void AppendRow( TDestination &destination, const TSource &source, Index_t i, MIndexSequence< size_t, FIELDS... > )
	{
		destination.AddToTail( static_cast< Field_t< static_cast< TypeIndex_t >( FIELDS ) > >( FieldAt< static_cast< TypeIndex_t >( FIELDS ) >( source, i ) )... );
	}

	/// @brief Rebuilds @p destination as an independent copy of @p source, row by row.
	///
	/// @details Ball's SoA vectors duplicate content by re-appending rather than by
	/// stealing a block -- the same convention `CRBTree` and `CHashMap` follow for
	/// their copy and move conversions.
	///
	/// @complexity O(rows * columns).
	template < typename TDestination, typename TSource >
	static void CopyRows( TDestination &destination, const TSource &source )
	{
		const Index_t nRows = source.Count();

		destination.RemoveAll();

		for ( Index_t i = 0; i < nRows; ++i )
			AppendRow( destination, source, i, FieldSequence_t() );
	}

public:
	///-----------------------------------------------------------------------------
	/// @brief Row handle: a cell address, not a copy of the row.
	///
	/// @details Holds the storage and an index rather than one reference per column,
	/// so it stays the size of a pointer plus an index whatever the arity. It is only
	/// meaningful while the access object that produced it is alive; the moment that
	/// lock is released the row may be relocated or removed.
	///
	/// @complexity O(1) for every member.
	///-----------------------------------------------------------------------------
	template < typename TStorage >
	class CRow
	{
	public:
		constexpr CRow( TStorage *pData, Index_t i ) noexcept : m_pData( pData ), m_i( i ) {}

		constexpr Index_t Index() const noexcept { return m_i; }

		template < TypeIndex_t FIELD > constexpr decltype( auto ) Get() const noexcept { return FieldAt< FIELD >( *m_pData, m_i ); }
		template < TypeIndex_t FIELD > void Set( const Field_t< FIELD > &value ) const noexcept { SetFieldAt< FIELD >( *m_pData, m_i, value ); }

	private:
		TStorage *m_pData;
		Index_t m_i;
	}; // class CRow

	using Row_t = CRow< Storage_t >;
	using ConstRow_t = CRow< const Storage_t >;

	///-----------------------------------------------------------------------------
	/// @brief Forward iterator over rows, valid only inside its access object's lock.
	///
	/// @complexity O(1) per step.
	///-----------------------------------------------------------------------------
	template < typename TStorage >
	class CRowIterator
	{
	public:
		constexpr CRowIterator( TStorage *pData, Index_t i ) noexcept : m_pData( pData ), m_i( i ) {}

		constexpr CRow< TStorage > operator*() const noexcept { return CRow< TStorage >( m_pData, m_i ); }
		constexpr CRowIterator &operator++() noexcept { ++m_i; return *this; }
		constexpr bool operator==( const CRowIterator &other ) const noexcept { return m_i == other.m_i; }
		constexpr bool operator!=( const CRowIterator &other ) const noexcept { return m_i != other.m_i; }

	private:
		TStorage *m_pData;
		Index_t m_i;
	}; // class CRowIterator

	///-----------------------------------------------------------------------------
	/// @brief Shared access to the SoA for the object's lifetime.
	///
	/// @details Holds the lock from construction to destruction, so everything read
	/// through it belongs to one consistent snapshot of the container -- the row
	/// count cannot change under an iteration, and two columns cannot disagree.
	/// References and row handles obtained from it must not outlive it.
	///
	/// @complexity O(1) to construct (one shared acquire) and to destroy.
	///-----------------------------------------------------------------------------
	class CReadAccess
	{
	public:
		explicit CReadAccess( const CConcurrentBase &owner ) noexcept :
			m_Lock( owner.m_Mutex ),
			m_pData( &owner.m_Data )
		{
		}

		CReadAccess( const CReadAccess & ) = delete;
		CReadAccess &operator=( const CReadAccess & ) = delete;
		CReadAccess( CReadAccess && ) noexcept = default;

		/// @complexity O(1). Straight from the SoA -- nothing is cached anywhere.
		Index_t Size() const noexcept { return m_pData->Count(); }
		bool Empty() const noexcept { return !m_pData->Count(); }
		Index_t Capacity() const noexcept { return m_pData->Capacity(); }

		/// @complexity O(1).
		template < TypeIndex_t FIELD > decltype( auto ) Get( Index_t i ) const noexcept { return FieldAt< FIELD >( *m_pData, i ); }
		ConstRow_t Row( Index_t i ) const noexcept { return ConstRow_t( m_pData, i ); }

		/// @complexity O(1). The column base pointer, for a bulk read of one field.
		template < TypeIndex_t FIELD > auto Column() const noexcept { return m_pData->template BaseBy< FIELD >(); }

		/// @brief Read-only view of the SoA, for anything the accessors above do not cover.
		const Storage_t &Data() const noexcept { return *m_pData; }

		CRowIterator< const Storage_t > begin() const noexcept { return CRowIterator< const Storage_t >( m_pData, 0 ); }
		CRowIterator< const Storage_t > end() const noexcept { return CRowIterator< const Storage_t >( m_pData, m_pData->Count() ); }

	private:
		CSharedLock< Mutex_t > m_Lock;
		const Storage_t *m_pData;
	}; // class CReadAccess

	///-----------------------------------------------------------------------------
	/// @brief Exclusive access to the SoA for the object's lifetime.
	///
	/// @details Several related operations performed through one access object form
	/// a single transaction: no other thread observes the container between them.
	/// Every structural operation here rewrites all columns before the lock is
	/// released, so the SoA invariant never leaks in a half-applied state.
	///
	/// @complexity O(1) to construct (one exclusive acquire) and to destroy.
	///-----------------------------------------------------------------------------
	class CWriteAccess
	{
	public:
		explicit CWriteAccess( CConcurrentBase &owner ) noexcept :
			m_Lock( owner.m_Mutex ),
			m_pData( &owner.m_Data )
		{
		}

		CWriteAccess( const CWriteAccess & ) = delete;
		CWriteAccess &operator=( const CWriteAccess & ) = delete;
		CWriteAccess( CWriteAccess && ) noexcept = default;

		/// @complexity O(1).
		Index_t Size() const noexcept { return m_pData->Count(); }
		bool Empty() const noexcept { return !m_pData->Count(); }
		Index_t Capacity() const noexcept { return m_pData->Capacity(); }

		/// @complexity O(1).
		template < TypeIndex_t FIELD > decltype( auto ) Get( Index_t i ) const noexcept { return FieldAt< FIELD >( *m_pData, i ); }
		template < TypeIndex_t FIELD > void Set( Index_t i, const Field_t< FIELD > &value ) const noexcept { SetFieldAt< FIELD >( *m_pData, i, value ); }
		Row_t Row( Index_t i ) const noexcept { return Row_t( m_pData, i ); }

		/// @complexity O(1).
		template < TypeIndex_t FIELD > auto Column() const noexcept { return m_pData->template BaseBy< FIELD >(); }
		Storage_t &Data() const noexcept { return *m_pData; }

		CRowIterator< Storage_t > begin() const noexcept { return CRowIterator< Storage_t >( m_pData, 0 ); }
		CRowIterator< Storage_t > end() const noexcept { return CRowIterator< Storage_t >( m_pData, m_pData->Count() ); }

		/// @brief Writes a complete row: no reader can see only part of it.
		///
		/// @complexity O(columns).
		void SetRow( Index_t i, const Ts &...values ) const noexcept
		{
			BALL_ASSERT_MESSAGE( i < m_pData->Count(), "SetRow requires an existing row" );

			SetRowFields( i, FieldSequence_t(), values... );
		}

		/// @brief Appends one row.
		///
		/// @return Index of the appended row.
		///
		/// @complexity O(1) amortized; O(rows * columns) on the append that grows the block.
		Index_t PushBack( const Ts &...values ) const
		{
			const Index_t i = m_pData->Count();

			m_pData->AddToTail( values... );

			return i;
		}

		/// @brief Inserts one row at @p i, shifting every column identically.
		///
		/// @complexity O(rows - i) per column.
		void Insert( Index_t i, const Ts &...values ) const { m_pData->Insert( i, values... ); }

		/// @brief Removes @p nCount rows at @p i from every column.
		///
		/// @complexity O(rows - i) per column.
		void Erase( Index_t i, Index_t nCount = 1 ) const { m_pData->Remove( i, nCount ); }

		/// @brief Removes the last row.
		///
		/// @complexity O(1) amortized.
		void PopBack() const
		{
			BALL_ASSERT_MESSAGE( m_pData->Count(), "PopBack requires a non-empty container" );

			m_pData->Remove( m_pData->Count() - 1 );
		}

		/// @brief Resizes the whole SoA; grown rows are value-initialized.
		///
		/// @complexity O(|new - old|) per column, plus a reallocation when the capacity changes.
		void Resize( Index_t nCount ) const { m_pData->SetCount( nCount ); }

		/// @brief Drops every row from every column.
		///
		/// @complexity O(rows) per column.
		void Clear() const { m_pData->RemoveAll(); }

		/// @brief Exchanges two rows across every column.
		///
		/// @complexity O(columns).
		void SwapElements( Index_t left, Index_t right ) const noexcept
		{
			BALL_ASSERT_MESSAGE( left < m_pData->Count() && right < m_pData->Count(), "SwapElements requires two existing rows" );

			if ( left != right )
				SwapRowFields( *m_pData, left, right, FieldSequence_t() );
		}

		///-----------------------------------------------------------------------------
		/// @brief Sorts every column by the @p FIELD -th one, ascending.
		///
		/// @details One permutation applied to the whole SoA: the sort only ever moves
		/// rows through @ref SwapElements, so a field can never end up beside another
		/// field's value. Quicksort with a median-of-three pivot, falling back to an
		/// insertion sort on short ranges; it allocates nothing.
		///
		/// @complexity O(rows log rows) comparisons on average, each swap O(columns).
		///-----------------------------------------------------------------------------
		template < TypeIndex_t FIELD >
		void SortBy() const
		{
			const Index_t nCount = m_pData->Count();

			if ( nCount > 1 )
				QuickSort< FIELD >( 0, static_cast< Index_t >( nCount - 1 ) );
		}

	private:
		template < size_t... FIELDS >
		void SetRowFields( Index_t i, MIndexSequence< size_t, FIELDS... >, const Ts &...values ) const noexcept
		{
			( SetFieldAt< static_cast< TypeIndex_t >( FIELDS ) >( *m_pData, i, values ), ... );
		}

		template < TypeIndex_t FIELD >
		Field_t< FIELD > KeyOf( Index_t i ) const noexcept { return static_cast< Field_t< FIELD > >( FieldAt< FIELD >( *m_pData, i ) ); }

		template < TypeIndex_t FIELD >
		void InsertionSort( Index_t nFirst, Index_t nLast ) const
		{
			for ( Index_t i = nFirst + 1; i <= nLast; ++i )
			{
				for ( Index_t j = i; j > nFirst && KeyOf< FIELD >( j ) < KeyOf< FIELD >( static_cast< Index_t >( j - 1 ) ); --j )
					SwapElements( j, static_cast< Index_t >( j - 1 ) );
			}
		}

		template < TypeIndex_t FIELD >
		void QuickSort( Index_t nFirst, Index_t nLast ) const
		{
			// Short ranges are cheaper to finish with an insertion pass than to keep
			// partitioning, and the recursion depth stays bounded by the tail loop.
			constexpr Index_t INSERTION_LIMIT = 16;

			while ( nFirst < nLast )
			{
				if ( static_cast< Index_t >( nLast - nFirst ) < INSERTION_LIMIT )
				{
					InsertionSort< FIELD >( nFirst, nLast );

					return;
				}

				// Median of three, parked at the front so the partition can ignore it.
				const Index_t nMiddle = static_cast< Index_t >( nFirst + ( nLast - nFirst ) / 2 );

				if ( KeyOf< FIELD >( nMiddle ) < KeyOf< FIELD >( nFirst ) )
					SwapElements( nMiddle, nFirst );

				if ( KeyOf< FIELD >( nLast ) < KeyOf< FIELD >( nFirst ) )
					SwapElements( nLast, nFirst );

				if ( KeyOf< FIELD >( nLast ) < KeyOf< FIELD >( nMiddle ) )
					SwapElements( nLast, nMiddle );

				const Field_t< FIELD > pivot = KeyOf< FIELD >( nMiddle );

				Index_t nLeft = nFirst;
				Index_t nRight = nLast;

				for ( ;; )
				{
					while ( KeyOf< FIELD >( nLeft ) < pivot )
						++nLeft;

					while ( pivot < KeyOf< FIELD >( nRight ) )
						--nRight;

					if ( nLeft >= nRight )
						break;

					SwapElements( nLeft, nRight );
					++nLeft;

					if ( nRight > nFirst )
						--nRight;
				}

				// Recurse into the smaller half and loop on the larger one, so the
				// stack depth stays logarithmic even on an adversarial input.
				if ( static_cast< Index_t >( nRight - nFirst ) < static_cast< Index_t >( nLast - nRight ) )
				{
					QuickSort< FIELD >( nFirst, nRight );
					nFirst = static_cast< Index_t >( nRight + 1 );
				}
				else
				{
					QuickSort< FIELD >( static_cast< Index_t >( nRight + 1 ), nLast );
					nLast = nRight;
				}
			}
		}

		CUniqueLock< Mutex_t > m_Lock;
		Storage_t *m_pData;
	}; // class CWriteAccess

	/// @complexity O(1) for the heap-backed form; O(N) for the inline one, which
	/// constructs its inline rows.
	CConcurrentBase() noexcept = default;
	~CConcurrentBase() noexcept = default;

	/// @complexity O(rows * columns): the source is read under a shared lock and
	/// rebuilt row by row, the same way every other Ball SoA container duplicates.
	CConcurrentBase( const CConcurrentBase &other )
	{
		CSharedLock< Mutex_t > lock( other.m_Mutex );

		CopyRows( m_Data, other.m_Data );
	}

	/// @complexity O(rows * columns). The source is emptied; only its SoA moves,
	/// because it is the only state there is.
	CConcurrentBase( CConcurrentBase &&other ) noexcept
	{
		CUniqueLock< Mutex_t > lock( other.m_Mutex );

		CopyRows( m_Data, other.m_Data );
		other.m_Data.RemoveAll();
	}

	/// @complexity O(rows * columns). Both mutexes are taken in address order, so
	/// two threads assigning the same pair in opposite directions cannot deadlock.
	CConcurrentBase &operator=( const CConcurrentBase &other )
	{
		if ( this != &other )
		{
			CScopedLock< Mutex_t > lock( m_Mutex, other.m_Mutex );

			CopyRows( m_Data, other.m_Data );
		}

		return *this;
	}

	CConcurrentBase &operator=( CConcurrentBase &&other ) noexcept
	{
		if ( this != &other )
		{
			CScopedLock< Mutex_t > lock( m_Mutex, other.m_Mutex );

			CopyRows( m_Data, other.m_Data );
			other.m_Data.RemoveAll();
		}

		return *this;
	}

	/// @brief Opens a read transaction over the whole SoA.
	///
	/// @complexity O(1).
	CReadAccess ReadAccess() const noexcept { return CReadAccess( *this ); }

	/// @brief Opens a write transaction over the whole SoA.
	///
	/// @complexity O(1).
	CWriteAccess WriteAccess() noexcept { return CWriteAccess( *this ); }

	/// @complexity O(1) plus one lock round trip. Read straight from the SoA: the
	/// count lives there and nowhere else, so no cheaper lock-free path exists by
	/// design.
	Index_t Size() const noexcept { return ReadAccess().Size(); }
	bool Empty() const noexcept { return ReadAccess().Empty(); }
	Index_t Capacity() const noexcept { return ReadAccess().Capacity(); }

	/// @complexity O(1) amortized plus one lock round trip.
	Index_t PushBack( const Ts &...values ) { return WriteAccess().PushBack( values... ); }

	/// @complexity O(rows - i) per column plus one lock round trip.
	void Insert( Index_t i, const Ts &...values ) { WriteAccess().Insert( i, values... ); }
	void Erase( Index_t i, Index_t nCount = 1 ) { WriteAccess().Erase( i, nCount ); }

	/// @complexity O(|new - old|) per column plus one lock round trip.
	void Resize( Index_t nCount ) { WriteAccess().Resize( nCount ); }
	void Clear() { WriteAccess().Clear(); }

	/// @complexity O(columns) plus one lock round trip.
	void SwapElements( Index_t left, Index_t right ) { WriteAccess().SwapElements( left, right ); }
	void SetRow( Index_t i, const Ts &...values ) { WriteAccess().SetRow( i, values... ); }

	/// @complexity O(rows log rows) plus one lock round trip.
	template < TypeIndex_t FIELD > void SortBy() { WriteAccess().template SortBy< FIELD >(); }

	/// @complexity O(1) plus one lock round trip. Returns a copy: a reference would
	/// outlive the lock that makes it meaningful.
	template < TypeIndex_t FIELD >
	Field_t< FIELD > Get( Index_t i ) const noexcept
	{
		const CReadAccess access( *this );

		BALL_ASSERT_MESSAGE( i < access.Size(), "Get requires an existing row" );

		return static_cast< Field_t< FIELD > >( access.template Get< FIELD >( i ) );
	}

	/// @complexity O(1) plus one lock round trip.
	template < TypeIndex_t FIELD >
	void Set( Index_t i, const Field_t< FIELD > &value ) noexcept
	{
		const CWriteAccess access( *this );

		BALL_ASSERT_MESSAGE( i < access.Size(), "Set requires an existing row" );

		access.template Set< FIELD >( i, value );
	}

	///-----------------------------------------------------------------------------
	/// @brief Removes the last row and hands its columns back, if there is one.
	///
	/// @param out One output per column, written only when a row was removed.
	///
	/// @return false when the container was empty.
	///
	/// @details The emptiness test and the removal happen inside one exclusive
	/// transaction, so two threads cannot both see the same last row.
	///
	/// @complexity O(columns) plus one lock round trip.
	///-----------------------------------------------------------------------------
	template < typename... Us >
	bool TryPopBack( Us &...out )
	{
		BALL_STATIC_ASSERT( sizeof...( Us ) == sizeof...( Ts ), "TryPopBack takes one output per column" );

		const CWriteAccess access( *this );
		const Index_t nCount = access.Size();

		if ( !nCount )
			return false;

		const Index_t iLast = static_cast< Index_t >( nCount - 1 );

		ReadRowInto( access, iLast, FieldSequence_t(), out... );
		access.PopBack();

		return true;
	}

	///-----------------------------------------------------------------------------
	/// @brief Copies the SoA into @p out, which ends up holding an independent copy.
	///
	/// @details Taken under a read lock and released before returning, so the copy
	/// is a consistent moment of the container and never a live alias of it. The
	/// container itself stores nothing about the snapshot.
	///
	/// @complexity O(rows * columns) plus one lock round trip.
	///-----------------------------------------------------------------------------
	template < typename TStorage >
	void SnapshotInto( TStorage &out ) const
	{
		CSharedLock< Mutex_t > lock( m_Mutex );

		CopyRows( out, m_Data );
	}

	/// @brief Independent copy of the container, taken under a read lock.
	///
	/// @complexity O(rows * columns) plus one lock round trip.
	CConcurrentBase Snapshot() const { return CConcurrentBase( *this ); }

	///-----------------------------------------------------------------------------
	/// @brief Exchanges the SoA contents of two containers.
	///
	/// @details Both are held exclusively, in address order. The storage is rebuilt
	/// through one temporary rather than swapped as a block, because a Ball SoA
	/// vector duplicates by re-appending; the mutexes stay where they are, since a
	/// waiter already holds their addresses.
	///
	/// @complexity O((rows + other rows) * columns).
	///-----------------------------------------------------------------------------
	void Swap( CConcurrentBase &other )
	{
		if ( this == &other )
			return;

		CScopedLock< Mutex_t > lock( m_Mutex, other.m_Mutex );

		Storage_t temporary;

		CopyRows( temporary, m_Data );
		CopyRows( m_Data, other.m_Data );
		CopyRows( other.m_Data, temporary );
	}

	///-----------------------------------------------------------------------------
	/// @brief Atomic operations on one `CAtomic< U >` column.
	///
	/// @details Each holds shared access for its duration. The shared lock is what
	/// stops a structural modification from relocating the row mid-operation; the
	/// atomic instruction itself is what orders the element against other threads.
	/// Several of these do **not** compose into a multi-field atomic update -- use
	/// @ref SetRow, or a @ref WriteAccess transaction, when fields share an invariant.
	///
	/// @complexity O(1) plus one shared lock round trip.
	///-----------------------------------------------------------------------------
	template < TypeIndex_t FIELD >
	FieldValue_t< FIELD > Load( Index_t i, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) const noexcept requires ( FIELD_IS_ATOMIC< FIELD > )
	{
		CSharedLock< Mutex_t > lock( m_Mutex );

		BALL_ASSERT_MESSAGE( i < m_Data.Count(), "Load requires an existing row" );

		return FieldAt< FIELD >( m_Data, i ).Load( eOrder );
	}

	template < TypeIndex_t FIELD >
	void Store( Index_t i, FieldValue_t< FIELD > value, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept requires ( FIELD_IS_ATOMIC< FIELD > )
	{
		CSharedLock< Mutex_t > lock( m_Mutex );

		BALL_ASSERT_MESSAGE( i < m_Data.Count(), "Store requires an existing row" );

		FieldAt< FIELD >( m_Data, i ).Store( value, eOrder );
	}

	template < TypeIndex_t FIELD >
	FieldValue_t< FIELD > Exchange( Index_t i, FieldValue_t< FIELD > value, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept requires ( FIELD_IS_ATOMIC< FIELD > )
	{
		CSharedLock< Mutex_t > lock( m_Mutex );

		BALL_ASSERT_MESSAGE( i < m_Data.Count(), "Exchange requires an existing row" );

		return FieldAt< FIELD >( m_Data, i ).Exchange( value, eOrder );
	}

	template < TypeIndex_t FIELD >
	FieldValue_t< FIELD > FetchAdd( Index_t i, FieldValue_t< FIELD > value, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept requires ( FIELD_IS_ATOMIC< FIELD > )
	{
		CSharedLock< Mutex_t > lock( m_Mutex );

		BALL_ASSERT_MESSAGE( i < m_Data.Count(), "FetchAdd requires an existing row" );

		return FieldAt< FIELD >( m_Data, i ).FetchAdd( value, eOrder );
	}

	template < TypeIndex_t FIELD >
	FieldValue_t< FIELD > FetchSub( Index_t i, FieldValue_t< FIELD > value, EMemoryOrder eOrder = EMemoryOrder::SEQ_CST ) noexcept requires ( FIELD_IS_ATOMIC< FIELD > )
	{
		CSharedLock< Mutex_t > lock( m_Mutex );

		BALL_ASSERT_MESSAGE( i < m_Data.Count(), "FetchSub requires an existing row" );

		return FieldAt< FIELD >( m_Data, i ).FetchSub( value, eOrder );
	}

	template < TypeIndex_t FIELD >
	bool CompareExchange( Index_t i, FieldValue_t< FIELD > &expected, FieldValue_t< FIELD > desired, EMemoryOrder eSuccess = EMemoryOrder::SEQ_CST, EMemoryOrder eFailure = EMemoryOrder::SEQ_CST ) noexcept requires ( FIELD_IS_ATOMIC< FIELD > )
	{
		CSharedLock< Mutex_t > lock( m_Mutex );

		BALL_ASSERT_MESSAGE( i < m_Data.Count(), "CompareExchange requires an existing row" );

		return FieldAt< FIELD >( m_Data, i ).CompareExchange( expected, desired, eSuccess, eFailure );
	}

private:
	template < typename TAccess, size_t... FIELDS, typename... Us >
	static void ReadRowInto( const TAccess &access, Index_t i, MIndexSequence< size_t, FIELDS... >, Us &...out )
	{
		( ( out = static_cast< Us >( access.template Get< static_cast< TypeIndex_t >( FIELDS ) >( i ) ) ), ... );
	}

	// The one authoritative state, and the one domain that guards it. Nothing else
	// is stored: no size, no capacity, no emptiness, no generation, no validity.
	Storage_t m_Data;
	mutable Mutex_t m_Mutex;
}; // class CConcurrentBase

///-----------------------------------------------------------------------------
/// @brief Heap-backed concurrent SoA with concurrent readers.
///-----------------------------------------------------------------------------
template < typename I, typename... Ts >
class CConcurrent : public CConcurrentBase< CSharedMutexSynchronization, I, 0, Ts... >
{
public:
	using Base_t = CConcurrentBase< CSharedMutexSynchronization, I, 0, Ts... >;
	using Base_t::Base_t;
};

/// @brief Inline-buffer counterpart of `CConcurrent`.
template < typename I, I N, typename... Ts >
class CBufferConcurrent : public CConcurrentBase< CSharedMutexSynchronization, I, N, Ts... >
{
public:
	using Base_t = CConcurrentBase< CSharedMutexSynchronization, I, N, Ts... >;
	using Base_t::Base_t;
};

///-----------------------------------------------------------------------------
/// @brief Concurrent SoA whose element operations carry their own synchronization.
///
/// @details No lock is taken. Sound only while every field is a `CAtomic< U >`
/// and no thread changes the container's shape during shared use; see
/// @ref CAtomicSynchronization.
///
/// @note This is the definition of the primary `CAtomic` template declared in
/// `types/atomic.hpp`, so one name covers both atomic shapes: a single argument
/// selects the atomic-value specialization, an index type plus one or more
/// columns selects this container.
///-----------------------------------------------------------------------------
template < typename I, typename... Ts >
class CAtomic : public CConcurrentBase< CAtomicSynchronization, I, 0, Ts... >
{
public:
	using Base_t = CConcurrentBase< CAtomicSynchronization, I, 0, Ts... >;
	using Base_t::Base_t;
};

/// Convenience spellings pinning the index type.
template < typename... Ts > using Concurrent16_t = CConcurrent< size16_t, Ts... >;
template < typename... Ts > using Concurrent32_t = CConcurrent< size32_t, Ts... >;
template < typename... Ts > using Concurrent64_t = CConcurrent< size64_t, Ts... >;

template < size32_t N, typename... Ts > using BufferConcurrent32_t = CBufferConcurrent< size32_t, N, Ts... >;
template < size64_t N, typename... Ts > using BufferConcurrent64_t = CBufferConcurrent< size64_t, N, Ts... >;

template < typename... Ts > using AtomicConcurrent32_t = CAtomic< size32_t, Ts... >;
template < typename... Ts > using AtomicConcurrent64_t = CAtomic< size64_t, Ts... >;

#endif // !defined( _INCLUDE_BALL_TYPES_CONCURRENT_HPP_ )

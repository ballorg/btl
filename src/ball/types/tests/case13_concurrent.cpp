module;

#include <ball/time/macros.h>

#include <thread>
#include <vector>

module Ball.Types;

import Ball.New;
import Ball.Time;
import :Meta;
import :Atomic;
import :Concurrent;
import :Lock;
import :String;
import :StringView;
import :Vector;
import :Tests.Case13;

using TestsOutput_t = BTL::BufferString_t< 4096 >;

namespace
{
	using Counter_t = BTL::CAtomic< BTL::uint64_t >;

	// Three columns of distinct types: a key, a value derived from it, and an
	// atomic counter. Every check below asserts the three stay on the same row.
	using Soa_t = BTL::Concurrent32_t< BTL::uint64_t, double, Counter_t >;
	using ExclusiveSoa_t = BTL::CConcurrentBase< BTL::CMutexSynchronization, BTL::size32_t, 0, BTL::uint64_t, double, Counter_t >;
	using BufferSoa_t = BTL::CBufferConcurrent< BTL::size32_t, 16, BTL::uint64_t, double, Counter_t >;
	using Index_t = Soa_t::Index_t;

	static void LogCheck( TestsOutput_t &sOut, BTL::StringView_t svLabel, bool bOk )
	{
		sOut.AppendMultiple( "BTL::Concurrent_t: ", svLabel, ": " );

		if ( bOk )
			sOut += "ok\n";
		else
			sOut += "mismatch\n";
	}

	static double ValueOf( BTL::uint64_t nKey ) { return static_cast< double >( nKey ) * 0.5; }

	template < typename TSoa >
	static void Append( TSoa &soa, BTL::uint64_t nKey )
	{
		soa.PushBack( nKey, ValueOf( nKey ), Counter_t( nKey ) );
	}

	// The SoA invariant in one predicate: every column of every row agrees with the
	// key column, and the row count each column reports is the shared one.
	template < typename TSoa >
	static bool IsConsistent( const TSoa &soa )
	{
		auto access = soa.ReadAccess();

		Index_t nSeen = 0;

		for ( auto row : access )
		{
			const BTL::uint64_t nKey = row.template Get< 0 >();

			if ( row.template Get< 1 >() != ValueOf( nKey ) || row.template Get< 2 >().Load() != nKey )
				return false;

			++nSeen;
		}

		return nSeen == access.Size();
	}
}

void Case13_Concurrent( TestsOutput_t &sOut )
{
	bool bAllOk = true;

	sOut += "---\n";

	// --- The primitives the container is built on ------------------------------
	{
		BTL::CAtomic< BTL::uint32_t > counter{ 5u };

		bool bOk = counter.Load() == 5u;

		bOk = bOk && counter.Exchange( 9u ) == 5u && counter.Load() == 9u;

		BTL::uint32_t nExpected = 9u;

		bOk = bOk && counter.CompareExchange( nExpected, 11u ) && counter.Load() == 11u;
		bOk = bOk && !counter.CompareExchange( nExpected, 13u ) && nExpected == 11u;
		bOk = bOk && counter.FetchAdd( 4u ) == 11u && counter.Load() == 15u;
		bOk = bOk && counter.FetchSub( 5u ) == 15u && counter.Load() == 10u;
		bOk = bOk && counter.FetchAnd( 0xCu ) == 10u && counter.Load() == 8u;
		bOk = bOk && counter.FetchOr( 1u ) == 8u && counter.Load() == 9u;
		bOk = bOk && counter.FetchXor( 8u ) == 9u && counter.Load() == 1u;

		// A float and a pointer travel through the same bit-cast backend.
		BTL::CAtomic< double > position{ 1.5 };
		BTL::CAtomic< const char * > pointer{ nullptr };
		const char szMarker[] = "marker";

		position.Store( 2.5 );
		pointer.Store( szMarker );
		bOk = bOk && position.Load() == 2.5 && pointer.Load() == szMarker;

		// A relocatable atomic is what lets an SoA column hold one.
		bOk = bOk && BTL::IS_TRIVIALLY_COPYABLE< BTL::CAtomic< BTL::uint64_t > > && sizeof( Counter_t ) == sizeof( BTL::uint64_t );

		BTL::CMutex mutex;

		bOk = bOk && mutex.TryLock();
		mutex.Unlock();

		BTL::CSharedMutex shared;

		{
			BTL::CSharedLock< BTL::CSharedMutex > readerOne( shared );
			BTL::CSharedLock< BTL::CSharedMutex > readerTwo( shared );

			// Readers coexist; a writer is refused while either is inside.
			bOk = bOk && !shared.TryLock();
		}

		bOk = bOk && shared.TryLock();
		shared.Unlock();

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "atomics and locks behave single-threaded", bOk );
	}

	// --- One `CAtomic` name, two shapes ---------------------------------------
	{
		// A single argument spells the atomic value; an index type followed by
		// columns spells the unsynchronized container. The traits must keep
		// telling the two apart, since only a column is an atomic field.
		using AtomicSoa_t = BTL::AtomicConcurrent32_t< BTL::uint64_t, Counter_t >;

		bool bOk = BTL::IS_ATOMIC< Counter_t > && !BTL::IS_ATOMIC< AtomicSoa_t >;

		bOk = bOk && BTL::IS_SAME< BTL::AtomicValue_t< Counter_t >, BTL::uint64_t >;
		bOk = bOk && BTL::IS_SAME< BTL::AtomicValue_t< AtomicSoa_t >, AtomicSoa_t >;
		bOk = bOk && AtomicSoa_t::SHARED_READS && AtomicSoa_t::FIELD_IS_ATOMIC< 1 > && !AtomicSoa_t::FIELD_IS_ATOMIC< 0 >;

		AtomicSoa_t soa;

		soa.PushBack( 3u, Counter_t( 0u ) );
		soa.FetchAdd< 1 >( 0, 7u );

		bOk = bOk && soa.Size() == 1u && soa.Get< 0 >( 0 ) == 3u && soa.Load< 1 >( 0 ) == 7u;

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "one CAtomic name covers a value and a lock-free SoA", bOk );
	}

	// --- Single-threaded container semantics -----------------------------------
	{
		Soa_t soa;

		bool bOk = soa.Empty() && !soa.Size();

		for ( BTL::uint64_t nKey = 0; nKey < 10u; ++nKey )
			Append( soa, nKey );

		bOk = bOk && soa.Size() == 10u && !soa.Empty() && IsConsistent( soa );
		bOk = bOk && soa.Get< 0 >( 3 ) == 3u && soa.Get< 1 >( 3 ) == ValueOf( 3u );

		// A whole-row write, so no reader can catch one column updated and not another.
		soa.SetRow( 3, 99u, ValueOf( 99u ), Counter_t( 99u ) );
		bOk = bOk && soa.Get< 0 >( 3 ) == 99u && IsConsistent( soa );

		// Reordering moves every column together.
		soa.SwapElements( 0, 9 );
		bOk = bOk && soa.Get< 0 >( 0 ) == 9u && soa.Get< 0 >( 9 ) == 0u && IsConsistent( soa );

		soa.Erase( 3 );
		bOk = bOk && soa.Size() == 9u && IsConsistent( soa );

		soa.Insert( 0, 42u, ValueOf( 42u ), Counter_t( 42u ) );
		bOk = bOk && soa.Size() == 10u && soa.Get< 0 >( 0 ) == 42u && IsConsistent( soa );

		// Resize grows and shrinks the complete SoA at once.
		soa.Resize( 20 );
		bOk = bOk && soa.Size() == 20u;
		soa.Resize( 5 );
		bOk = bOk && soa.Size() == 5u;

		soa.Clear();
		bOk = bOk && soa.Empty() && !soa.Size();

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "single-threaded operations keep the SoA invariant", bOk );
	}

	// --- Sorting applies one permutation to every column -----------------------
	{
		Soa_t soa;

		for ( BTL::uint64_t i = 0; i < 500u; ++i )
			Append( soa, static_cast< BTL::uint64_t >( ( i * 7919u ) % 1000u ) );

		soa.SortBy< 0 >();

		bool bOk = IsConsistent( soa ) && soa.Size() == 500u;

		{
			auto access = soa.ReadAccess();

			BTL::uint64_t nPrevious = 0;
			bool bFirst = true;

			for ( auto row : access )
			{
				const BTL::uint64_t nKey = row.Get< 0 >();

				bOk = bOk && ( bFirst || nPrevious <= nKey );
				nPrevious = nKey;
				bFirst = false;
			}
		}

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "sorting permutes every column identically", bOk );
	}

	// --- Copy, move, assignment, swap and snapshot ------------------------------
	{
		Soa_t source;

		for ( BTL::uint64_t nKey = 0; nKey < 50u; ++nKey )
			Append( source, nKey );

		Soa_t copied( source );
		bool bOk = copied.Size() == 50u && IsConsistent( copied ) && source.Size() == 50u;

		Soa_t moved( BTL::Move( copied ) );
		bOk = bOk && moved.Size() == 50u && IsConsistent( moved ) && copied.Empty();

		Soa_t assigned;
		assigned = source;
		bOk = bOk && assigned.Size() == 50u && IsConsistent( assigned );

		Soa_t moveAssigned;
		moveAssigned = BTL::Move( moved );
		bOk = bOk && moveAssigned.Size() == 50u && IsConsistent( moveAssigned ) && moved.Empty();

		// Self-assignment must not empty the container.
		Soa_t &alias = assigned;
		assigned = alias;
		bOk = bOk && assigned.Size() == 50u;

		Soa_t other;
		Append( other, 777u );

		assigned.Swap( other );
		bOk = bOk && assigned.Size() == 1u && other.Size() == 50u && IsConsistent( assigned ) && IsConsistent( other );

		// The snapshot is an independent copy: mutating the source must not touch it.
		BTL::CVector< BTL::size32_t, BTL::uint64_t, double, Counter_t > snapshot;

		source.SnapshotInto( snapshot );
		bOk = bOk && snapshot.Count() == 50u;

		source.Clear();
		bOk = bOk && snapshot.Count() == 50u && source.Empty();

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "copy, move, swap and snapshot duplicate only the SoA", bOk );
	}

	// --- The other capacity and synchronization flavors -------------------------
	{
		BufferSoa_t buffered;
		ExclusiveSoa_t exclusive;

		bool bOk = true;

		for ( BTL::uint64_t nKey = 0; nKey < 40u; ++nKey )
		{
			Append( buffered, nKey );
			Append( exclusive, nKey );
		}

		// The inline flavor crosses out of its 16 inline rows and stays correct.
		bOk = bOk && buffered.Size() == 40u && IsConsistent( buffered );
		bOk = bOk && exclusive.Size() == 40u && IsConsistent( exclusive );
		bOk = bOk && !ExclusiveSoa_t::SHARED_READS && Soa_t::SHARED_READS;

		buffered.SortBy< 0 >();
		exclusive.SortBy< 0 >();
		bOk = bOk && IsConsistent( buffered ) && IsConsistent( exclusive );

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "exclusive-mutex and inline-buffer flavors agree", bOk );
	}

	// --- Parallel appends from many threads -------------------------------------
	{
		Soa_t soa;

		constexpr int nThreads = 8;
		constexpr BTL::uint64_t nPerThread = 400u;

		std::vector< std::thread > threads;

		for ( int nThread = 0; nThread < nThreads; ++nThread )
		{
			threads.emplace_back( [ &soa, nThread ]()
			{
				for ( BTL::uint64_t i = 0; i < nPerThread; ++i )
					Append( soa, static_cast< BTL::uint64_t >( nThread ) * 1000000u + i );
			} );
		}

		for ( auto &thread : threads )
			thread.join();

		const bool bOk = soa.Size() == nThreads * nPerThread && IsConsistent( soa );

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "parallel appends lose no row and split no row", bOk );
	}

	// --- Readers running against writers ----------------------------------------
	{
		Soa_t soa;

		for ( BTL::uint64_t nKey = 0; nKey < 200u; ++nKey )
			Append( soa, nKey );

		BTL::CAtomic< BTL::uint32_t > nStop{ 0u };
		BTL::CAtomic< BTL::uint32_t > nInconsistent{ 0u };

		std::vector< std::thread > readers;
		std::vector< std::thread > writers;

		for ( int nThread = 0; nThread < 6; ++nThread )
		{
			readers.emplace_back( [ &soa, &nStop, &nInconsistent ]()
			{
				while ( !nStop.Load( BTL::EMemoryOrder::RELAXED ) )
				{
					// One read transaction: the row count cannot move under the walk,
					// and no row can be caught half written.
					auto access = soa.ReadAccess();

					for ( auto row : access )
					{
						if ( row.Get< 1 >() != ValueOf( row.Get< 0 >() ) || row.Get< 2 >().Load() != row.Get< 0 >() )
							nInconsistent.FetchAdd( 1u );
					}
				}
			} );
		}

		for ( int nThread = 0; nThread < 2; ++nThread )
		{
			writers.emplace_back( [ &soa ]()
			{
				for ( BTL::uint64_t i = 0; i < 300u; ++i )
				{
					Append( soa, 1000u + i );

					// A write transaction: the size test and the erase it decides on
					// happen without another thread stepping between them.
					auto access = soa.WriteAccess();

					if ( access.Size() > 300u )
						access.Erase( 0 );
				}
			} );
		}

		for ( auto &writer : writers )
			writer.join();

		nStop.Store( 1u, BTL::EMemoryOrder::RELAXED );

		for ( auto &reader : readers )
			reader.join();

		const bool bOk = !nInconsistent.Load() && IsConsistent( soa );

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "readers never observe a partially applied write", bOk );
	}

	// --- Atomic field updates mixed with locked structural access ----------------
	{
		Soa_t soa;

		constexpr Index_t nRows = 16;
		constexpr int nThreads = 8;
		constexpr BTL::uint64_t nPerThread = 2000u;

		for ( BTL::uint64_t nKey = 0; nKey < nRows; ++nKey )
			soa.PushBack( nKey, ValueOf( nKey ), Counter_t( 0u ) );

		std::vector< std::thread > threads;

		for ( int nThread = 0; nThread < nThreads; ++nThread )
		{
			threads.emplace_back( [ &soa ]()
			{
				for ( BTL::uint64_t i = 0; i < nPerThread; ++i )
					soa.FetchAdd< 2 >( static_cast< Index_t >( i % nRows ), 1u );
			} );
		}

		// A reader taking shared access alongside the atomic writers.
		threads.emplace_back( [ &soa ]()
		{
			for ( int i = 0; i < 200; ++i )
			{
				auto access = soa.ReadAccess();

				for ( auto row : access )
					( void )row.Get< 2 >().Load( BTL::EMemoryOrder::RELAXED );
			}
		} );

		for ( auto &thread : threads )
			thread.join();

		BTL::uint64_t nTotal = 0;

		{
			auto access = soa.ReadAccess();

			for ( auto row : access )
				nTotal += row.Get< 2 >().Load();
		}

		const bool bOk = nTotal == nThreads * nPerThread && soa.Size() == nRows;

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "atomic field updates lose no increment", bOk );
	}

	// --- Racing TryPopBack: every row is handed out exactly once ------------------
	{
		Soa_t soa;

		constexpr BTL::uint64_t nRows = 2000u;

		for ( BTL::uint64_t nKey = 0; nKey < nRows; ++nKey )
			Append( soa, nKey );

		BTL::CAtomic< BTL::uint64_t > nPopped{ 0u };
		BTL::CAtomic< BTL::uint32_t > nTorn{ 0u };

		std::vector< std::thread > threads;

		for ( int nThread = 0; nThread < 8; ++nThread )
		{
			threads.emplace_back( [ &soa, &nPopped, &nTorn ]()
			{
				BTL::uint64_t nKey = 0;
				double flValue = 0.0;
				Counter_t counter;

				while ( soa.TryPopBack( nKey, flValue, counter ) )
				{
					if ( flValue != ValueOf( nKey ) || counter.Load() != nKey )
						nTorn.FetchAdd( 1u );

					nPopped.FetchAdd( 1u );
				}
			} );
		}

		for ( auto &thread : threads )
			thread.join();

		const bool bOk = nPopped.Load() == nRows && !nTorn.Load() && soa.Empty();

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "racing TryPopBack hands out each row once, whole", bOk );
	}

	// --- Structural churn from several directions at once ------------------------
	{
		Soa_t left;
		Soa_t right;

		for ( BTL::uint64_t i = 0; i < 400u; ++i )
		{
			Append( left, static_cast< BTL::uint64_t >( ( i * 7919u ) % 400u ) );
			Append( right, 1000u + i );
		}

		BTL::CAtomic< BTL::uint32_t > nInconsistent{ 0u };

		std::vector< std::thread > threads;

		threads.emplace_back( [ &left ]() { for ( int i = 0; i < 15; ++i ) left.SortBy< 0 >(); } );
		threads.emplace_back( [ &left, &right ]() { for ( int i = 0; i < 15; ++i ) left.Swap( right ); } );
		threads.emplace_back( [ &right, &left ]() { for ( int i = 0; i < 15; ++i ) right.Swap( left ); } );
		threads.emplace_back( [ &left ]() { for ( int i = 0; i < 15; ++i ) { Soa_t copy( left ); ( void )copy.Size(); } } );
		threads.emplace_back( [ &left, &nInconsistent ]()
		{
			for ( int i = 0; i < 60; ++i )
			{
				auto access = left.ReadAccess();

				for ( auto row : access )
				{
					if ( row.Get< 1 >() != ValueOf( row.Get< 0 >() ) )
						nInconsistent.FetchAdd( 1u );
				}
			}
		} );

		for ( auto &thread : threads )
			thread.join();

		const bool bOk = !nInconsistent.Load() && IsConsistent( left ) && IsConsistent( right ) && left.Size() + right.Size() == 800u;

		bAllOk = bAllOk && bOk;
		LogCheck( sOut, "sort, swap and copy contend without tearing a row", bOk );
	}

	sOut.AppendMultiple( "BTL::Concurrent_t: " );

	if ( bAllOk )
		sOut += "ok\n";
	else
		sOut += "mismatch\n";

	sOut += "---\n";
}

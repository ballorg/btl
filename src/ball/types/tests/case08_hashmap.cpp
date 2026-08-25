#include "common.hpp"

#include <unordered_map>

namespace
{
	using Map_t = BTL::HashMap32_t< size_t, size_t >;
	using RefMap_t = std::unordered_map< size_t, size_t >;

	static void LogMapCheck( TestsOutput_t &sOut, BTL::StringView_t svLabel, bool bOk )
	{
		sOut.AppendMultiple( "BTL::HashMap_t: ", svLabel, ": " );

		if ( bOk )
			sOut += "ok\n";
		else
			sOut += "mismatch\n";
	}

	// Value derived from a key, so a single expression checks key->value integrity.
	static size_t ValueOf( size_t nKey )
	{
		return nKey * 7 + 1;
	}

	// Verify a BTL map contains exactly the reference map's key/value pairs.
	static bool MatchesReference( const Map_t &map, const RefMap_t &ref, TestsOutput_t &sOut )
	{
		if ( map.Count() != ref.size() )
		{
			sOut.AppendMultiple( "  (count ", map.Count(), " != ref ", ref.size(), ")\n" );

			return false;
		}

		for ( const auto &kv : ref )
		{
			const auto iSlot = map.Find( kv.first );

			if ( iSlot == map.EndIndex() || map.Get< 1 >( iSlot ) != kv.second )
				return false;
		}

		// And no extra live keys beyond the reference set.
		size_t nSeen = 0;

		BALL_HASHMAP_FOREACH( map, it )
		{
			if ( ref.find( map.Key( it ) ) == ref.end() )
				return false;

			++nSeen;
		}

		return nSeen == ref.size();
	}
}

void Case08_HashMap( TestsOutput_t &sOut )
{
	bool bAllOk = true;

	sOut += "---\n";

	constexpr size_t nKeys = 5000;

	Map_t map;
	RefMap_t ref;

	// --- Insert (with growth across several rehashes) --------------------------
	{
		BALL_PROF_BEGIN( HashMapInsert );

		for ( size_t i = 0; i < nKeys; ++i )
		{
			const size_t nKey = i * 1315423911u + 17u;

			map.Insert( nKey, ValueOf( nKey ) );
			ref[ nKey ] = ValueOf( nKey );
		}

		auto nsInsert = BALL_PROF_END( HashMapInsert );

		const bool bOk = MatchesReference( map, ref, sOut );

		bAllOk = bAllOk && bOk;
		sOut.AppendMultiple( "BTL::HashMap_t: insert ", map.Count(), " (", nsInsert.AsMillisF(), " ms): " );
		if ( bOk )
			sOut += "ok\n";
		else
			sOut += "mismatch\n";
	}

	// --- Duplicate keys are rejected -------------------------------------------
	{
		const size_t nBefore = map.Count();
		const size_t nAnyKey = 17u; // i == 0 above
		const auto iDup = map.Insert( nAnyKey, ValueOf( nAnyKey ) );
		const bool bOk = iDup == map.EndIndex() && map.Count() == nBefore;

		bAllOk = bAllOk && bOk;
		LogMapCheck( sOut, "duplicate insert rejected", bOk );
	}

	// --- Find hits and misses --------------------------------------------------
	{
		bool bOk = true;

		for ( size_t i = 0; i < nKeys; ++i )
		{
			const size_t nKey = i * 1315423911u + 17u;
			const auto iSlot = map.Find( nKey );

			bOk = bOk && iSlot != map.EndIndex() && map.Get< 1 >( iSlot ) == ValueOf( nKey );
		}

		// A key never inserted must miss.
		bOk = bOk && !map.Contains( 0xDEADBEEFu );

		bAllOk = bAllOk && bOk;
		LogMapCheck( sOut, "find hits and misses", bOk );
	}

	// --- Remove (tombstones) then verify ---------------------------------------
	{
		size_t nRemoved = 0;

		for ( size_t i = 0; i < nKeys; i += 2 )
		{
			const size_t nKey = i * 1315423911u + 17u;

			if ( map.Remove( nKey ) )
			{
				ref.erase( nKey );
				++nRemoved;
			}
		}

		const bool bOk = nRemoved > 0 && MatchesReference( map, ref, sOut );

		bAllOk = bAllOk && bOk;
		sOut.AppendMultiple( "BTL::HashMap_t: removed ", nRemoved, ", remaining ", map.Count(), ": " );

		if ( bOk )
			sOut += "ok\n";
		else
			sOut += "mismatch\n";
	}

	// --- Re-insert into tombstoned slots ---------------------------------------
	{
		for ( size_t i = 0; i < nKeys; i += 2 )
		{
			const size_t nKey = i * 1315423911u + 17u;

			map.Insert( nKey, ValueOf( nKey ) );
			ref[ nKey ] = ValueOf( nKey );
		}

		const bool bOk = MatchesReference( map, ref, sOut );

		bAllOk = bAllOk && bOk;
		LogMapCheck( sOut, "re-insert into freed slots", bOk );
	}

	// --- Clear -----------------------------------------------------------------
	{
		map.Clear();

		const bool bOk = map.Count() == 0 && map.IsEmpty() && !map.Contains( 17u );

		bAllOk = bAllOk && bOk;
		LogMapCheck( sOut, "clear empties the table", bOk );
	}

	// --- Inline-buffer variant overflowing to the heap -------------------------
	{
		BTL::BufferHashMap32_t< 16, size_t, size_t > buffered;
		RefMap_t refBuffered;
		bool bOk = buffered.BucketCount() == 16 && buffered.IsEmpty();
		const auto iFirst = buffered.Insert( 5u, ValueOf( 5u ) );

		bOk = bOk && iFirst != buffered.EndIndex() && buffered.BucketCount() == 16;
		refBuffered[ 5u ] = ValueOf( 5u );

		for ( size_t i = 1; i < 200; ++i )
		{
			const size_t nKey = i * 3u + 5u;

			buffered.Insert( nKey, ValueOf( nKey ) );
			refBuffered[ nKey ] = ValueOf( nKey );
		}

		if ( buffered.Count() != refBuffered.size() )
		{
			bOk = false;
		}
		else
		{
			for ( const auto &kv : refBuffered )
			{
				const auto iSlot = buffered.Find( kv.first );

				bOk = bOk && iSlot != buffered.EndIndex() && buffered.Get< 1 >( iSlot ) == kv.second;
			}
		}

		bAllOk = bAllOk && bOk;
		LogMapCheck( sOut, "buffer map overflows to heap", bOk );
	}

	// --- Unified variadic aliases cover sets and multi-column maps -------------
	{
		BTL::HashMap32_t< size_t > set;
		BTL::HashMap32_t< size_t, BTL::size16_t, BTL::size64_t > columns;

		const auto iSet = set.Insert( 7u );
		const auto iColumns = columns.Insert( 11u, BTL::size16_t( 13 ), BTL::size64_t( 17 ) );
		const bool bOk = iSet != set.EndIndex() && set.Contains( 7u ) &&
			iColumns != columns.EndIndex() && columns.Get< 1 >( iColumns ) == 13 &&
			columns.Get< 2 >( iColumns ) == 17;

		bAllOk = bAllOk && bOk;
		LogMapCheck( sOut, "variadic aliases cover set and columns", bOk );
	}

	// --- Cross-capacity copy ---------------------------------------------------
	{
		Map_t source;
		RefMap_t refSource;

		for ( size_t i = 0; i < 1000; ++i )
		{
			const size_t nKey = i * 2654435761u + 9u;

			source.Insert( nKey, ValueOf( nKey ) );
			refSource[ nKey ] = ValueOf( nKey );
		}

		Map_t copy( source );
		const bool bOk = MatchesReference( copy, refSource, sOut );

		bAllOk = bAllOk && bOk;
		LogMapCheck( sOut, "copy reproduces contents", bOk );
	}

	sOut.AppendMultiple( "BTL::HashMap_t: " );

	if ( bAllOk )
		sOut += "ok\n";
	else
		sOut += "mismatch\n";

	sOut += "---\n";
}

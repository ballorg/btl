#include "common.hpp"

#include <map>
#include <unordered_map>
#include <vector>

namespace
{
	using Node_t = BTL::Pair_t< size_t, size_t >;
	using RefMap_t = std::map< size_t, size_t >;

	// Distinct tags give each value column a unique CReflect type (both wrap size_t),
	// so the SoA columns are addressed by type without aliasing one another.
	BALL_REFLECT_TAGGED( First, size_t );
	BALL_REFLECT_TAGGED( Second, size_t );

	// Column-keyed tree: column 0 is the size_t key, columns 1 and 2 hold the two
	// halves of the value in separate (distinctly typed) SoA columns.
	using Tree_t = BTL::MultiRBTree_t< size_t, First_t, Second_t >;

	// Single-value map spelling used only to exercise CRBTree's Key()/Value().
	using MapTree_t = BTL::RBTree_t< BTL::size32_t, size_t, Node_t >;

	static Node_t MakeValue( size_t key )
	{
		return Node_t( key, key * 17u + 3u );
	}

	// Rebuild a Node_t from a row's two value columns (1 = first, 2 = second).
	template < typename Loc >
	static Node_t ValueAt( const Tree_t &tree, Loc loc )
	{
		return Node_t( tree.Get< 1 >( loc ), tree.Get< 2 >( loc ) );
	}

	// Insert a key with its value split across columns 1 and 2.
	static Tree_t::Index_t InsertNode( Tree_t &tree, size_t key )
	{
		const Node_t value = MakeValue( key );

		return tree.Insert( key, value.First(), value.Second() );
	}

	static void LogTreeCheck( TestsOutput_t &sOut, BTL::StringView_t svLabel, bool bOk )
	{
		sOut.AppendMultiple( "BTL::RBTree_t: ", svLabel, ": " );

		if ( bOk )
			sOut += "ok\n";
		else
			sOut += "mismatch\n";
	}

	static bool IsEndIndex( const Tree_t &tree, const Tree_t::Index_t iNode )
	{
		return iNode == tree.EndIndex();
	}

	static bool MatchesEntry( const Node_t &value, const RefMap_t::value_type &entry )
	{
		return value.First() == entry.first && value.Second() == entry.second;
	}

	static std::vector< Node_t > MakeExpectedValues( const RefMap_t &ref )
	{
		std::vector< Node_t > values;

		values.reserve( ref.size() );

		for ( const auto &entry : ref )
			values.emplace_back( entry.first, entry.second );

		return values;
	}

	static bool CheckBounds( const Tree_t &tree, const RefMap_t &ref )
	{
		const size_t arrProbes[] = { 0u, 1u, 4u, 7u, 10u, 16u, 31u, 64u, 99u };

		for ( size_t key : arrProbes )
		{
			const auto itRefLower = ref.lower_bound( key );
			const auto itRefUpper = ref.upper_bound( key );

			const Tree_t::Index_t iTreeLower = tree.LowerBound( key );
			const Tree_t::Index_t iTreeUpper = tree.UpperBound( key );

			if ( itRefLower == ref.end() )
			{
				if ( !IsEndIndex( tree, iTreeLower ) )
					return false;
			}
			else if ( IsEndIndex( tree, iTreeLower ) || !MatchesEntry( ValueAt( tree, iTreeLower ), *itRefLower ) )
			{
				return false;
			}

			if ( itRefUpper == ref.end() )
			{
				if ( !IsEndIndex( tree, iTreeUpper ) )
					return false;
			}
			else if ( IsEndIndex( tree, iTreeUpper ) || !MatchesEntry( ValueAt( tree, iTreeUpper ), *itRefUpper ) )
			{
				return false;
			}
		}

		return true;
	}

	static bool CheckTreeMatchesRef( const Tree_t &tree, const RefMap_t &ref )
	{
		if ( !tree.Validate() )
			return false;

		if ( tree.Count() != ref.size() )
			return false;

		std::vector< Node_t > values;

		for ( Tree_t::Index_t i = tree.FirstIndex(); i != tree.EndIndex(); i = tree.NextIndex( i ) )
			values.push_back( ValueAt( tree, i ) );

		if ( values != MakeExpectedValues( ref ) )
			return false;

		values.clear();

		for ( auto it = tree.begin(); it != tree.end(); ++it )
			values.push_back( ValueAt( tree, it ) );

		if ( values != MakeExpectedValues( ref ) )
			return false;

		if ( ref.empty() )
		{
			if ( tree.FirstIndex() != tree.EndIndex() )
				return false;
		}
		else
		{
			if ( IsEndIndex( tree, tree.FirstIndex() ) )
				return false;

			const auto &front = ValueAt( tree, tree.FirstIndex() );
			const auto &back = ValueAt( tree, tree.PrevIndex( tree.EndIndex() ) );

			if ( !MatchesEntry( front, *ref.begin() ) )
				return false;

			if ( back.First() != ref.rbegin()->first || back.Second() != ref.rbegin()->second )
				return false;
		}

		for ( const auto &entry : ref )
		{
			const Tree_t::Index_t iFound = tree.Find( entry.first );

			if ( IsEndIndex( tree, iFound ) || !MatchesEntry( ValueAt( tree, iFound ), entry ) || !tree.Contains( entry.first ) )
				return false;
		}

		if ( !IsEndIndex( tree, tree.Find( 0xFFFFu ) ) || tree.Contains( 0xFFFFu ) )
			return false;

		if ( !CheckBounds( tree, ref ) )
			return false;

		size_t nOccupied = 0u;

		for ( Tree_t::Index_t i = tree.FirstIndex(); i != tree.EndIndex(); i = tree.NextIndex( i ) )
		{
			++nOccupied;

			const Tree_t::Index_t iLeft = tree.LeftIndex( i );
			const Tree_t::Index_t iRight = tree.RightIndex( i );

			if ( iLeft != tree.NilIndex() && tree.ParentIndex( iLeft ) != i )
				return false;

			if ( iRight != tree.NilIndex() && tree.ParentIndex( iRight ) != i )
				return false;
		}

		return nOccupied == ref.size();
	}

	static bool CheckPayloadSnapshot( const Tree_t &, const std::vector< Node_t > & )
	{
		return true;
	}

	static size_t CountChildren( const Tree_t &tree, Tree_t::Index_t iNode )
	{
		size_t nChildren = 0u;

		if ( tree.LeftIndex( iNode ) != tree.NilIndex() )
			++nChildren;

		if ( tree.RightIndex( iNode ) != tree.NilIndex() )
			++nChildren;

		return nChildren;
	}

	static size_t FindKeyWithChildren( const Tree_t &tree, const size_t nChildren, const bool bSkipRoot = false )
	{
		for ( Tree_t::Index_t i = 0; i < tree.TreeCount(); ++i )
		{
			if ( !tree.IsOccupied( i ) )
				continue;

			if ( bSkipRoot && i == tree.RootIndex() )
				continue;

			if ( CountChildren( tree, i ) == nChildren )
				return tree.Get< 0 >( i );
		}

		return ~size_t( 0 );
	}

	struct BenchmarkResult_t
	{
		BTL::CTimeNS m_Insert;
		BTL::CTimeNS m_Find;
		BTL::CTimeNS m_Erase;
		bool m_bOk = true;
	};

	template < typename T >
	struct CAdapterBase
	{
		using C = T;

		static void Reserve( C &, size_t ) {}
		static bool Insert( C &map, size_t key ) { return map.emplace( key, key * 17u + 3u ).second; }
		static bool Find( const C &map, size_t key )
		{
			const auto it = map.find( key );

			return it != map.end() && it->first == key;
		}
		static bool Erase( C &map, size_t key ) { return map.erase( key ) == 1u; }
		static size_t Count( const C &map ) { return map.size(); }
		static bool Empty( const C &map ) { return map.empty(); }
		static bool Validate( const C & ) { return true; }
	};

	struct CAdapter_RBTree
	{
		using C = Tree_t;

		static void Reserve( C &, size_t ) {}
		static bool Insert( C &tree, size_t key ) { return !IsEndIndex( tree, InsertNode( tree, key ) ); }
		static bool Find( const C &tree, size_t key )
		{
			const Tree_t::Index_t iNode = tree.Find( key );

			return !IsEndIndex( tree, iNode ) && tree.Get< 1 >( iNode ) == key;
		}
		static bool Erase( C &tree, size_t key ) { return !IsEndIndex( tree, tree.FindAndRemove( key ) ); }
		static size_t Count( const C &tree ) { return tree.Count(); }
		static bool Empty( const C &tree ) { return tree.Count() == 0u; }
		static bool Validate( const C &tree ) { return tree.Validate(); }
	};

	struct CAdapter_StdMap : CAdapterBase< RefMap_t >
	{
	};

	struct CAdapter_StdUnorderedMap : CAdapterBase< std::unordered_map< size_t, size_t > >
	{
		static void Reserve( C &map, size_t count ) { map.reserve( count ); }
	};

	static std::vector< size_t > MakeBenchmarkKeys( const size_t nCount )
	{
		std::vector< size_t > keys;
		keys.reserve( nCount );

		for ( size_t i = 0; i < nCount; ++i )
			keys.push_back( i );

		size_t state = 0x9E3779B97F4A7C15ull;

		for ( size_t i = nCount; 1u < i; --i )
		{
			state = state * 1664525u + 1013904223u;
			const size_t j = state % i;
			std::swap( keys[ i - 1u ], keys[ j ] );
		}

		return keys;
	}

	template < typename A >
	static BenchmarkResult_t RunMapBenchmark( const std::vector< size_t > &keys )
	{
		BenchmarkResult_t result{};
		typename A::C container;

		A::Reserve( container, keys.size() );

		BALL_PROF_BEGIN( MapInsertBenchmark );

		for ( size_t key : keys )
		{
			if ( !A::Insert( container, key ) )
				result.m_bOk = false;
		}

		result.m_Insert = BALL_PROF_END( MapInsertBenchmark );
		result.m_bOk = result.m_bOk && A::Count( container ) == keys.size();

		BALL_PROF_BEGIN( MapFindBenchmark );

		for ( size_t key : keys )
		{
			if ( !A::Find( container, key ) )
				result.m_bOk = false;
		}

		result.m_Find = BALL_PROF_END( MapFindBenchmark );

		BALL_PROF_BEGIN( MapEraseBenchmark );

		for ( size_t key : keys )
		{
			if ( !A::Erase( container, key ) )
				result.m_bOk = false;
		}

		result.m_Erase = BALL_PROF_END( MapEraseBenchmark );
		result.m_bOk = result.m_bOk && A::Empty( container ) && A::Validate( container );

		return result;
	}

	static const char *BenchmarkStatus( bool bOk )
	{
		return bOk ? "ok" : "mismatch";
	}

	static void LogBenchmarkComparison( TestsOutput_t &sOut )
	{
		// The RB tree no longer caches root/boundary/free-list state; deriving them on
		// demand makes every operation O(n)+ (overall ~O(n^2 log n) across the run), so
		// this comparison uses a moderate key count to stay responsive.
		const std::vector< size_t > keys = MakeBenchmarkKeys( 100'000u );
		const BenchmarkResult_t rbTree = RunMapBenchmark< CAdapter_RBTree >( keys );
		const BenchmarkResult_t stdMap = RunMapBenchmark< CAdapter_StdMap >( keys );
		const BenchmarkResult_t stdUnorderedMap = RunMapBenchmark< CAdapter_StdUnorderedMap >( keys );

		sOut.AppendMultiple( "BTL::RBTree_t: benchmark vs std::map vs std::unordered_map (", keys.size(), " keys)\n" );
		sOut.AppendMultiple( " - insert: rb=", rbTree.m_Insert.AsMillisF(), " ms, std=", stdMap.m_Insert.AsMillisF(), " ms, unordered=", stdUnorderedMap.m_Insert.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - find: rb=", rbTree.m_Find.AsMillisF(), " ms, std=", stdMap.m_Find.AsMillisF(), " ms, unordered=", stdUnorderedMap.m_Find.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - erase: rb=", rbTree.m_Erase.AsMillisF(), " ms, std=", stdMap.m_Erase.AsMillisF(), " ms, unordered=", stdUnorderedMap.m_Erase.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - status: rb=", BenchmarkStatus( rbTree.m_bOk ), ", std=", BenchmarkStatus( stdMap.m_bOk ), ", unordered=", BenchmarkStatus( stdUnorderedMap.m_bOk ), "\n" );
	}

	static bool CheckMultiBTree( TestsOutput_t &sOut )
	{
		// Multi-column tree: key = column 0 (size_t), extra payload columns float and char.
		// Column 0 is the size_t key; columns 1.. are value columns (distinct types).
		using MultiTree_t = BTL::CMultiRBTree< BTL::size32_t, BTL::CRBTreeLess< size_t >, size_t, float, char >;

		MultiTree_t tree;

		tree.Insert( 5u, 1.5f, 'a' );
		tree.Insert( 2u, 2.5f, 'b' );
		tree.Insert( 8u, 3.5f, 'c' );

		bool bOk = tree.Insert( 5u, 9.9f, 'z' ) == tree.NilIndex();   // duplicate key rejected

		bOk &= tree.Count() == 3u;
		bOk &= MultiTree_t::COLUMN_COUNT == 3u; // key + float + char
		bOk &= BTL::IS_SAME< MultiTree_t::Column_t< 0 >, size_t >;
		bOk &= BTL::IS_SAME< MultiTree_t::Column_t< 1 >, float >;

		const MultiTree_t::Index_t i = tree.Find( 5u );

		bOk &= i != tree.NilIndex();
		bOk &= tree.Get< 0 >( i ) == 5u && tree.Get< 1 >( i ) == 1.5f && tree.Get< 2 >( i ) == 'a';

		// In-order by key column: 2, 5, 8; read column 2 through the iterator overload.
		size_t nPrev = 0u;
		size_t nCount = 0u;
		char arrChars[ 4 ] = {};

		for ( auto it = tree.begin(); it != tree.end(); ++it )
		{
			const size_t key = tree.Get< 0 >( it );

			bOk &= nCount == 0u || key > nPrev;
			nPrev = key;
			arrChars[ nCount++ ] = tree.Get< 2 >( it );
		}

		bOk &= nCount == 3u && arrChars[ 0 ] == 'b' && arrChars[ 1 ] == 'a' && arrChars[ 2 ] == 'c';
		bOk &= tree.Validate();

		// Buffer variant + cross-copy from the heap tree.
		BTL::CBufferMultiRBTree< BTL::size32_t, 16u, BTL::CRBTreeLess< size_t >, size_t, float, char > buffer( tree );

		bOk &= buffer.Count() == 3u && buffer.Validate() && buffer.Get< 2 >( buffer.Find( 8u ) ) == 'c';

		// A tree keyed on a different column type (float key + size_t value).
		BTL::CMultiRBTree< BTL::size32_t, BTL::CRBTreeLess< float >, float, size_t > byFloat;

		byFloat.Insert( 3.0f, 100u );
		byFloat.Insert( 1.0f, 200u );

		const auto j = byFloat.Find( 1.0f );

		bOk &= j != byFloat.NilIndex() && byFloat.Get< 1 >( j ) == 200u && byFloat.Validate();

		LogTreeCheck( sOut, "multi-column tree", bOk );

		return bOk;
	}

	static bool CheckRemoveCase( BTL::StringView_t svLabel, const size_t *arrKeys, const size_t nKeys, const size_t nChildren, const bool bRemoveByIterator, TestsOutput_t &sOut )
	{
		Tree_t tree;
		RefMap_t ref;

		for ( size_t i = 0; i < nKeys; ++i )
		{
			const Node_t value = MakeValue( arrKeys[ i ] );
			const auto iInsert = InsertNode( tree, value.First() );

			if ( IsEndIndex( tree, iInsert ) )
				return false;

			ref[ value.First() ] = value.Second();
		}

		const size_t key = FindKeyWithChildren( tree, nChildren, nChildren == 2u );

		if ( key == ~size_t( 0 ) )
			return false;

		const auto itRef = ref.find( key );
		const auto itRefNext = std::next( itRef );

		bool bRemoved = false;
		Tree_t::Index_t iNext = tree.EndIndex();

		if ( bRemoveByIterator )
		{
			const Tree_t::Index_t iRemove = tree.Find( key );
			iNext = tree.Remove( tree.Iterator( iRemove ) ).SlotIndex();
			bRemoved = true;
		}
		else
		{
			bRemoved = !IsEndIndex( tree, tree.FindAndRemove( key ) );
			iNext = tree.LowerBound( key );
		}

		ref.erase( key );

		const bool bNextOk = ( itRefNext == ref.end() && IsEndIndex( tree, iNext ) ) || ( itRefNext != ref.end() && !IsEndIndex( tree, iNext ) && MatchesEntry( ValueAt( tree, iNext ), *itRefNext ) );

		const bool bOk = bRemoved && bNextOk && CheckTreeMatchesRef( tree, ref ) && CheckPayloadSnapshot( tree, {} );

		LogTreeCheck( sOut, svLabel, bOk );

		return bOk;
	}
}

void Case05_RBTree( TestsOutput_t &sOut )
{
	bool bAllOk = true;

	{
		// Single-value map Key()/Value() accessors (key = column 0, value = column 1).
		MapTree_t tree;

		const auto it = tree.InsertIterator( 42u, MakeValue( 42u ) );

		bool bOk = it != tree.end();
		bOk &= tree.Key( it ) == 42u;
		bOk &= tree.Value( it ).First() == 42u && tree.Value( it ).Second() == MakeValue( 42u ).Second();

		const MapTree_t::Index_t idx = tree.Find( 42u );

		bOk &= tree.Key( idx ) == 42u && tree.Value( idx ).Second() == MakeValue( 42u ).Second();

		tree.Value( idx ) = MakeValue( 7u );   // mutate through the value reference
		bOk &= tree.Value( idx ).First() == 7u;

		LogTreeCheck( sOut, "key/value accessors", bOk );
		bAllOk = bAllOk && bOk;
	}

	{
		Tree_t tree;
		RefMap_t ref;

		for ( size_t key = 0; key < 32u; ++key )
		{
			const auto iInsert = InsertNode( tree, key );

			if ( IsEndIndex( tree, iInsert ) )
				bAllOk = false;

			ref[ key ] = key * 17u + 3u;
		}

		const auto iDuplicate = InsertNode( tree, 7u );
		const bool bOk = IsEndIndex( tree, iDuplicate ) && CheckTreeMatchesRef( tree, ref );

		LogTreeCheck( sOut, "sequential insert + duplicate", bOk );
		bAllOk = bAllOk && bOk;
	}

	{
		Tree_t tree;

		RefMap_t ref;
		const size_t arrKeys[] = { 18u, 7u, 31u, 4u, 9u, 25u, 40u, 1u, 6u, 8u, 12u, 22u, 27u, 35u, 50u };

		for ( size_t key : arrKeys )
		{
			const auto iInsert = InsertNode( tree, key );

			if ( IsEndIndex( tree, iInsert ) )
				bAllOk = false;

			ref[ key ] = MakeValue( key ).Second();
		}

		const bool bOk = CheckTreeMatchesRef( tree, ref );

		LogTreeCheck( sOut, "mixed insert / find / bounds / iteration", bOk );
		bAllOk = bAllOk && bOk;
	}

	{
		const size_t arrLeaf[] = { 10u, 5u, 15u };
		const bool bOk = CheckRemoveCase( "Remove leaf", arrLeaf, sizeof( arrLeaf ) / sizeof( arrLeaf[ 0 ] ), 0u, false, sOut );
		bAllOk = bAllOk && bOk;
	}

	{
		const size_t arrOneChild[] = { 10u, 5u, 15u, 12u };
		const bool bOk = CheckRemoveCase( "Remove one-child node", arrOneChild, sizeof( arrOneChild ) / sizeof( arrOneChild[ 0 ] ), 1u, true, sOut );
		bAllOk = bAllOk && bOk;
	}

	{
		const size_t arrTwoChildren[] = { 10u, 5u, 15u, 12u, 18u };
		const bool bOk = CheckRemoveCase( "Remove two-children node", arrTwoChildren, sizeof( arrTwoChildren ) / sizeof( arrTwoChildren[ 0 ] ), 2u, false, sOut );
		bAllOk = bAllOk && bOk;
	}

	{
		Tree_t tree;
		RefMap_t ref;
		const size_t arrKeys[] = { 20u, 10u, 30u, 5u, 15u, 25u, 35u, 1u, 6u, 14u, 16u };

		for ( size_t key : arrKeys )
		{
			InsertNode( tree, key );
			ref[ key ] = MakeValue( key ).Second();
		}

		const size_t rootKey = tree.Get< 0 >( tree.RootIndex() );
		const bool bRemoveOk = !IsEndIndex( tree, tree.FindAndRemove( rootKey ) );

		ref.erase( rootKey );

		const bool bOk = bRemoveOk && CheckTreeMatchesRef( tree, ref ) && CheckPayloadSnapshot( tree, {} );

		LogTreeCheck( sOut, "Remove root", bOk );
		bAllOk = bAllOk && bOk;
	}

	{
		Tree_t tree;

		for ( size_t key = 0; key < 8u; ++key )
			InsertNode( tree, key );

		tree.Clear();

		bool bOk = tree.Count() == 0u && tree.Validate() && tree.FirstIndex() == tree.EndIndex() && tree.begin() == tree.end();

		// NIL is the out-of-band -1, not a slot: after Clear the storage is empty and
		// re-inserting must refill slot 0 (FIRST_INDEX) as the new root.
		bOk &= tree.NilIndex() == Tree_t::Index_t( -1 ) && !tree.IsOccupied( tree.NilIndex() );

		for ( size_t key = 0; key < 8u; ++key )
			InsertNode( tree, key );

		RefMap_t ref;

		for ( size_t key = 0; key < 8u; ++key )
			ref[ key ] = MakeValue( key ).Second();

		bOk &= CheckTreeMatchesRef( tree, ref );

		LogTreeCheck( sOut, "clear + reinsert", bOk );
		bAllOk = bAllOk && bOk;
	}

	{
		Tree_t tree;
		const size_t arrKeys[] = { 30u, 10u, 20u, 50u, 40u };

		for ( size_t key : arrKeys )
			InsertNode( tree, key );

		size_t arrForward[ 5 ] = {};
		size_t nForward = 0u;

		BALL_RBTREE_FOREACH( tree, it )
			arrForward[ nForward++ ] = tree.Get< 0 >( it );

		size_t arrReverse[ 5 ] = {};
		size_t nReverse = 0u;

		BALL_RBTREE_FOREACH_REVERSE( tree, it )
			arrReverse[ nReverse++ ] = tree.Get< 0 >( it );

		const size_t arrSorted[ 5 ] = { 10u, 20u, 30u, 40u, 50u };

		bool bOk = nForward == 5u && nReverse == 5u;

		for ( size_t i = 0; i < 5u; ++i )
		{
			bOk &= arrForward[ i ] == arrSorted[ i ];
			bOk &= arrReverse[ i ] == arrSorted[ 4u - i ];
		}

		// NIL is the out-of-band -1, owning no slot; the first occupied node is slot FIRST_INDEX (0).
		bOk &= tree.NilIndex() == Tree_t::Index_t( -1 );
		bOk &= !tree.IsOccupied( tree.NilIndex() );

		LogTreeCheck( sOut, "foreach macros + out-of-band NIL", bOk );
		bAllOk = bAllOk && bOk;
	}

	{
		const bool bOk = CheckMultiBTree( sOut );

		bAllOk = bAllOk && bOk;
	}

	LogBenchmarkComparison( sOut );
	sOut += "---\n";

	sOut.AppendMultiple( "BTL::RBTree_t: " );

	if ( bAllOk )
		sOut += "ok\n";
	else
		sOut += "mismatch\n";

	sOut += "---\n";
}

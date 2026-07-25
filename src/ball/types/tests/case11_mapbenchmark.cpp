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

	static Node_t Make( size_t key )
	{
		return Node_t( key, key * 17u + 3u );
	}

	static bool IsEndIndex( const Tree_t &tree, const Tree_t::Index_t iNode )
	{
		return iNode == tree.EndIndex();
	}

	// Insert a key with its value split across columns 1 and 2.
	static Tree_t::Index_t InsertNode( Tree_t &tree, size_t key )
	{
		const Node_t value = Make( key );

		return tree.Insert( key, value.First(), value.Second() );
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

	struct CAdapter_HashMap
	{
		using C = BTL::HashMap32_t< size_t, size_t >;

		static void Reserve( C &, size_t ) {}
		static bool Insert( C &map, size_t key ) { return map.Insert( key, key * 17u + 3u ) != map.EndIndex(); }
		static bool Find( const C &map, size_t key )
		{
			const auto iSlot = map.Find( key );

			return iSlot != map.EndIndex() && map.Value( iSlot ) == key * 17u + 3u;
		}
		static bool Erase( C &map, size_t key ) { return map.Remove( key ); }
		static size_t Count( const C &map ) { return map.Count(); }
		static bool Empty( const C &map ) { return map.IsEmpty(); }
		static bool Validate( const C & ) { return true; }
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

	static BTL::StringView_t BenchmarkStatus( bool bOk )
	{
		return bOk ? BTL::StringView_t( "ok" ) : BTL::StringView_t( "mismatch" );
	}

	static void LogBenchmarkComparison( TestsOutput_t &sOut )
	{
		// The RB tree no longer caches root/boundary/free-list state; deriving them on
		// demand makes every operation O(n)+ (overall ~O(n^2 log n) across the run), so
		// this comparison uses a moderate key count to stay responsive.
		const std::vector< size_t > keys = MakeBenchmarkKeys( 100'000u );
		const BenchmarkResult_t rbTree = RunMapBenchmark< CAdapter_RBTree >( keys );
		const BenchmarkResult_t stdMap = RunMapBenchmark< CAdapter_StdMap >( keys );
		const BenchmarkResult_t hashMap = RunMapBenchmark< CAdapter_HashMap >( keys );
		const BenchmarkResult_t stdUnorderedMap = RunMapBenchmark< CAdapter_StdUnorderedMap >( keys );

		sOut.AppendMultiple( "BTL::RBTree_t: benchmark vs std::map vs BTL::HashMap_t vs std::unordered_map (", keys.size(), " keys)\n" );
		sOut.AppendMultiple( " - insert: rb=", rbTree.m_Insert.AsMillisF(), " ms, std=", stdMap.m_Insert.AsMillisF(), " ms, hashmap=", hashMap.m_Insert.AsMillisF(), " ms, unordered=", stdUnorderedMap.m_Insert.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - find: rb=", rbTree.m_Find.AsMillisF(), " ms, std=", stdMap.m_Find.AsMillisF(), " ms, hashmap=", hashMap.m_Find.AsMillisF(), " ms, unordered=", stdUnorderedMap.m_Find.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - erase: rb=", rbTree.m_Erase.AsMillisF(), " ms, std=", stdMap.m_Erase.AsMillisF(), " ms, hashmap=", hashMap.m_Erase.AsMillisF(), " ms, unordered=", stdUnorderedMap.m_Erase.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - status: rb=", BenchmarkStatus( rbTree.m_bOk ), ", std=", BenchmarkStatus( stdMap.m_bOk ), ", hashmap=", BenchmarkStatus( hashMap.m_bOk ), ", unordered=", BenchmarkStatus( stdUnorderedMap.m_bOk ), "\n" );
	}
}

void Case11_MapBenchmark( TestsOutput_t &sOut )
{
	LogBenchmarkComparison( sOut );
	sOut += "---\n";
}

module;

#include <ball/time/macros.h>

#include <algorithm>
#include <map>
#include <unordered_map>
#include <vector>

module Ball.Types;

import Ball.New;
import Ball.Time;
import :Meta;
import :HashMap;
import :RBTree;
import :String;
import :StringView;
import :Tests.Case11;

using TestsOutput_t = BTL::BufferString_t< 4096 >;

namespace
{
	using RefMap_t = std::map< size_t, size_t >;
	using Tree_t = BTL::RBTree32_t< size_t, size_t >;

	static bool IsEndIndex( const Tree_t &tree, const Tree_t::Index_t iNode )
	{
		return iNode == tree.EndIndex();
	}

	static Tree_t::Index_t InsertNode( Tree_t &tree, size_t key )
	{
		return tree.Insert( key, key * 17u + 3u );
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
		static constexpr size_t FIND_PASSES = 1u;

		static void Reserve( C &, size_t ) {}
		static bool Insert( C &map, size_t key ) { return map.emplace( key, key * 17u + 3u ).second; }
		static bool Find( const C &map, size_t key )
		{
			const auto it = map.find( key );

			return it != map.end();
		}
		static bool Erase( C &map, size_t key ) { return map.erase( key ) == 1u; }
		static size_t Count( const C &map ) { return map.size(); }
		static bool Empty( const C &map ) { return map.empty(); }
		static bool Validate( const C & ) { return true; }
	};

	struct CAdapter_StdUnorderedMap : CAdapterBase< std::unordered_map< size_t, size_t > >
	{
		static constexpr size_t FIND_PASSES = 32u;
		static void Reserve( C &map, size_t count ) { map.reserve( count ); }
	};

	struct CAdapter_RBTree
	{
		using C = Tree_t;
		static constexpr size_t FIND_PASSES = 1u;

		static void Reserve( C &, size_t ) {}
		static bool Insert( C &tree, size_t key ) { return !IsEndIndex( tree, InsertNode( tree, key ) ); }
		static bool Find( const C &tree, size_t key )
		{
			const Tree_t::Index_t iNode = tree.Find( key );

			return !IsEndIndex( tree, iNode );
		}
		static bool Erase( C &tree, size_t key ) { return !IsEndIndex( tree, tree.FindAndRemove( key ) ); }
		static size_t Count( const C &tree ) { return tree.Count(); }
		static bool Empty( const C &tree ) { return tree.Count() == 0u; }
		static bool Validate( const C &tree ) { return tree.Validate(); }
	};

	struct CAdapter_StdMap : CAdapterBase< RefMap_t >
	{
	};

	struct CAdapter_HashMap
	{
		using C = BTL::HashMap32_t< size_t, size_t >;
		static constexpr size_t FIND_PASSES = 32u;

		static void Reserve( C &map, size_t count )
		{
			const C::Index_t nBuckets = BTL::BitCeil( static_cast< C::Index_t >( count * 2u ) );

			map.SetCount( nBuckets );
		}
		static bool Insert( C &map, size_t key ) { return map.Insert( key, key * 17u + 3u ) != map.EndIndex(); }
		static bool Find( const C &map, size_t key )
		{
			return map.Find( key ) != map.EndIndex();
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
		typename A::C *pContainer = new typename A::C;
		typename A::C &container = *pContainer;

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

		for ( size_t nPass = 0; nPass < A::FIND_PASSES; ++nPass )
		{
			for ( size_t key : keys )
			{
				if ( !A::Find( container, key ) )
					result.m_bOk = false;
			}
		}

		result.m_Find = BTL::CTimeNS( BALL_PROF_END( MapFindBenchmark ).GetNSs() / A::FIND_PASSES );

		BALL_PROF_BEGIN( MapEraseBenchmark );

		for ( size_t key : keys )
		{
			if ( !A::Erase( container, key ) )
				result.m_bOk = false;
		}

		result.m_Erase = BALL_PROF_END( MapEraseBenchmark );
		result.m_bOk = result.m_bOk && A::Empty( container ) && A::Validate( container );
		delete pContainer;

		return result;
	}

	static BTL::CTimeNS MedianTime( const std::vector< BenchmarkResult_t > &results, BTL::CTimeNS BenchmarkResult_t::*pTime )
	{
		std::vector< BTL::timens_t > times;
		times.reserve( results.size() );

		for ( const BenchmarkResult_t &result : results )
			times.push_back( ( result.*pTime ).GetNSs() );

		std::sort( times.begin(), times.end() );

		return BTL::CTimeNS( times[ times.size() / 2u ] );
	}

	static BenchmarkResult_t MedianResult( const std::vector< BenchmarkResult_t > &results )
	{
		BenchmarkResult_t median;
		median.m_Insert = MedianTime( results, &BenchmarkResult_t::m_Insert );
		median.m_Find = MedianTime( results, &BenchmarkResult_t::m_Find );
		median.m_Erase = MedianTime( results, &BenchmarkResult_t::m_Erase );

		for ( const BenchmarkResult_t &result : results )
			median.m_bOk = median.m_bOk && result.m_bOk;

		return median;
	}

	static BTL::StringView_t BenchmarkStatus( bool bOk )
	{
		return bOk ? BTL::StringView_t( "ok" ) : BTL::StringView_t( "mismatch" );
	}

	static void LogBenchmarkComparison( TestsOutput_t &sOut )
	{
#if defined( NDEBUG )
		static constexpr size_t TRIAL_COUNT = 3u;
		static constexpr size_t KEY_COUNT = 500'000u;
#else
		static constexpr size_t TRIAL_COUNT = 1u;
		static constexpr size_t KEY_COUNT = 100'000u;
#endif
		const std::vector< size_t > keys = MakeBenchmarkKeys( KEY_COUNT );
		std::vector< BenchmarkResult_t > rbTreeTrials;
		std::vector< BenchmarkResult_t > stdMapTrials;
		std::vector< BenchmarkResult_t > hashMapTrials;
		std::vector< BenchmarkResult_t > stdUnorderedTrials;
		rbTreeTrials.reserve( TRIAL_COUNT );
		stdMapTrials.reserve( TRIAL_COUNT );
		hashMapTrials.reserve( TRIAL_COUNT );
		stdUnorderedTrials.reserve( TRIAL_COUNT );

		for ( size_t i = 0; i < TRIAL_COUNT; ++i )
		{
			if ( !( i & 1u ) )
			{
				rbTreeTrials.push_back( RunMapBenchmark< CAdapter_RBTree >( keys ) );
				stdMapTrials.push_back( RunMapBenchmark< CAdapter_StdMap >( keys ) );
				hashMapTrials.push_back( RunMapBenchmark< CAdapter_HashMap >( keys ) );
				stdUnorderedTrials.push_back( RunMapBenchmark< CAdapter_StdUnorderedMap >( keys ) );
			}
			else
			{
				stdUnorderedTrials.push_back( RunMapBenchmark< CAdapter_StdUnorderedMap >( keys ) );
				hashMapTrials.push_back( RunMapBenchmark< CAdapter_HashMap >( keys ) );
				stdMapTrials.push_back( RunMapBenchmark< CAdapter_StdMap >( keys ) );
				rbTreeTrials.push_back( RunMapBenchmark< CAdapter_RBTree >( keys ) );
			}
		}

		const BenchmarkResult_t rbTree = MedianResult( rbTreeTrials );
		const BenchmarkResult_t stdMap = MedianResult( stdMapTrials );
		const BenchmarkResult_t hashMap = MedianResult( hashMapTrials );
		const BenchmarkResult_t stdUnorderedMap = MedianResult( stdUnorderedTrials );

		sOut.AppendMultiple( "BTL::RBTree_t: benchmark median of ", TRIAL_COUNT, " trials vs std::map (", keys.size(), " keys)\n" );
		sOut.AppendMultiple( " - insert: rbtree=", rbTree.m_Insert.AsMillisF(), " ms, map=", stdMap.m_Insert.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - find: rbtree=", rbTree.m_Find.AsMillisF(), " ms, map=", stdMap.m_Find.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - erase: rbtree=", rbTree.m_Erase.AsMillisF(), " ms, map=", stdMap.m_Erase.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - total: rbtree=", rbTree.m_Insert.AsMillisF() + rbTree.m_Find.AsMillisF() + rbTree.m_Erase.AsMillisF(), " ms, map=", stdMap.m_Insert.AsMillisF() + stdMap.m_Find.AsMillisF() + stdMap.m_Erase.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - status: rbtree=", BenchmarkStatus( rbTree.m_bOk ), ", map=", BenchmarkStatus( stdMap.m_bOk ), "\n" );
		sOut += "---\n";

		sOut.AppendMultiple( "BTL::HashMap_t: benchmark median of ", TRIAL_COUNT, " trials vs std::unordered_map (", keys.size(), " keys)\n" );
		sOut.AppendMultiple( " - insert: hashmap=", hashMap.m_Insert.AsMillisF(), " ms, unordered=", stdUnorderedMap.m_Insert.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - find: hashmap=", hashMap.m_Find.AsMillisF(), " ms, unordered=", stdUnorderedMap.m_Find.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - erase: hashmap=", hashMap.m_Erase.AsMillisF(), " ms, unordered=", stdUnorderedMap.m_Erase.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - total: hashmap=", hashMap.m_Insert.AsMillisF() + hashMap.m_Find.AsMillisF() + hashMap.m_Erase.AsMillisF(), " ms, unordered=", stdUnorderedMap.m_Insert.AsMillisF() + stdUnorderedMap.m_Find.AsMillisF() + stdUnorderedMap.m_Erase.AsMillisF(), " ms\n" );
		sOut.AppendMultiple( " - status: hashmap=", BenchmarkStatus( hashMap.m_bOk ), ", unordered=", BenchmarkStatus( stdUnorderedMap.m_bOk ), "\n" );
	}
}

void Case11_MapBenchmark( TestsOutput_t &sOut )
{
	LogBenchmarkComparison( sOut );
	sOut += "---\n";
}

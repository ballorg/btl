#ifdef BALL_ENABLE_MODULES
import Ball.New;
import Ball.Types;
#else // !defined( BALL_ENABLE_MODULES )
#	include <ball/new.hpp>
#	include <ball/types.hpp>
#endif // defined( BALL_ENABLE_MODULES )

using namespace Ball::Types;

struct pair_t
{
	size_t first;
	size_t second;

	bool operator==( const pair_t &diff ) const { return first == diff.first && second == diff.second; }
};

// Extern C section.
extern "C"
{
	int puts( const char *pszTextNoNextLine );
};

template < class V >
String_t DumpVector( const V &vec )
{
	BufferString_t< 1024 > sBuffer;

	sBuffer.AppendMultiple( "Count: ", vec.Count(), "\n" );
	sBuffer.AppendMultiple( "Capacity: ", vec.Capacity(), "\n" );
	sBuffer.AppendMultiple( "Size: ", vec.Size(), "\n" );
	sBuffer.AppendMultiple( "Capacity size: ", vec.CapacitySize(), "\n" );

	if constexpr ( V::IS_GROWABLE )
	{
		sBuffer.AppendMultiple( "Fixed count: ", vec.FixedCount(), "\n" );
		sBuffer.AppendMultiple( "Fixed size: ", vec.FixedSize(), "\n" );
		sBuffer.AppendMultiple( "Fixed capacity: ", vec.FixedCapacity(), "\n" );
		sBuffer.AppendMultiple( "Fixed capacity size: ", vec.FixedCapacitySize(), "\n" );
		sBuffer.AppendMultiple( "Is overflowed: ", vec.IsOverflow(), "\n" );
	}

	sBuffer += "---\n";

	for ( const auto &it : vec )
	{
		sBuffer.AppendMultiple("*", &it, " = {", it.first, ", ", it.second, "}\n");
	}

	sBuffer += "---\n";

	return sBuffer;
}

// Entry point section.
int main()
{
	Vector_t< pair_t > vec;

	{
		for ( size_t n = 0; n < 100'000; n += 2 )
		{
			vec.AddToTail( pair_t{ n + 1, n + 2 } );
			// vec.AddToTail( pair_t{ 11, 12 } );
			// vec.AddToTail( pair_t{ 13, 14 } );
		}

		auto nCount = vec.Count();

		if ( nCount >= 2 )
		{
			vec.Insert( vec.Remove( nCount - 2 ), pair_t{ 0u, 0u } );
		}

		auto str = DumpVector( vec );

		str.ReplaceAll( "{0, 0}", "<removed element>" );

		{
			constexpr decltype( vec )::Element_t search { 11, 12 };

			auto iFound = vec.Find( search );

			if ( vec.IsValidIndex( iFound ) )
			{
				str.AppendMultiple( "Found {", search.first, ", ", search.second, "} element at index ", iFound, "\n" );
			}
			else
			{
				str.AppendMultiple( "Can't find {", search.first, ", ", search.second, "} element \n" );
			}
		}

		str += "---\0";

		puts( str.String() );
	}

	return 0;
}

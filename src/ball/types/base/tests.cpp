import Ball.New;
import Ball.Types;

// Macros section.
#define BALL_STATIC_ASSERT_SIZEOF( type, size ) static_assert( sizeof( type ) == ( size ), "sizeof( " #type " ) must be " #size " bytes")
#define BALL_STATIC_ASSERT_SIZEOF_BITS( type, bits ) BALL_STATIC_ASSERT_SIZEOF( type, bits / 8 )
#define BALL_APPEND_SIZEOF( buffer, type ) \
	buffer.AppendMultiple( "sizeof( " #type " ) = ", sizeof( type ), " bytes (", sizeof( type ) * 8 , " bits)\n" );


// Structure section.
union Digit_t
{
	char str[2];
	char n;

	Digit_t( const char num = 0 ) : str("0") { n += num; }
};

// Extern C section.
extern "C"
{
	int puts( const char *pszTextNoNextLine );
};

// Helpers

using namespace Ball::Types;

// Entry point section.
int main()
{
	// Fixed types.

	BALL_STATIC_ASSERT_SIZEOF_BITS( int8_t, 8 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( int16_t, 16 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( int32_t, 32 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( int64_t, 64 );

	BALL_STATIC_ASSERT_SIZEOF_BITS( uint8_t, 8 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( uint16_t, 16 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( uint32_t, 32 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( uint64_t, 64 );

	BALL_STATIC_ASSERT_SIZEOF_BITS( sint8_t, 8 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( sint16_t, 16 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( sint32_t, 32 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( sint64_t, 64 );

	// Character types.

	BALL_STATIC_ASSERT_SIZEOF_BITS( char8_t, 8 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( char16_t, 16 );
	BALL_STATIC_ASSERT_SIZEOF_BITS( char32_t, 32 );

	// Word types.

	BufferString_t< 2048 > sBuffer;

	// BALL_APPEND_SIZEOF( AppendString, sBuffer, void_t );
	BALL_APPEND_SIZEOF( sBuffer, ptr_t );
	BALL_APPEND_SIZEOF( sBuffer, nullptr_t );

	sBuffer.Append( "---\n" );

	BALL_APPEND_SIZEOF( sBuffer, char_t );
	BALL_APPEND_SIZEOF( sBuffer, short_t );
	BALL_APPEND_SIZEOF( sBuffer, int_t );
	BALL_APPEND_SIZEOF( sBuffer, long_t );
	BALL_APPEND_SIZEOF( sBuffer, llong_t );
	BALL_APPEND_SIZEOF( sBuffer, size_t );

	sBuffer.Append( "---\n" );

	BALL_APPEND_SIZEOF( sBuffer, uchar_t );
	BALL_APPEND_SIZEOF( sBuffer, ushort_t );
	BALL_APPEND_SIZEOF( sBuffer, uint_t );
	BALL_APPEND_SIZEOF( sBuffer, ulong_t );
	BALL_APPEND_SIZEOF( sBuffer, ullong_t );
	BALL_APPEND_SIZEOF( sBuffer, usize_t );

	sBuffer.Append( "---\n" );

	BALL_APPEND_SIZEOF( sBuffer, schar_t );
	BALL_APPEND_SIZEOF( sBuffer, sshort_t );
	BALL_APPEND_SIZEOF( sBuffer, sint_t );
	BALL_APPEND_SIZEOF( sBuffer, slong_t );
	BALL_APPEND_SIZEOF( sBuffer, sllong_t );
	BALL_APPEND_SIZEOF( sBuffer, ssize_t );

	sBuffer.Append( "---\n" );

	BALL_APPEND_SIZEOF( sBuffer, int8_t );
	BALL_APPEND_SIZEOF( sBuffer, int16_t );
	BALL_APPEND_SIZEOF( sBuffer, int32_t );
	BALL_APPEND_SIZEOF( sBuffer, int64_t );

	sBuffer.Append( "---\n" );

	BALL_APPEND_SIZEOF( sBuffer, uint8_t );
	BALL_APPEND_SIZEOF( sBuffer, uint16_t );
	BALL_APPEND_SIZEOF( sBuffer, uint32_t );
	BALL_APPEND_SIZEOF( sBuffer, uint64_t );

	sBuffer.Append( "---\n" );

	BALL_APPEND_SIZEOF( sBuffer, sint8_t );
	BALL_APPEND_SIZEOF( sBuffer, sint16_t );
	BALL_APPEND_SIZEOF( sBuffer, sint32_t );
	BALL_APPEND_SIZEOF( sBuffer, sint64_t );

	sBuffer.Append( "---\n" );

	BALL_APPEND_SIZEOF( sBuffer, char8_t );
	BALL_APPEND_SIZEOF( sBuffer, char16_t );
	BALL_APPEND_SIZEOF( sBuffer, char32_t );

	sBuffer.Append( "---" );

	puts( sBuffer.String() );

	return 0;
}
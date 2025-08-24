import Ball.Types.Base.Arch;
import Ball.Types.Base.Characters;
import Ball.Types.Base.Fixed;

// Macros section.
#define BALL_STATIC_ASSERT_SIZEOF( type, size ) static_assert( sizeof( type ) == ( size ), "sizeof( " #type " ) must be " #size " bytes")
#define BALL_STATIC_ASSERT_SIZEOF_BITS( type, bits ) BALL_STATIC_ASSERT_SIZEOF( type, bits / 8 )
#define BALL_APPEND_SIZEOF( func, pos, buf, type, end_str ) \
	{ \
		func( pos, buf, "sizeof( " #type " ) = "); \
		func( pos, buf, Digit_t( sizeof( type ) ).str ); \
		func( pos, buf, " bytes" ); \
		if constexpr ( end_str && end_str[0] ) \
			func( pos, buf, end_str ); \
	}


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

using namespace Ball::Types::Base::Arch;
using namespace Ball::Types::Base::Fixed;
using namespace Ball::Types::Base::Characters;

template < usize_t N >
static auto AppendString( usize_t &nPos, char_t ( &sBuffer )[ N ], cstr_t pszText ) -> usize_t
{
	usize_t n = 0;

	while ( nPos + 1 < N && *pszText )
	{
		sBuffer[ nPos ] = *pszText++;
		++nPos;
	}

	sBuffer[ nPos ] = '\0';

	return nPos;
};

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

	usize_t nPos = 0;
	char sBuffer[1024];

	// BALL_APPEND_SIZEOF( AppendString, sBuffer, void_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, ptr_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, nullptr_t, "\n" );

	AppendString( nPos, sBuffer, "---\n" );

	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, char_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, short_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, int_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, long_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, llong_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, size_t, "\n" );

	AppendString( nPos, sBuffer, "---\n" );

	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, uchar_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, ushort_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, uint_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, ulong_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, ullong_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, usize_t, "\n" );

	AppendString( nPos, sBuffer, "---\n" );

	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, schar_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, sshort_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, sint_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, slong_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, sllong_t, "\n" );
	BALL_APPEND_SIZEOF( AppendString, nPos, sBuffer, ssize_t, "" );

	puts( sBuffer );

	return 0;
}
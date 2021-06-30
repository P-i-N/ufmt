#include <ufmt/ufmt.hpp>

#include <format>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename... Args>
void TestEqualFormat( const char *f, Args &&... args )
{
	auto ufmtResult = ufmt::format( f, args... );
	auto formatResult = std::format( f, args... );

	if ( ufmtResult == formatResult )
	{
		printf( " equal: \"%s\"\n", formatResult.c_str() );
	}
	else
	{
		printf( " ERROR:\n" );
		printf( "  ufmt: \"%s\"\n", ufmtResult.c_str() );
		printf( "format: \"%s\"\n\n", formatResult.c_str() );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
	/*
	TestEqualFormat( "{:<30}", "left aligned" );
	TestEqualFormat( "{:>30}", "right aligned" );
	TestEqualFormat( "{:^30}", "centered" );
	TestEqualFormat( "{:x}", 123456789ll );
	TestEqualFormat( "{:x}", -123456789ll );
	TestEqualFormat( "{:+#X}", 123456789ll );
	TestEqualFormat( "{:+#X}", -123456789ll );
	TestEqualFormat( "{: #X}", 123456789ll );
	TestEqualFormat( "{: #X}", -123456789ll );
	TestEqualFormat( "{:#016X}", 123456789ll );
	TestEqualFormat( "{:#016X}", -123456789ll );
	TestEqualFormat( "{:#016b}", 123456789ll );
	TestEqualFormat( "{:#016b}", -123456789ll );
	TestEqualFormat( "{:f}", 3.141592653458 );
	TestEqualFormat( "{:.3f}", 3.141592653458 );
	TestEqualFormat( "{:#f}", 3.141592653458 );
	TestEqualFormat( "{:#.3f}", 3.141592653458 );
	TestEqualFormat( "{:#0.3f}", 3.141592653458 );
	TestEqualFormat( "{:#016.3f}", 3.141592653458 );
	TestEqualFormat( "{:#016.3f}", -3.141592653458 );
	TestEqualFormat( "{:+#016.3f}", 3.141592653458 );
	TestEqualFormat( "{:+#016.3F}", -3.141592653458 );
	TestEqualFormat( "{: #016.3f}", 3.141592653458 );
	TestEqualFormat( "{: #016.3F}", -3.141592653458 );
	*/

	TestEqualFormat( "{:a}", 3.141592653458 );
	TestEqualFormat( "{:a}", 3.14f );

	/*
	TestEqualFormat( "{}", 'X' );

	int x = 666;
	TestEqualFormat( "{}", reinterpret_cast<void *>( &x ) );
	*/

	return 0;
}

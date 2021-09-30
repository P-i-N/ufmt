#include <ufmt/ufmt.hpp>

#include <chrono>
#include <format>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------------------------------------------------
struct Stopwatch
{
	std::string name = "";
	std::chrono::nanoseconds start = std::chrono::high_resolution_clock::now().time_since_epoch();

	~Stopwatch()
	{
		auto duration = std::chrono::high_resolution_clock::now().time_since_epoch() - start;
		std::cout << name << ": " << duration.count() / 1000000 << " ms" << std::endl;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename... Args>
void TestEqualFormat( const char *f, Args &&... args )
{
	char buff[256] = { };

	auto ufmtResult = ufmt::format( f, args... );
	auto formatResult = std::format( f, args... );

	ufmt::format_to( buff, f, args... );

	if ( std::string( buff ) == formatResult && ufmtResult == formatResult )
	{
		printf( " equal: \"%s\"\n", formatResult.c_str() );
	}
	else
	{
		printf( " ERROR: format = \"%s\"\n", f );
		printf( "  ufmt: \"%s\"\n", ufmtResult.c_str() );
		printf( "  buff: \"%s\"\n", buff );
		printf( "format: \"%s\"\n\n", formatResult.c_str() );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename... Args>
void TestPerformance( const char *f, Args &&... args )
{
	constexpr size_t NumIterations = 1000000;

	printf( "Performance test: \"%s\", %d args\n", f, int( sizeof...( Args ) ) );

	size_t numCharsGenerated = 0;

	{
		Stopwatch sw{ "  ufmt time" };

		for ( size_t i = 0; i < NumIterations; ++i )
		{
			auto ufmtResult = ufmt::format( f, args... );
			numCharsGenerated += ufmtResult.size();
		}
	}

	if ( 1 )
	{
		Stopwatch sw{ "format time" };

		size_t numCharsGenerated = 0;

		for ( size_t i = 0; i < NumIterations; ++i )
		{
			auto ufmtResult = std::format( f, args... );
			numCharsGenerated += ufmtResult.size();
		}
	}

	std::cout << "Chars generated: " << numCharsGenerated << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
	if ( 0 )
	{
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

		TestEqualFormat( "Hubble's H{0} {1} {2} km/sec/mpc.", "0", "=", 71 );
	}

	char buff[16];

	if ( 1 )
	{
		// format_to buffer tests
		auto numChars = ufmt::format_to0( buff, "{}", "0123456789abcdefghij" );


	}

	if ( 0 )
	{
		TestPerformance( "{:.3f}", 3.141592653458 );
		TestPerformance( "{} {} {}", 1, 1.0f, 2.0 );
		TestPerformance( "Some {} with some {}: {} {} {}", "text", "values", 123456789ull, 999999999999.0, 0.0f );
	}

	return 0;
}

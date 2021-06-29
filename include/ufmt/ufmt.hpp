#pragma once

#include <string.h>

#if !defined(UFMT_NOEXCEPT)
	#define UFMT_NOEXCEPT noexcept
#endif

#if !defined(UFMT_SPRINTF)
	#if defined(_MSC_VER)
		#define UFMT_SPRINTF sprintf_s
	#else
		#define UFMT_SPRINTF sprintf
	#endif
#endif

#if !defined(UFMT_DO_NOT_USE_STL)

#include <string>
#include <string_view>

namespace ufmt {

using string_t = std::string;
using string_view_t = std::string_view;

} // namespace ufmt
#endif

/* Forward declarations */
namespace ufmt { struct format_desc; }
namespace ufmt::detail { struct wrapper; }

namespace ufmt {

struct format_desc
{
	string_view_t format;
	int base = 10;
	int precision = 0;
	size_t width = 0;
	bool prefix = false;
	bool floating_point = false;
	char sign = '-';
	char fill = ' ';
	char type = 0;

	enum class alignment
	{
		none,
		left,
		right,
		center
	};

	alignment align = alignment::none;

	static format_desc parse(
	    string_view_t argFormat,
	    const detail::wrapper *const args,
	    size_t numArgs,
	    size_t argIndex );
};

template <typename T> struct formatter
{
	static string_t to_string( const void *arg, const format_desc &fd )
	{
		return "?";
	}

	static int to_int( const void *arg ) { return 0; }
};

template <typename T> struct formatter<T &>
{
	static string_t to_string( const void *arg, const format_desc &fd ) { return formatter<T>::to_string( arg, fd ); }
	static int to_int( const void *arg ) { return formatter<T>::to_int( arg ); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail {

static constexpr size_t StackBufferLength = 80;

inline bool is_digit( char ch ) { return ch >= '0' && ch <= '9'; }
inline bool is_upper( char ch ) { return ch >= 'A' && ch <= 'Z'; }

using formatter_func = string_t( * )( const void *arg, const format_desc &fd );

struct wrapper
{
	const void *ptr = nullptr;
	formatter_func func = nullptr;
};

string_t format_wrapped_args( const char *f, const detail::wrapper *const args, size_t numArgs );

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename... Args>
string_t format( const char *f, Args &&... args )
{
	if constexpr ( sizeof...( Args ) != 0 )
	{
		const detail::wrapper wrappedArgs[] = { { &args, formatter<Args>::to_string }... };
		return detail::format_wrapped_args( f, wrappedArgs, sizeof...( Args ) );
	}

	return detail::format_wrapped_args( f, nullptr, 0 );
}

} // namespace ufmt

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ufmt::detail {

enum class numeric_type
{
	signed_integer,
	unsigned_integer,
	floating_point
};

template <typename T, numeric_type Type> struct numeric_formatter
{
	static string_t to_string( const void *arg, const format_desc &fd )
	{
		auto value = *reinterpret_cast<const T *>( arg );

		if constexpr ( Type == numeric_type::floating_point )
		{
			if ( fd.type && strchr( "bBdoxXn", fd.type ) )
			{
				auto valueInt64 = int64_t( value );
				return numeric_formatter<int64_t, numeric_type::signed_integer>::to_string( &valueInt64, fd );
			}
		}
		else
		{
			if ( fd.type && strchr( "aAeEfF", fd.type ) )
			{
				auto valueDouble = double( value );
				return numeric_formatter<double, numeric_type::floating_point>::to_string( &valueDouble, fd );
			}
		}

		char buff[StackBufferLength] = { };
		char *prefixCursor = buff;

		if ( value > 0 )
		{
			if ( fd.sign == '+' || fd.sign == ' ' )
				*prefixCursor++ = fd.sign;
		}
		else if ( value < 0 && ( fd.base != 10 || Type == numeric_type::floating_point ) )
		{
			if constexpr ( Type != numeric_type::unsigned_integer )
			{
				*prefixCursor++ = '-';
				value = -value;
			}
		}

		char prefixChar = 0;

		if constexpr ( Type == numeric_type::floating_point )
		{

		}
		else
		{
			if ( fd.prefix )
				prefixChar = fd.type;
		}

		if ( prefixChar )
		{
			*prefixCursor++ = '0';
			*prefixCursor++ = fd.type;
		}

		if constexpr ( Type == numeric_type::signed_integer )
		{
			if constexpr ( sizeof( T ) <= 4 )
				_itoa( int( value ), prefixCursor, fd.base );
			else if constexpr ( sizeof( T ) == 8 )
				_i64toa( int64_t( value ), prefixCursor, fd.base );
		}
		else if constexpr ( Type == numeric_type::unsigned_integer )
		{
			if constexpr ( sizeof( T ) <= 4 )
				_ultoa( uint( value ), prefixCursor, fd.base );
			else if constexpr ( sizeof( T ) == 8 )
				_ui64toa( uint64_t( value ), prefixCursor, fd.base );
		}
		else if constexpr ( Type == numeric_type::floating_point )
		{
			char fmtBuff[StackBufferLength] = { '%' };

			if ( fd.precision > 0 )
			{
				fmtBuff[1] = '.';
				fmtBuff[2] = '*';
				fmtBuff[3] = fd.type;
				fmtBuff[4] = 0;

				UFMT_SPRINTF( prefixCursor, detail::StackBufferLength - ( prefixCursor - buff ), fmtBuff, fd.precision, value );
			}
			else
			{
				fmtBuff[1] = fd.type;
				fmtBuff[2] = 0;

				UFMT_SPRINTF( prefixCursor, detail::StackBufferLength - ( prefixCursor - buff ), fmtBuff, value );
			}
		}

		if ( is_upper( fd.type ) )
		{
			auto *c = buff;
			while ( *c )
			{
				*c = toupper( *c );
				++c;
			}
		}

		if ( auto len = strlen( buff ); len < fd.width )
		{
			string_t result = string_t( buff, prefixCursor - buff );

			result += string_t( fd.width - len, '0' );

			result += prefixCursor;
			return result;
		}

		return buff;
	}

	static int to_int( const void *arg )
	{
		return int( *reinterpret_cast<const T *>( arg ) );
	}
};

//---------------------------------------------------------------------------------------------------------------------
inline string_t format_wrapped_args( const char *f, const detail::wrapper *const args, size_t numArgs )
{
	// Early out
	if ( f == nullptr || *f == 0 )
		return string_t();

	string_t result = "";

	char argBuff[StackBufferLength];
	size_t argLength = 0;
	size_t argIndex = 0;

	bool parsingFormat = false;
	char prevCh = 0;

	while ( *f )
	{
		auto ch = *f++;
		bool append = !parsingFormat;

		if ( parsingFormat )
		{
			if ( ch == '}' )
			{
				parsingFormat = false;
				prevCh = 0;
				append = false;
				argBuff[argLength] = 0;

				auto *argFormat = argBuff;

				if ( argLength )
				{
					for ( size_t i = 0; i < argLength && ( *argFormat ) != ':'; ++i )
						++argFormat;

					if ( argFormat < argBuff + argLength )
					{
						*argFormat = 0;
						++argFormat;
					}

					if ( detail::is_digit( argBuff[0] ) )
						argIndex = atoi( argBuff );
				}

				if ( argIndex < numArgs )
				{
					format_desc fd = format_desc::parse( argFormat, args, numArgs, argIndex );
					auto argStr = args[argIndex].func( args[argIndex].ptr, fd );

					if ( argStr.size() < fd.width )
					{
						if ( fd.align == format_desc::alignment::left || fd.align == format_desc::alignment::right )
						{
							string_t padding = string_t( fd.width - argStr.size(), fd.fill );

							if ( fd.align == format_desc::alignment::left )
								argStr += padding;
							else
								argStr = padding + argStr;
						}
						else if ( fd.align == format_desc::alignment::center )
						{
							string_t padding = string_t( ( fd.width - argStr.size() ) / 2, fd.fill );

							argStr = padding + argStr + padding;

							if ( argStr.size() < fd.width )
								argStr += string_t( &fd.fill, 1 );
						}
					}

					result += argStr;
				}

				++argIndex;
			}
			else if ( ch == '{' )
			{
				parsingFormat = false;
				prevCh = 0;
				append = true;
			}
			else
			{
				if ( argLength < StackBufferLength - 1 )
					argBuff[argLength++] = ch;
			}
		}
		else
		{
			if ( ch == '{' )
			{
				parsingFormat = true;
				prevCh = 0;
				append = false;

				argLength = 0;
			}
			else if ( ch == '}' )
				append = ( prevCh == '}' );
		}

		if ( append )
			result += string_t( &ch, 1 );

		prevCh = ch;
	}

	return result;
}

} // namespace ufmt::detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ufmt {

template <> struct formatter<int16_t> : detail::numeric_formatter<int16_t, detail::numeric_type::signed_integer> { };
template <> struct formatter<int32_t> : detail::numeric_formatter<int32_t, detail::numeric_type::signed_integer> { };
template <> struct formatter<int64_t> : detail::numeric_formatter<int64_t, detail::numeric_type::signed_integer> { };

template <> struct formatter<uint8_t > : detail::numeric_formatter<uint8_t,  detail::numeric_type::unsigned_integer> { };
template <> struct formatter<uint16_t> : detail::numeric_formatter<uint16_t, detail::numeric_type::unsigned_integer> { };
template <> struct formatter<uint32_t> : detail::numeric_formatter<uint32_t, detail::numeric_type::unsigned_integer> { };
template <> struct formatter<uint64_t> : detail::numeric_formatter<uint64_t, detail::numeric_type::unsigned_integer> { };

template <> struct formatter<double> : detail::numeric_formatter<double, detail::numeric_type::floating_point> { };

template <> struct formatter<bool>
{
	static string_t to_string( const void *arg, const format_desc &fd )
	{
		auto value = *reinterpret_cast<const bool *>( arg );
		return value ? "true" : "false";
	}

	static int to_int( const void *arg ) { return 0; }
};

template <> struct formatter<float>
{
	static string_t to_string( const void *arg, const format_desc &fd )
	{
		double value = *reinterpret_cast<const float *>( arg );
		return formatter<double>::to_string( &value, fd );
	}

	static int to_int( const void *arg ) { return 0; }
};

template <> struct formatter<char>
{
	static string_t to_string( const void *arg, const format_desc &fd )
	{
		char value = *reinterpret_cast<const char *>( arg );

		if ( fd.type && strchr( "bBdnoxX", fd.type ) )
		{
			return detail::numeric_formatter<char, detail::numeric_type::signed_integer>::to_string( &value, fd );
		}

		return string_t( &value, 1 );
	}

	static int to_int( const void *arg ) { return 0; }
};

template <> struct formatter<const char *>
{
	static string_t to_string( const void *arg, const format_desc &fd )
	{
		const char *value = *reinterpret_cast<const char *const *>( arg );
		return value ? value : "nullptr";
	}

	static int to_int( const void *arg ) { return 0; }
};

template <size_t N> struct formatter<const char ( & )[N]>
{
	static string_t to_string( const void *arg, const format_desc &fd )
	{
		const char *value = *reinterpret_cast<const char *( & )[N]>( arg );
		return value;
	}

	static int to_int( const void *arg ) { return 0; }
};

template <> struct formatter<string_t>
{
	static string_t to_string( const void *arg, const format_desc &fd )
	{
		return *reinterpret_cast<const string_t *>( arg );
	}

	static int to_int( const void *arg ) { return 0; }
};

template <> struct formatter<string_view_t>
{
	static string_t to_string( const void *arg, const format_desc &fd )
	{
		return string_t( *reinterpret_cast<const string_view_t *>( arg ) );
	}

	static int to_int( const void *arg ) { return 0; }
};

template <typename T> struct formatter<T *>
{
	static string_t to_string( const void *arg, const format_desc &fd )
	{
		format_desc fdPtr = fd;
		if ( fdPtr.type == 0 || fdPtr.type == 'p' )
		{
			fdPtr.prefix = true;
			fdPtr.type = 'x';
			fdPtr.base = 16;
		}
		else if ( fdPtr.type == 'P' )
		{
			fdPtr.prefix = true;
			fdPtr.type = 'X';
			fdPtr.base = 16;
		}

		auto value = *reinterpret_cast<const size_t *>( arg );
		return detail::numeric_formatter<size_t, detail::numeric_type::unsigned_integer>::to_string( &value, fdPtr );
	}

	static int to_int( const void *arg ) { return 0; }
};

//---------------------------------------------------------------------------------------------------------------------
inline format_desc format_desc::parse(
    string_view_t argFormat,
    const detail::wrapper *const args,
    size_t numArgs,
    size_t argIndex )
{
	format_desc result;
	result.format = argFormat;

	size_t numCharsLeft = argFormat.size();
	if ( !numCharsLeft )
		return result;

	const auto *chars = argFormat.data();

	// Fill character
	if ( numCharsLeft >= 2 && ( *chars ) != '{' && ( *chars ) != '}' && strchr( "<>=^", chars[1] ) )
	{
		result.fill = *chars++;
		--numCharsLeft;
	}

	// Alignment
	if ( auto *alignChar = strchr( "<>^=", *chars ); numCharsLeft && alignChar )
	{
		if ( *alignChar == '<' )
			result.align = alignment::left;
		else if ( *alignChar == '>' )
			result.align = alignment::right;
		else if ( *alignChar == '^' )
			result.align = alignment::center;

		++chars;
		--numCharsLeft;
	}

	// Sign
	if ( auto *signChar = strchr( "+- ", *chars ); numCharsLeft && signChar )
	{
		result.sign = *chars++;
		--numCharsLeft;
	}

	// Type prefix
	if ( numCharsLeft && *chars == '#' )
	{
		result.prefix = true;
		++chars;
		--numCharsLeft;
	}

	// Sign-aware zero-padding for numeric types
	if ( numCharsLeft && *chars == '0' )
	{
		result.fill = '0';

		++chars;
		--numCharsLeft;
	}

	// Alignment width
	if ( numCharsLeft && detail::is_digit( *chars ) )
	{
		char buff[detail::StackBufferLength] = { };
		size_t buffLength = 0;

		while ( numCharsLeft && detail::is_digit( *chars ) && buffLength < detail::StackBufferLength - 1 )
		{
			buff[buffLength++] = *chars++;
			--numCharsLeft;
		}

		if ( auto widthI = atoi( buff ); widthI >= 0 )
			result.width = size_t( widthI );
	}

	// Precision
	if ( numCharsLeft >= 2 && *chars == '.' && detail::is_digit( chars[1] ) )
	{
		++chars;
		--numCharsLeft;

		char buff[detail::StackBufferLength] = { };
		size_t buffLength = 0;

		while ( numCharsLeft && detail::is_digit( *chars ) && buffLength < detail::StackBufferLength - 1 )
		{
			buff[buffLength++] = *chars++;
			--numCharsLeft;
		}

		if ( ( result.precision = atoi( buff ) ) < 0 )
			result.precision = 0;
	}

	// Type
	if ( numCharsLeft && strchr( "bBdnoxXaAceEfFgGps", *chars ) )
	{
		result.type = *chars;

		if ( result.type == 'b' || result.type == 'B' )
			result.base = 2;
		else if ( result.type == 'o' )
			result.base = 8;
		else if ( result.type == 'x' || result.type == 'X' )
			result.base = 16;

		++chars;
		--numCharsLeft;
	}

	return result;
}

} // namespace ufmt

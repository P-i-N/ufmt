#pragma once

#include <string.h>

/* Forward declarations */
namespace ufmt::detail { struct wrapper; }

#include "ufmt_writer.hpp"

namespace ufmt {

struct format_desc
{
	int base = 10;
	int precision = 0;
	size_t width = 0;
	bool prefix = false;
	bool floating_point = false;
	int sign = '-';
	int fill = ' ';
	int type = 0;

	enum class alignment
	{
		none,
		left,
		right,
		center
	};

	alignment align = alignment::none;

	template <typename C>
	static format_desc parse(
	    const C *valuePtrFormatStr,
	    const detail::wrapper *const argPtrs,
	    size_t numArgs,
	    size_t valuePtrIndex );
};

template <typename W, typename T> struct formatter
{
	static bool write( void *writerPtr, const void *valuePtr, const format_desc &fd )
	{
		W &w = *reinterpret_cast<W *>( writerPtr );

		const auto &value = *reinterpret_cast<const T *>( valuePtr );
		return w.append( data( value ), length( value ) );
	}
};

template <typename W, typename T> struct formatter<W, T &>
{
	static bool write( void *writerPtr, const void *valuePtr, const format_desc &fd )
	{
		return formatter<W, T>::write( writerPtr, valuePtr, fd );
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail {

using formatter_write_func = bool( * )( void *writerPtr, const void *valuePtr, const format_desc &fd );

struct wrapper
{
	const void *ptr = nullptr;
	formatter_write_func writeFunc = nullptr;
};

template <typename W, typename C>
size_t format_wrapped_args_to(
    W &w,
    const C *formatStr,
    size_t formatStrLen,
    const detail::wrapper *const argPtrs,
    size_t numArgs );

template <bool ZT, typename O, typename C, size_t N, typename... Args>
size_t format_to_( O &output, const C( &formatStr )[N], Args &&... argPtrs )
{
	detail::writer<O> w = { output };
	const detail::wrapper wrappedArgs[] { { &argPtrs, formatter<decltype( w ), Args>::write }..., { } };
	auto numChars = detail::format_wrapped_args_to( w, formatStr, N - 1, wrappedArgs, sizeof...( Args ) );
	if constexpr ( ZT ) { w.zero_terminate(); }
	return numChars;
}

template <bool ZT, typename O, typename T, typename... Args>
size_t format_to_( O &output, T formatStr, Args &&... argPtrs )
{
	detail::writer<O> w = { output };
	const detail::wrapper wrappedArgs[] { { &argPtrs, formatter<decltype( w ), Args>::write }..., { } };
	auto numChars = detail::format_wrapped_args_to( w, data( formatStr ), length( formatStr ), wrappedArgs, sizeof...( Args ) );
	if constexpr ( ZT ) { w.zero_terminate(); }
	return numChars;
}

template <bool ZT, typename C, typename T, typename... Args>
size_t format_to_n_( C *output, size_t outputLen, T formatStr, Args &&... argPtrs )
{
	detail::buffer_writer<C> w( output, outputLen );
	const detail::wrapper wrappedArgs[] { { &argPtrs, formatter<decltype( w ), Args>::write }..., { } };
	auto numChars = detail::format_wrapped_args_to( w, data( formatStr ), length( formatStr ), wrappedArgs, sizeof...( Args ) );
	if constexpr ( ZT ) { w.zero_terminate(); }
	return numChars;
}

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename O, typename C, size_t N, typename... Args>
size_t format_to( O &output, const C ( &formatStr )[N], Args &&... argPtrs )
{
	return detail::format_to_<false>( output, formatStr, argPtrs... );
}

template <typename O, typename T, typename... Args>
size_t format_to( O &output, T formatStr, Args &&... argPtrs )
{
	return detail::format_to_<false>( output, formatStr, argPtrs... );
}

template <typename C, typename T, typename... Args>
size_t format_to_n( C *output, size_t outputLen, T formatStr, Args &&... argPtrs )
{
	return detail::format_to_n_<false>( output, outputLen, formatStr, argPtrs... );
}

template <typename O, typename C, size_t N, typename... Args>
size_t format_to0( O &output, const C( &formatStr )[N], Args &&... argPtrs )
{
	return detail::format_to_<true>( output, formatStr, argPtrs... );
}

template <typename O, typename T, typename... Args>
size_t format_to0( O &output, T formatStr, Args &&... argPtrs )
{
	return detail::format_to_<true>( output, formatStr, argPtrs... );
}

template <typename C, typename T, typename... Args>
size_t format_to_n0( C *output, size_t outputLen, T formatStr, Args &&... argPtrs )
{
	return detail::format_to_n_<true>( output, outputLen, formatStr, argPtrs... );
}

#if !defined(UFMT_DO_NOT_USE_STL)
template <typename C, typename... Args>
std::basic_string<C> format( std::basic_string_view<C> formatStr, Args &&... argPtrs )
{
	std::basic_string<C> result;
	format_to( result, formatStr, argPtrs... );
	return result;
}

template <typename... Args>
std::string format( std::string_view formatStr, Args &&... argPtrs )
{
	return format<char>( formatStr, argPtrs... );
}

template <typename... Args>
std::wstring format( std::wstring_view formatStr, Args &&... argPtrs )
{
	return format<wchar_t>( formatStr, argPtrs... );
}
#endif

} // namespace ufmt

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ufmt::detail {

enum class numeric_type
{
	signed_integer,
	unsigned_integer,
	floating_point
};

template <typename W, typename T, numeric_type Type> struct numeric_formatter
{
	static bool write( void *writerPtr, const void *valuePtr, const format_desc &fd )
	{
		W &w = *reinterpret_cast<W *>( writerPtr );

		auto value = *reinterpret_cast<const T *>( valuePtr );

		if constexpr ( Type == numeric_type::floating_point )
		{
			if ( fd.type && detail::find_char( "bBdoxXn", fd.type ) )
			{
				auto valueInt64 = int64_t( value );
				return numeric_formatter<W, int64_t, numeric_type::signed_integer>::write( writerPtr, &valueInt64, fd );
			}
		}
		else
		{
			if ( fd.type && detail::find_char( "aAeEfF", fd.type ) )
			{
				auto valueDouble = double( value );
				return numeric_formatter<W, double, numeric_type::floating_point>::write( writerPtr, &valueDouble, fd );
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
			// TBD
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
#if defined(_MSC_VER)
				_itoa( int( value ), prefixCursor, fd.base );
#else
				snprintf( prefixCursor, detail::StackBufferLength - ( prefixCursor - buff ), "%" PRIi32, int32_t( value ) );
#endif
			else if constexpr ( sizeof( T ) == 8 )
#if defined(_MSC_VER)
				_i64toa( int64_t( value ), prefixCursor, fd.base );
#else
				snprintf( prefixCursor, detail::StackBufferLength - ( prefixCursor - buff ), "%" PRIi64, int64_t( value ) );
#endif
		}
		else if constexpr ( Type == numeric_type::unsigned_integer )
		{
			if constexpr ( sizeof( T ) <= 4 )
#if defined(_MSC_VER)
				_ultoa( uint( value ), prefixCursor, fd.base );
#else
				snprintf( prefixCursor, detail::StackBufferLength - ( prefixCursor - buff ), "%" PRIu32, uint32_t( value ) );
#endif
			else if constexpr ( sizeof( T ) == 8 )
#if defined(_MSC_VER)
				_ui64toa( uint64_t( value ), prefixCursor, fd.base );
#else
				snprintf( prefixCursor, detail::StackBufferLength - ( prefixCursor - buff ), "%" PRIu64, uint64_t( value ) );
#endif
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

				snprintf( prefixCursor, detail::StackBufferLength - ( prefixCursor - buff ), fmtBuff, fd.precision, value );
			}
			else
			{
				fmtBuff[1] = fd.type;
				fmtBuff[2] = 0;

				snprintf( prefixCursor, detail::StackBufferLength - ( prefixCursor - buff ), fmtBuff, value );
			}

			if ( ( fd.type == 'a' || fd.type == 'A' ) && !fd.prefix )
			{
				auto *c = prefixCursor;
				while ( c[1] )
				{
					*c = c[2];
					++c;
				}
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

		if ( auto len = length( buff ); len < fd.width )
		{
			w.append( buff, prefixCursor - buff );
			w.append( "0", 1, fd.width - len );
			return w.append( prefixCursor );
		}

		return w.append( buff );
	}
};

//---------------------------------------------------------------------------------------------------------------------
template <typename W, typename C>
inline size_t format_wrapped_args_to(
    W &w,
    const C *formatStr,
    size_t formatStrLen,
    const detail::wrapper *const argPtrs,
    size_t numArgs )
{
	// Early out
	if ( formatStr == nullptr || *formatStr == 0 )
		return 0;

	C valuePtrBuff[StackBufferLength] = { };
	size_t valuePtrLength = 0;
	size_t valuePtrIndex = 0;

	bool parsingFormat = false;
	C prevCh = 0;

	while ( formatStrLen-- )
	{
		auto ch = *formatStr++;
		bool append = !parsingFormat;

		if ( parsingFormat )
		{
			if ( ch == C( '}' ) )
			{
				parsingFormat = false;
				prevCh = 0;
				append = false;
				valuePtrBuff[valuePtrLength] = 0;

				auto *valuePtrFormat = valuePtrBuff;

				if ( valuePtrLength )
				{
					for ( size_t i = 0; i < valuePtrLength && ( *valuePtrFormat ) != C( ':' ); ++i )
						++valuePtrFormat;

					if ( valuePtrFormat < valuePtrBuff + valuePtrLength )
					{
						*valuePtrFormat = 0;
						++valuePtrFormat;
					}

					if ( detail::is_digit( valuePtrBuff[0] ) )
						valuePtrIndex = detail::string_to_int( valuePtrBuff );
				}

				if ( valuePtrIndex < numArgs )
				{
					format_desc fd = format_desc::parse( valuePtrFormat, argPtrs, numArgs, valuePtrIndex );

					auto prevLen = w.length();
					argPtrs[valuePtrIndex].writeFunc( &w, argPtrs[valuePtrIndex].ptr, fd );

					if ( auto len = w.length() - prevLen; len < fd.width )
					{
						auto padLen = fd.width - len;

						if ( fd.align == format_desc::alignment::left )
							w.append( &fd.fill, 1, padLen );
						else if ( fd.align == format_desc::alignment::right )
							w.insert( prevLen, &fd.fill, 1, padLen );
						else if ( fd.align == format_desc::alignment::center )
						{
							padLen /= 2;

							w.insert( prevLen, &fd.fill, 1, padLen );
							w.append( &fd.fill, 1, padLen );

							if ( len + 2 * padLen < fd.width )
								w.append( &fd.fill, 1 );
						}
					}
				}

				++valuePtrIndex;
			}
			else if ( ch == C( '{' ) )
			{
				parsingFormat = false;
				prevCh = 0;
				append = true;
			}
			else
			{
				if ( valuePtrLength < StackBufferLength - 1 )
					valuePtrBuff[valuePtrLength++] = ch;
			}
		}
		else
		{
			if ( ch == C( '{' ) )
			{
				parsingFormat = true;
				prevCh = 0;
				append = false;

				valuePtrLength = 0;
			}
			else if ( ch == C( '}' ) )
				append = ( prevCh == C( '}' ) );
		}

		if ( append )
			w.append( &ch, 1 );

		prevCh = ch;
	}

	return w.length();
}

} // namespace ufmt::detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ufmt {

template <typename W> struct formatter<W, int16_t> : detail::numeric_formatter<W, int16_t, detail::numeric_type::signed_integer> { };
template <typename W> struct formatter<W, int32_t> : detail::numeric_formatter<W, int32_t, detail::numeric_type::signed_integer> { };
template <typename W> struct formatter<W, int64_t> : detail::numeric_formatter<W, int64_t, detail::numeric_type::signed_integer> { };

template <typename W> struct formatter<W, uint8_t > : detail::numeric_formatter<W, uint8_t,  detail::numeric_type::unsigned_integer> { };
template <typename W> struct formatter<W, uint16_t> : detail::numeric_formatter<W, uint16_t, detail::numeric_type::unsigned_integer> { };
template <typename W> struct formatter<W, uint32_t> : detail::numeric_formatter<W, uint32_t, detail::numeric_type::unsigned_integer> { };
template <typename W> struct formatter<W, uint64_t> : detail::numeric_formatter<W, uint64_t, detail::numeric_type::unsigned_integer> { };

template <typename W> struct formatter<W, double> : detail::numeric_formatter<W, double, detail::numeric_type::floating_point> { };

template <typename W> struct formatter<W, bool>
{
	static bool write( void *writerPtr, const void *valuePtr, const format_desc &fd )
	{
		W &w = *reinterpret_cast<W *>( writerPtr );
		auto value = *reinterpret_cast<const bool *>( valuePtr );

		return w.append( value ? "true" : "false" );
	}
};

template <typename W> struct formatter<W, float>
{
	static bool write( void *writerPtr, const void *valuePtr, const format_desc &fd )
	{
		double value = *reinterpret_cast<const float *>( valuePtr );
		return formatter<W, double>::write( writerPtr, &value, fd );
	}
};

template <typename W> struct formatter<W, char>
{
	static bool write( void *writerPtr, const void *valuePtr, const format_desc &fd )
	{
		char value = *reinterpret_cast<const char *>( valuePtr );

		if ( fd.type && detail::find_char( "bBdnoxX", fd.type ) )
		{
			return detail::numeric_formatter<W, char, detail::numeric_type::signed_integer>::write( writerPtr, &value, fd );
		}

		W &w = *reinterpret_cast<W *>( writerPtr );
		return w.append( &value, 1 );
	}
};

template <typename W> struct formatter<W, const char *>
{
	static bool write( void *writerPtr, const void *valuePtr, const format_desc &fd )
	{
		const char *value = *reinterpret_cast<const char *const *>( valuePtr );

		W &w = *reinterpret_cast<W *>( writerPtr );
		return w.append( value ? value : "nullptr" );
	}
};

template <typename W, size_t N> struct formatter<W, const char ( & )[N]>
{
	static bool write( void *writerPtr, const void *valuePtr, const format_desc &fd )
	{
		W &w = *reinterpret_cast<W *>( writerPtr );
		return w.append( *reinterpret_cast<const char *( & )[N]>( valuePtr ), N - 1 );
	}
};

template <typename W, typename T> struct formatter<W, T *>
{
	static bool write( void *writerPtr, const void *valuePtr, const format_desc &fd )
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

		auto value = *reinterpret_cast<const size_t *>( valuePtr );
		return detail::numeric_formatter<W, size_t, detail::numeric_type::unsigned_integer>::write( writerPtr, &value, fdPtr );
	}
};

//---------------------------------------------------------------------------------------------------------------------
template <typename C>
inline format_desc format_desc::parse(
    const C *valuePtrFormatStr,
    const detail::wrapper *const argPtrs,
    size_t numArgs,
    size_t valuePtrIndex )
{
	format_desc result;

	size_t numCharsLeft = length( valuePtrFormatStr );
	if ( !numCharsLeft )
		return result;

	const auto *chars = valuePtrFormatStr;

	// Fill character
	if ( numCharsLeft >= 2 && ( *chars ) != '{' && ( *chars ) != '}' && detail::find_char( "<>=^", chars[1] ) )
	{
		result.fill = *chars++;
		--numCharsLeft;
	}

	// Alignment
	if ( auto alignChar = detail::find_char( "<>^=", *chars ); numCharsLeft && alignChar )
	{
		if ( alignChar == '<' )
			result.align = alignment::left;
		else if ( alignChar == '>' )
			result.align = alignment::right;
		else if ( alignChar == '^' )
			result.align = alignment::center;

		++chars;
		--numCharsLeft;
	}

	// Sign
	if ( numCharsLeft && detail::find_char( "+- ", *chars ) )
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
		result.width = detail::string_to_uint( chars, numCharsLeft );

	// Precision
	if ( numCharsLeft >= 2 && *chars == '.' && detail::is_digit( chars[1] ) )
	{
		++chars;
		--numCharsLeft;

		result.precision = detail::string_to_uint( chars, numCharsLeft );
	}

	// Type
	if ( numCharsLeft && detail::find_char( "bBdnoxXaAceEfFgGps", *chars ) )
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

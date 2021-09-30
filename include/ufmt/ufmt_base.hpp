#pragma once

#if !defined(UFMT_NOEXCEPT)
	#define UFMT_NOEXCEPT noexcept
#endif

#if !defined(UFMT_DO_NOT_USE_STL)
	#include <string>
	#include <string_view>
#endif

namespace ufmt {

//---------------------------------------------------------------------------------------------------------------------
template <typename C> size_t length( const C *str )
{
	size_t result = 0;

	if ( str )
		while ( *str++ )
			++result;

	return result;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename C> bool length( const C *str, size_t &result )
{
	if ( result == size_t( -1 ) )
		result = length( str );

	if ( !str || !result )
		return false;

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename C> const C *data( const C *str ) { return str; }

#if !defined(UFMT_DO_NOT_USE_STL)
template <typename C>
inline size_t length( const std::basic_string<C> &str ) { return str.size(); }

template <typename C>
inline size_t length( const std::basic_string_view<C> &strView ) { return strView.size(); }

template <typename C>
inline const C *data( const std::basic_string<C> &str ) { return str.data(); }

template <typename C>
inline const C *data( const std::basic_string_view<C> &strView ) { return strView.data(); }

template <typename C, typename T>
inline void append( std::basic_string<C> &str, const T &strToAppend ) { str += strToAppend; }

template <typename C, typename T>
inline void append( std::basic_string<C> &str, const T *strToAppend, size_t len )
{
	str.append( strToAppend, len );
}

template <typename C, typename T>
inline void insert( std::basic_string<C> &str, size_t pos, const T &strToInsert ) { str.insert( pos, strToInsert ); }

template <typename C, typename T>
inline std::basic_string<C> convert( const T *str, size_t len )
{
	if constexpr ( sizeof( C ) == sizeof( T ) )
		return std::basic_string<C>( reinterpret_cast<const C *>( str ), len );

	std::basic_string<C> result( len, 0 );

	while ( len-- )
		result[len] = C( str[len] );

	return result;
}
#endif

} // namespace ufmt

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ufmt::detail {

static constexpr size_t StackBufferLength = 80;

template <typename C> inline bool is_digit( C ch ) UFMT_NOEXCEPT { return unsigned( ch - C( '0' ) ) <= 9; }
template <typename C> inline bool is_upper( C ch ) UFMT_NOEXCEPT { return ch >= C( 'A' ) && ch <= C( 'Z' ); }
template <typename C> inline bool is_space( C ch ) UFMT_NOEXCEPT { return ch > 0 && ch <= 32; }

//---------------------------------------------------------------------------------------------------------------------
template <typename C> inline C find_char( const char *chars, C ch ) UFMT_NOEXCEPT
{
	while ( *chars )
		if ( auto c = *chars++; C( c ) == ch )
			return ch;

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename C>
inline int string_to_int( const C *str ) UFMT_NOEXCEPT
{
	if ( !str )
		return 0;

	while ( is_space( *str ) )
		++str;

	int sign = 1, result = 0;

	if ( *str == '-' )
	{
		sign = -1;
		++str;
	}

	unsigned digit;
	while ( ( digit = ( unsigned( ( *str++ ) - '0' ) ) ) <= 9 )
		result = result * 10 + int( digit );

	return result * sign;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename C>
inline unsigned string_to_uint( const C *&str, size_t &numCharsLeft ) UFMT_NOEXCEPT
{
	if ( !str )
		return 0;

	while ( numCharsLeft && is_space( *str ) )
	{
		++str;
		--numCharsLeft;
	}

	unsigned digit, result = 0;
	while ( numCharsLeft-- && ( ( digit = ( unsigned( ( *str++ ) - '0' ) ) ) <= 9 ) )
		result = result * 10 + digit;

	++numCharsLeft;
	--str;

	return result;
}

} // namespace ufmt::detail

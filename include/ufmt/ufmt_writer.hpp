#pragma once

#include "ufmt_base.hpp"

namespace ufmt::detail {

template <typename T>
struct buffer_writer
{
	T *begin = nullptr;

	T *end = begin;

	T *cursor = begin;

	size_t length() const noexcept { return size_t( cursor - begin ); }

	size_t remaining() const noexcept { return size_t( ( cursor < end ) ? ( end - cursor ) : 0 ); }

	bool remaining( size_t numBytes ) const noexcept { return cursor + numBytes <= end; }

	template <typename U>
	bool append( const U *str, size_t len = size_t( -1 ), size_t repeat = 1 )
	{
		if ( !ufmt::length( str, len ) )
			return true;

		if ( !remaining( len * repeat ) )
		{
			auto *c = cursor;
			cursor += len * repeat;

			while ( repeat-- )
			{
				auto *in = str;
				auto l = len;

				while ( ( c < end ) && l-- )
					*c++ = T( *in++ );
			}

			return false;
		}

		while ( repeat-- )
		{
			auto *in = str;
			auto l = len;

			while ( l-- )
				*cursor++ = T( *in++ );
		}

		return true;
	}

	template <typename U>
	bool insert( size_t pos, const U *str, size_t len = size_t( -1 ), size_t repeat = 1 )
	{
		if ( !ufmt::length( str, len ) )
			return true;

		bool result = true;

		auto charsToCopy = len * repeat;
		if ( auto rem = remaining(); charsToCopy > rem )
		{
			charsToCopy = rem;
			result = false;
		}

		for ( size_t i = charsToCopy, j = len * repeat - 1, k = 1; i--; --j, ++k )
			*( cursor + j ) = *( cursor - k );

		cursor += len * repeat;

		while ( repeat-- )
		{
			auto *in = str;
			auto l = len;

			while ( l-- )
				begin[pos++] = T( *in++ );
		}

		return result;
	}

	void zero_terminate()
	{
		if ( begin < end )
		{
			if ( cursor < end )
				( *cursor ) = 0;
			else
				end[-1] = 0;
		}
	}

	buffer_writer( T *buffer, size_t len )
		: begin( buffer )
		, end( buffer ? ( buffer + len ) : nullptr )
		, cursor( begin )
	{

	}
};

template <typename T, typename C>
struct string_writer
{
	T &output;

	size_t length() const noexcept { return ufmt::length( output ); }
	size_t remaining() const noexcept { return size_t( -1 ); }
	bool remaining( size_t numBytes ) const noexcept { return true; }

	template <typename U>
	bool append( const U *str, size_t len = size_t( -1 ), size_t repeat = 1 )
	{
		if ( !ufmt::length( str, len ) )
			return true;

		if constexpr ( sizeof( U ) == sizeof( C ) )
		{
			while ( repeat-- )
				ufmt::append( output, reinterpret_cast<const C *>( str ), len );
		}
		else
		{
			auto convertedStr = ufmt::convert<C>( str, len );

			while ( repeat-- )
				ufmt::append( output, convertedStr );
		}

		return true;
	}

	template <typename U>
	bool insert( size_t pos, const U *str, size_t len = size_t( -1 ), size_t repeat = 1 )
	{
		if ( !ufmt::length( str, len ) )
			return true;

		auto convertedStr = ufmt::convert<C>( str, len );

		while ( repeat-- )
			ufmt::insert( output, pos, convertedStr );

		return true;
	}

	void zero_terminate()
	{

	}

	string_writer( T &str )
		: output( str )
	{

	}
};

template <typename T> struct writer { };

template <typename T, size_t N>
struct writer<T[N]> : buffer_writer<T>
{
	writer( T *buffer ) : buffer_writer<T>( buffer, N ) { }
};

#if !defined(UFMT_DO_NOT_USE_STL)
template <typename C> struct writer<std::basic_string<C>> : string_writer<std::basic_string<C>, C>
{
	writer( std::basic_string<C> &str ) : string_writer<std::basic_string<C>, C>( str ) { }
};
#endif

} // namespace ufmt::detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

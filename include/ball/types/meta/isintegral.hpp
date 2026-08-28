#ifndef _INCLUDE_BALL_TYPES_META_ISINTEGRAL_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISINTEGRAL_HPP_

#	pragma once

#	include "removecv.hpp"

template < typename T > struct MIsIntegral              { static constexpr bool VALUE = false; };
template <> struct MIsIntegral< bool >                  { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< char >                  { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< signed char >           { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< unsigned char >         { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< wchar_t >               { static constexpr bool VALUE = true; };
#	if defined( __cpp_char8_t )
template <> struct MIsIntegral< char8_t >               { static constexpr bool VALUE = true; };
#	endif
template <> struct MIsIntegral< char16_t >              { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< char32_t >              { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< short >                 { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< unsigned short >        { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< int >                   { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< unsigned int >          { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< long >                  { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< unsigned long >         { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< long long >             { static constexpr bool VALUE = true; };
template <> struct MIsIntegral< unsigned long long >    { static constexpr bool VALUE = true; };

template < typename T >
inline constexpr bool IS_INTEGRAL = MIsIntegral< RemoveCV_t< T > >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISINTEGRAL_HPP_ )

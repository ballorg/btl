#ifndef _INCLUDE_BALL_TYPES_META_GET_VECTOR_HPP_
#	define _INCLUDE_BALL_TYPES_META_GET_VECTOR_HPP_

#	pragma once

template < class V, auto N > using GrowableViewBy_t = typename V::template GrowableView_t< N >;
template < class V, auto N > using ConstGrowableViewBy_t = typename V::template ConstGrowableView_t< N >;

template < typename T, class V > constexpr decltype( auto ) GetBase( V &view ) noexcept { return Get< T >( view ); }
template < typename T, class V > constexpr decltype( auto ) GetBase( const V &view ) noexcept { return Get< T >( view ); }
template < auto K, class V > constexpr decltype( auto ) GetBaseBy( V &view ) noexcept { return view.template BaseBy< K >(); }
template < typename T, class V, typename I > constexpr decltype( auto ) GetElement( V &view, I i ) noexcept { return view.template Get< T >( i ); }
template < typename T, class V, typename I > constexpr decltype( auto ) GetElement( const V &view, I i ) noexcept { return view.template Get< T >( i ); }
template < typename T, class V > constexpr decltype( auto ) GetData( V &view ) noexcept { return view.template DataBy< T >(); }
template < typename T, class V > constexpr decltype( auto ) GetData( const V &view ) noexcept { return view.template DataBy< T >(); }
template < typename T, class V > constexpr decltype( auto ) GetFixedData( V &view ) noexcept { return view.template FixedDataBy< T >(); }
template < typename T, class V > constexpr decltype( auto ) GetFixedData( const V &view ) noexcept { return view.template FixedDataBy< T >(); }
template < typename T, class V, typename I > constexpr decltype( auto ) GetStorage( V &view, I nCount ) noexcept { return view.template StorageBy< T >( nCount ); }
template < typename T, class V, typename I, typename P > constexpr decltype( auto ) GetStorage( V &view, I nCount, P pData ) noexcept { return view.template StorageBy< T >( nCount, pData ); }
template < typename T, class V > constexpr bool IsColumnOverflow( const V &view ) noexcept { return view.template IsOverflowBy< T >( view.Count() ); }
template < typename T, class V, typename I > constexpr decltype( auto ) FindBy( const V &view, const T &value, I iFrom ) noexcept { return view.template FindBy< T >( value, iFrom ); }
template < typename T, class V, typename I > constexpr decltype( auto ) RFindBy( const V &view, const T &value, I iFrom ) noexcept { return view.template RFindBy< T >( value, iFrom ); }
template < typename... Ts, class V, typename... Us > constexpr void StoreFirst( V &view, Us &&...args ) noexcept { view.template StoreFirstElement< 0, Ts... >( Forward< Us >( args )... ); }
template < typename T, class V, typename I > constexpr void ShiftStorageLeft( V &view, I i, I nRemove, I nCount ) noexcept { view.template ShiftLeftBaseBy< T >( i, nRemove, nCount ); }
template < typename T, class V, typename I > constexpr void ShiftStorageRight( V &view, I i, I nAdd, I nCount ) noexcept { view.template ShiftRightBaseBy< T >( i, nAdd, nCount ); }

template < typename T, class V > constexpr decltype( auto ) Packed_Get( V &view ) noexcept { return view.template Packed_Base< T >(); }
template < typename T, class V > constexpr decltype( auto ) Packed_Get( const V &view ) noexcept { return view.template Packed_Base< T >(); }
template < typename T, class V > constexpr decltype( auto ) Packed_GetData( V &view ) noexcept { return view.template Packed_DataBy< T >(); }
template < typename T, class V > constexpr decltype( auto ) Packed_GetFixedData( V &view ) noexcept { return view.template Packed_FixedDataBy< T >(); }
template < typename T, class V > constexpr bool Packed_IsColumnOverflow( const V &view ) noexcept { return view.template Packed_IsOverflowBy< T >(); }
template < typename T, class V, typename I > constexpr size_t Packed_GetSize( const V &, I nCount ) noexcept { return V::template Packed_Size< T >( nCount ); }
template < typename T, class V, typename I > constexpr decltype( auto ) Packed_Get( const V &view, I i ) noexcept { return view.template Get< T >( i ); }
template < typename T, class V, typename I > constexpr void Packed_ShiftLeft( V &view, I i, I nRows, I nShift ) noexcept { view.template Packed_ShiftRowsLeft< T >( i, nRows, nShift ); }
template < typename T, class V, typename I > constexpr void Packed_ShiftRight( V &view, I i, I nRows, I nShift ) noexcept { view.template Packed_ShiftRowsRight< T >( i, nRows, nShift ); }
template < typename T, class V, typename I > constexpr void Packed_Clear( V &view, I i, I nRows ) noexcept { view.template Packed_ClearRowsBy< T >( i, nRows ); }
template < typename T, class V, typename I, typename U > constexpr void Packed_Set( V &view, I i, U &&value ) noexcept { view.template Packed_SetBy< T >( i, Forward< U >( value ) ); }

#endif // !defined( _INCLUDE_BALL_TYPES_META_GET_VECTOR_HPP_ )

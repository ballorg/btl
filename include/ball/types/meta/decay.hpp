#ifndef _INCLUDE_BALL_META_DECAY_HPP_
#	define _INCLUDE_BALL_META_DECAY_HPP_

#	pragma once

#	include "removecv.hpp"
#	include "removereference.hpp"

template < typename T > using Decay_t = RemoveReference_t< RemoveCV_t< T > >;

#endif // !defined( _INCLUDE_BALL_META_DECAY_HPP_ )

#ifndef _INCLUDE_BALL_TYPES_PREFETCH_HPP_
#	define _INCLUDE_BALL_TYPES_PREFETCH_HPP_

#	pragma once

/// @brief Selects whether a cache hint targets a subsequent read or write.
enum class EPrefetchAccess : bool
{
	READ = false, ///< Prefetch for reading.
	WRITE = true ///< Prefetch for writing.
};

#endif // !defined( _INCLUDE_BALL_TYPES_PREFETCH_HPP_ )

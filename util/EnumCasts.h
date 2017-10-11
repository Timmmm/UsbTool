#pragma once

#include <type_traits>

// For converting `enum class` to and from their underlying integral type.
// See http://stackoverflow.com/a/14589519/265521
template<typename E>
constexpr auto to_integral(E e) -> typename std::underlying_type<E>::type 
{
	return static_cast<typename std::underlying_type<E>::type>(e);
}

template<typename E>
constexpr auto from_integral(typename std::underlying_type<E>::type e) -> E
{
	return static_cast<E>(e);
}

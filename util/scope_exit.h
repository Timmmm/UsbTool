#pragma once

#include <type_traits>
#include <utility>

template<typename Callable>
class scope_exit
{
	Callable ExitFunction;
	
public:
	template <typename Fp>
	explicit scope_exit(Fp &&F) : ExitFunction(std::forward<Fp>(F))
	{
	}
	
	scope_exit(scope_exit &&Rhs) : ExitFunction(std::move(Rhs.ExitFunction))
	{
	}
	
	~scope_exit()
	{
		ExitFunction();
	}
};

template <typename Callable>
scope_exit<typename std::decay<Callable>::type> make_scope_exit(Callable&& F)
{
	return scope_exit<typename std::decay<Callable>::type>(std::forward<Callable>(F));
}

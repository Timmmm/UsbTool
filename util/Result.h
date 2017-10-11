#pragma once

#include <string>
#include <iostream>

// This is a C++14 implementation of C++17's std::variant.
// You can replace it with std::variant in a couple of years.
#include <mpark/variant.hpp>

// A Result class - a tagged union that can contain either the successful result
// (T) or an error (E).
template<typename T, typename E>
class Result : private mpark::variant<T, E>
{
public:
	explicit Result(const T& val) : mpark::variant<T, E>(mpark::in_place_index<0>, val) {}
	// The void paramter is needed to distinguish the case when T and E are the same. Ideally we could
	// have used a bool template parameter on the constructor, which compiles, but there's no legal
	// syntax to actually use it.
	Result(const E& err, mpark::monostate) : mpark::variant<T, E>(mpark::in_place_index<1>, err) {}
	
	explicit Result(const T&& val) : mpark::variant<T, E>(mpark::in_place_index<0>, val) {}
	Result(const E&& err, mpark::monostate) : mpark::variant<T, E>(mpark::in_place_index<1>, err) {}
	
	// Assume the result was successful and return the successful value.
	// If this was not the case the program prints an error and aborts. TODO: Throw exception instead?
	T& unwrap()
	{
		return expect("Called unwrap() on an Err Result."); // TODO: __FUNCTION__ etc in debug mode.
	}
	
	// The same as unwrap but you specify the error message.
	T& expect(const std::string& message)
	{
		if (*this)
			return mpark::get<0>(*this);
		
		std::cerr << message << std::endl;
		std::terminate();
	}
	
	// Get the error, or abort if it isn't an error.
	E& unwrap_err()
	{
		if (!*this)
			return mpark::get<1>(*this);
		
		std::cerr << "Called unwrap_err() on an Ok Result." << std::endl;
		std::terminate();
	}
	
	// Get the value or the default value.
	T unwrap_or_default()
	{
		if (*this)
			return mpark::get<0>(*this);
		return T();
	}

	// Get the value or an alternative value if it is an error.
	T& unwrap_or(T& val)
	{
		if (*this)
			return mpark::get<0>(*this);
		return val;
	}
	
	// If this result is Err() you can convert it to another result type.
	template<typename T2>
	operator Result<T2, E>()
	{
		if (!*this)
			return Result<T2, E>(mpark::get<1>(*this), mpark::monostate());
		
		std::cerr << "Error converting Result types - Result was Ok!" << std::endl;
		std::terminate();
	}
	
	// void version.
	operator Result<void, E>()
	{
		if (!*this)
			return Result<void, E>(mpark::get<1>(*this), mpark::monostate());
		
		std::cerr << "Error converting Result types - Result was Ok!" << std::endl;
		std::terminate();
	}
	
	// TODO: map(), map_err() etc.
	
	// Operator bool and operator! indicate whether the result is successful.
	operator bool() const { return mpark::variant<T, E>::index() == 0; }
	bool operator!() const { return mpark::variant<T, E>::index() != 0; }
};

// Specialisation for error-only case.
template<typename E>
class Result<void, E> : private mpark::variant<mpark::monostate, E>
{
public:
	// Default variant type is the first one, which is ok.
	Result() {}
	Result(const E& val, mpark::monostate) : mpark::variant<mpark::monostate, E>(val) {}
	Result(const E&& val, mpark::monostate) : mpark::variant<mpark::monostate, E>(val) {}
	
	void unwrap()
	{
		expect("Error unwrapping value!"); // TODO: __FUNCTION__ etc in debug mode.
	}
	
	void expect(const std::string& message)
	{
		if (*this)
			return;
		
		std::cerr << message << std::endl;
		std::terminate();
	}
	
	// Get the error, or abort if it isn't an error.
	E& unwrap_err()
	{
		if (!*this)
			return mpark::get<1>(*this);
		
		std::cerr << "Called unwrap_err() on an Ok Result." << std::endl;
		std::terminate();
	}
	
	// If this result is Err() you can convert it to another result type.
	template<typename T2>
	operator Result<T2, E>()
	{
		if (!*this)
			return Result<T2, E>(mpark::get<1>(*this), mpark::monostate());
		
		std::cerr << "Error converting Result types - Result was Ok!" << std::endl;
		std::terminate();
	}
	
	// void version.
	operator Result<void, E>()
	{
		if (!*this)
			return Result<void, E>(mpark::get<1>(*this), mpark::monostate());
		
		std::cerr << "Error converting Result types - Result was Ok!" << std::endl;
		std::terminate();
	}
	
	// TODO: map(), map_err() etc.
	
	operator bool() const { return mpark::variant<mpark::monostate, E>::index() == 0; }
	bool operator!() const { return mpark::variant<mpark::monostate, E>::index() != 0; }
};


// These types are used to make automatic template paramter value deduction work nicely.
// Because template paramters are only inferred from function parameters - not return types -
// we split the `return Ok(1);` process up into two steps. First the type T is inferred
// from the paramter to the function `Ok(T&)` and used to make an `OkVal` containing T.
//
// Next the `OkVal` is implicitly converted to a `Result<T,E>` using `operator Result<T,E>()`.
// The template parameters for `operator Result<T,E>()` is automatically inferred, because
// of... reasons.
template<typename T>
class OkVal
{
public:
	OkVal(const T& val) : value(val) {}
	OkVal(const T&& val) : value(val) {}
	
	template<typename E>
	operator Result<T, E>() const
	{
		return Result<T, E>(value);
	}
private:
	const T& value;
};

// Specialisation for void.
template<>
class OkVal<void>
{
public:
	OkVal() {}
	
	template<typename E>
	operator Result<void, E>() const
	{
		return Result<void, E>();
	}
};

// T can be inferred because it is in a parameter list.
template<typename T>
OkVal<T> Ok(const T& val)
{
	return OkVal<T>(val);
}

template<typename T>
OkVal<T> Ok(const T&& val)
{
	return OkVal<T>(val);
}

// Specialisation for void.
inline OkVal<void> Ok()
{
	return OkVal<void>();
}

// The same for errors. I haven't bothered supporting the case where E is void because
// why would you want that?
template<typename E>
class ErrVal
{
public:
	ErrVal(E& err) : error(err) {}
	ErrVal(E&& err) : error(err) {}
	
	template<typename T>
	operator Result<T, E>() const
	{
		return Result<T, E>(error, mpark::monostate());
	}
private:
	E& error;
};

// T can be inferred because it is in a parameter list.
template<typename E>
ErrVal<E> Err(E& err)
{
	return ErrVal<E>(err);
}

template<typename E>
ErrVal<E> Err(E&& err)
{
	return ErrVal<E>(err);
}

// Try macro. Only works on GCC or Clang because it uses compound statements.
#define TRY(x)              \
	({                      \
		auto&& ref = (x);   \
		if (!ref) {         \
			return ref;     \
		}                   \
		ref.unwrap();       \
	})


// A compromise for MSVC: TRY() where you don't need to use the result.
#define MSTRY(x)            \
	do {                    \
		auto&& ref = (x);   \
		if (!ref) {         \
			return ref;     \
		}                   \
	} while (0)

// A compromise for MSVC: TRY() where you want to assign the result to a variable.
#define MSTRY_ASSIGN(o, x)  \
	do {                    \
		auto&& ref = (x);   \
		if (!ref) {         \
			return ref;     \
		}                   \
        o = ref.unwrap();   \
	} while (0)


// Result where the error type is a string for convenience.
template<typename T>
using SResult = Result<T, std::string>;


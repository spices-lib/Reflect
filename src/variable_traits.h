#pragma once
#include <type_traits>
#include <tuple>

namespace detail {
	
	template<typename T> 
	struct variable_type {
		using type = T;
	};

	template<typename Class, typename T> 
	struct variable_type<T Class::*> {
		using type = T;
	};
}

template<typename T>
using variable_type_t = typename detail::variable_type<T>::type;

namespace internal {

	template<typename T> 
	struct basic_variable_traits {
		using type = variable_type_t<T>;
		static constexpr bool is_member = std::is_member_pointer_v<T>;
	};
}

template<typename>
struct variable_traits;

template<typename T>
struct variable_traits<T *> : internal::basic_variable_traits<T>
{
	using pointer_type = T *;
};

template<typename Class, typename T>
struct variable_traits<T Class::*> : internal::basic_variable_traits<T Class::*>
{
	using pointer_type = T Class::*;
};
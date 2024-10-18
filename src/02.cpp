#include <type_traits>
#include <tuple>
#include <string>
#include "function_traits.h"
#include "variable_traits.h"

struct Person final
{
	std::string familyName;
	float height;
	bool isFemale;

	void IntroduceMyself() const {}
	bool IsFemale() const { return false; }
	bool GetMarried(Person& other)
	{
		bool success = other.isFemale != isFemale;
		if (isFemale)
		{
			familyName = "Mrs." + other.familyName;
		}
		else
		{
			familyName = "Mr." + familyName;
		}
		return success;
	}
};

template<typename T>
struct TypeInfo;

template<typename RetT, typename ...Params>
auto function_pointer_type(RetT(*)(Params...)) -> RetT(*)(Params...);

template<typename RetT, typename Class, typename ...Params>
auto function_pointer_type(RetT(Class::*)(Params...)) -> RetT(Class::*)(Params...);

template<typename RetT, typename Class, typename ...Params>
auto function_pointer_type(RetT(Class::*)(Params...) const ) -> RetT(Class::*)(Params...) const;

template<auto F>
using function_pointer_type_t = decltype(function_pointer_type(F));

template<auto F>
using function_traits_t = function_traits<function_pointer_type_t<F>>;

template<typename T>
struct is_function
{
	static constexpr bool value = std::is_function_v<std::remove_pointer_t<T>> || std::is_member_function_pointer_v<T>;
};

template<typename T>
constexpr bool is_function_v = is_function<T>::value;

template<typename T, bool isFunc>
struct basic_field_traits;

template<typename T>
struct basic_field_traits<T, true> : public function_traits<T>
{
	using traits = function_traits<T>;

	constexpr bool is_member() const
	{
		return traits::is_member;
	}

	constexpr bool is_const() const
	{
		return traits::is_const;
	}

	constexpr bool is_function() const
	{
		return true;
	}

	constexpr bool is_variable() const
	{
		return false;
	}
};

template<typename T>
struct basic_field_traits<T, false> : public variable_traits<T>
{
	using traits = variable_traits<T>;

	constexpr bool is_member() const
	{
		return traits::is_member;
	}

	constexpr bool is_const() const
	{
		return traits::is_const;
	}

	constexpr bool is_function() const
	{
		return false;
	}

	constexpr bool is_variable() const
	{
		return true;
	}

	constexpr size_t param_count() const
	{
		return std::tuple_size_v<typename traits::args>;
	}
};

template<typename T>
struct field_traits : public basic_field_traits<T, is_function_v<T>>
{
	constexpr field_traits(T&& pointer, std::string_view name) : pointer{ pointer }, name(name.substr(name.find_last_of(":"))) {}

	T pointer;
	std::string_view name;
};

#define BEGIN_CLASS(x) template<> struct TypeInfo<x> {
#define functions(...)  static constexpr auto functions = std::make_tuple(__VA_ARGS__);
#define func(F)          field_traits{ F, #F }
#define END_CLASS() };

BEGIN_CLASS(Person)
	functions(
		func(&Person::GetMarried),
		func(&Person::IntroduceMyself),
		func(&Person::IsFemale)
	)
END_CLASS()

template<typename T>
constexpr auto reflected_type()
{
	return TypeInfo<T>();
}

template<size_t Idx, typename ...Args, typename Class>
void VisitTuple(const std::tuple<Args...>& tuple, Class* instance)
{
	using tuple_type = std::tuple<Args...>;

	if constexpr (Idx >= std::tuple_size_v<tuple_type>)
	{
		return;
	}
	else
	{
		if constexpr (auto elem = std::get<Idx>(tuple); elem.param_count()= 0)
		{
			(instance->*elem.pointer)();

			VisitTuple<Idx + 1>(tuple, instance);
		}
	}
}

auto t = std::make_tuple(1, 2, 3, 4);

template<size_t... Idx, typename Tuple, typename Function>
void VisitTuple(Tuple tuple, Function&& f, std::index_sequence<Idx...>)
{
	(f(std::get<Idx>(tuple)), ...);
}

template<typename ...Args>
struct type_list
{
	static constexpr size_t size = sizeof...(Args);

};

namespace detail {

	// get first param.

	template<typename>
	struct head;

	template<typename T, typename ...Remains>
	struct head<type_list<T, Remains...>>
	{
		using type = T;
	};

	// get last param

	template<typename>
	struct tail;

	template<typename T, typename ...Remains>
	struct tail<type_list<T, Remains...>>
	{
		using type = type_list<Remains...>;
	};

	// get index param

	template<typename, size_t>
	struct nth;

	template<typename T, typename ...Remains>
	struct nth<type_list<T, Remains...>, 0>
	{
		using type = T;
	};

	template<typename T, typename ...Remains, size_t N>
	struct nth<type_list<T, Remains...>, N>
	{
		using type = typename nth<type_list<Remains...>, N-1>::type;
	};

	// calculate count meet request.

	template<typename, template<typename> typename, size_t N>
	struct count;

	template<typename T, typename ...Remains, template<typename> typename F>
	struct count<type_list<T, Remains...>, F, 0>
	{
		static constexpr int value = F<T>::value ? 1 : 0;
	};

	template<typename T, typename ...Remains, template<typename> typename F, size_t N>
	struct count<type_list<T, Remains...>, F, N>
	{
		static constexpr int value = (F<T>::value ? 1 : 0) + count<type_list<Remains...>, F, N - 1>::value;
	};

	// replace param with new param

	template<typename, template<typename> typename>
	struct map;

	template<typename ...Args, template<typename> typename F>
	struct map<type_list<Args...>, F>
	{
		using type = type_list<typename F<Args>::type...>;
	};

	// psuh a param to params first

	template<typename, typename>
	struct cons;

	template<typename ...Args, typename T>
	struct cons<T, type_list<Args...>>
	{
		using type = type_list<T, Args...>;
	};

	// combine two params

	template<typename, typename>
	struct concat;

	template<typename ...Args1, typename ...Args2>
	struct concat<type_list<Args1...>, type_list<Args2...>>
	{
		using type = type_list<Args1..., Args2...>;
	};

	// keep params except last.

	template<typename>
	struct init;

	template<typename T>
	struct init<type_list<T>>
	{
		using type = type_list<>;
	};

	template<typename T, typename ...Remains>
	struct init<type_list<T, Remains...>>
	{
		using type = typename cons<T, typename init<type_list<Remains...>>::type>::type;
	};

	// filter

	template<typename, template<typename> typename>
	struct filter;

	template<template<typename> typename F>
	struct filter<type_list<>, F>
	{
		using type = type_list<>;
	};

	template<typename T, typename ...Remains, template<typename> typename F>
	struct filter<type_list<T, Remains...>, F>
	{
		using type = std::conditional_t <F<T>::value,
			typename cons<T, typename filter<type_list<Remains...>, F>::type>::type,
			typename filter<type_list<Remains...>, F>::type
		>;
	};

}

template<typename TypeList>
using head = typename detail::head<TypeList>::type;

template<typename TypeList>
using tail = typename detail::tail<TypeList>::type;

template<typename TypeList, size_t N>
using nth = typename detail::nth<TypeList, N>::type;

template<typename TypeList, template<typename> typename F>
constexpr int count = detail::count<TypeList, F, TypeList::size - 1>::value;

template<typename T>
struct is_intergral
{
	static constexpr bool value = std::is_integral_v<T>;
};

template<typename T>
struct change_to_float
{
	using type = std::conditional_t<std::is_integral_v<T>, float, T>;
};

template<typename TypeList, typename T>
using cons = typename detail::cons<T, TypeList>::type;

template<typename TypeList1, typename TypeList2>
using concat = typename detail::concat<TypeList1, TypeList2>::type;

template<typename TypeList>
using init = typename detail::init<TypeList>::type;

template<typename TypeList, template<typename> typename F>
using filter = typename detail::filter<TypeList, F>::type;

template<typename T>
struct is_not_char
{
	static constexpr bool value = !std::is_same_v<T, char>;
};

//template<typename, size_t N>
//struct get_interger_type_count;
//
//template<typename T, typename ...Remains>
//struct get_interger_type_count<type_list<T, Remains...>, 0>
//{
//	static constexpr size_t value = std::is_integral_v<T> ? 1 : 0;
//};
//
//template<typename T, typename ...Remains, size_t N>
//struct get_interger_type_count<type_list<T, Remains...>, N>
//{
//	static constexpr size_t value = (std::is_integral_v<T> ? 1 : 0) + get_interger_type_count<type_list<Remains...>, N-1>::value;
//};



int main0()
{
	using nameType = variable_traits<decltype(&Person::familyName)>;
	using functionType = variable_traits<decltype(&Person::GetMarried)>;

	using type1 = function_pointer_type_t<&Person::GetMarried>;
	using type2 = function_pointer_type_t<&Person::IntroduceMyself>;
	using type3 = function_pointer_type_t<&Person::IsFemale>;

	static_assert(std::is_same_v<type1, bool(Person::*)(Person&)>);
	static_assert(std::is_same_v<type2, void(Person::*)(void)const>);
	static_assert(std::is_same_v<type3, bool(Person::*)(void)const>);


	auto info = reflected_type<Person>();

	VisitTuple(t, [](auto&& elem) {
		
	}, std::make_index_sequence<std::tuple_size_v<decltype(t)>>());


	using type = type_list<int, char, double, int, char, float>;
	using first_elem = head<type>;
	using tail_elem = tail<type>;
	using result = nth<type, 2>;
	constexpr auto value = count<type, is_intergral>;

	using List = typename detail::map<type, change_to_float>::type;
	using initresult = init<type>;
	using filterresult = filter<type, is_not_char>;
	return 0;
}

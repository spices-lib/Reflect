#include <type_traits>
#include <tuple>

template<typename T>
struct remove_pointer {};

template<typename T>
struct remove_pointer<T*>
{
	using type = T;
};

template<typename>
struct remove_const;

template<typename T>
struct remove_const<const T>
{
	using type = T;
};

void Foo(int, double) {};

struct Person
{
	bool IsFemale() {}
	bool IsFemaleConst() const {}
};

template<typename>
struct function_traits;

template<typename Ret, typename ...Args>
struct function_traits<Ret(*)(Args...)>
{
	using return_type = Ret;
	using param_type = std::tuple<Args...>;
	static constexpr bool is_class = true;
};

template<typename Ret, typename Class, typename ...Args>
struct function_traits<Ret(Class::*)(Args...)>
{
	using return_type = Ret;
	using class_type = Class;
	using param_type = std::tuple<Args...>;
	static constexpr bool is_class = true;
};

template<typename>
struct variable_traits;

template<typename Ret, typename Class>
struct variable_traits<Ret(Class::*)>
{
	using return_type = Ret;
	using class_type = Class;
	static constexpr bool is_variable = true;
};

template<typename T>
struct TypeInfo;

#define BEGIN_CLASS(T) template<> struct TypeInfo<T> { using type = T;                                         
#define FUNCTIONS(...) using function_traits = std::tuple<__VA_ARGS__>;
#define END_CLASS() };

BEGIN_CLASS(Person)
FUNCTIONS(function_traits<decltype(&Person::IsFemale)>, function_traits<decltype(&Person::IsFemaleConst)>)
END_CLASS()

int main1()
{
	using type = TypeInfo<Person>;


	return 0;
}
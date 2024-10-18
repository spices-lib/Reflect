#include <type_traits>
#include <tuple>
#include <string>
#include <vector>
#include "function_traits.h"
#include "variable_traits.h"
#include <iostream>


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

class Numeric;
class Enum;
class Class;

class Type
{
public:

	template<typename T>
	friend class EnumFactory;

	template<typename T>
	friend class ClassFactory;

	enum class Kind
	{
		Numeric,
		Enum,
		Class,
	};

	virtual ~Type() = default;

	Type(const std::string& name, Kind kind) : name_(name), kind_(kind) {}

	auto& GetName() const { return name_; }
	auto& GetKind() const { return kind_; }

	const Numeric* AsNumeric() const
	{
		if (kind_ == Kind::Numeric)
		{
			return reinterpret_cast<const Numeric*>(this);
		}
		else
		{
			return nullptr;
		}
	}

	const Enum* AsEnum() const
	{
		if (kind_ == Kind::Enum)
		{
			return reinterpret_cast<const Enum*>(this);
		}
		else
		{
			return nullptr;
		}
	}

	const Class* AsClass() const
	{
		if (kind_ == Kind::Class)
		{
			return reinterpret_cast<const Class*>(this);
		}
		else
		{
			return nullptr;
		}
	}

private:
	std::string name_;
	Kind kind_;
};

class Numeric : public Type
{
public:

	enum class Kind {
		Unkonwn, 
		Int8, 
		Int16, 
		Int32, 
		Int64, 
		Int128,
		Float, 
		Double
	};

	Numeric(Kind kind, bool isSigned) : Type(getName(kind), Type::Kind::Numeric),  kind_(kind), isSigned_(isSigned) {}

	auto GetKind() const { return kind_; }
	bool isSigned() const { return isSigned_; }

	template<typename T>
	static Numeric Create()
	{
		return Numeric{ detectKind<T>(), std::is_signed_v<T> };
	}

private:

	Kind kind_;
	bool isSigned_;

	static std::string getName(Kind kind)
	{
		switch (kind)
		{
		case Kind::Int8:
			return "int8";
		case Kind::Int16:
			return "int16";
		case Kind::Int32:
			return "int32";
		case Kind::Int64:
			return "int64";
		case Kind::Int128:
			return "int128";
		case Kind::Float:
			return "float";
		case Kind::Double:
			return "double";
		}

		return "UnKonwn";
	}

	template<typename T>
	static Kind detectKind()
	{
		if constexpr (std::is_same_v<T, char>)
		{
			return Kind::Int8;
		}
		else if constexpr (std::is_same_v<T, short>)
		{
			return Kind::Int16;
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			return Kind::Int32;
		}
		else if constexpr (std::is_same_v<T, long>)
		{
			return Kind::Int64;
		}
		else if constexpr (std::is_same_v<T, long long>)
		{
			return Kind::Int128;
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return Kind::Float;
		}
		else if constexpr (std::is_same_v<T, double>)
		{
			return Kind::Double;
		}
		else
		{
			return Kind::Unkonwn;
		}
	}
};

class Enum : public Type
{
public:
	struct Item
	{
		using value_type = uint64_t;
		std::string name;
		value_type value;
	};

	Enum() : Type("Unknown", Type::Kind::Enum) {}
	Enum(const std::string& name) : Type(name, Type::Kind::Enum) {}

	template<typename T>
	void Add(const std::string& name, T value)
	{
		items_.push_back(Item{ name, static_cast<typename Item::value_type>(value) });
	}

	auto& GetItems() const { return items_; }

private:
	std::vector<Item> items_;
};

class MemberVariable
{
public:
	std::string name;
	const Type* type;

	template<typename T>
	static MemberVariable Create(const std::string& name);
};

class MemberFunction
{
public:
	std::string name;
	const Type* retType;
	std::vector<const Type*> paramType;

	template<typename T>
	static MemberFunction Create(const std::string& name);

private:

	template<typename Params, size_t ...Idx>
	static std::vector<const Type*> ConvertTypeList2Vector(std::index_sequence<Idx...>);
};

class Class : public Type
{
public:
	Class() : Type("", Type::Kind::Class) {}
	Class(const std::string& name) : Type(name, Type::Kind::Class) {}

	void AddVar(MemberVariable&& var)
	{
		vars_.push_back(std::move(var));
	}

	void AddFunc(MemberFunction&& func)
	{
		funcs_.push_back(std::move(func));
	}

	auto& GetVariable() const { return vars_; }
	auto& GetFunctions() const { return funcs_; }

private:
	std::vector<MemberVariable> vars_;
	std::vector<MemberFunction> funcs_;

};

template<typename T>
class NumericFactory final
{
public:
	static NumericFactory& Instance()
	{
		static NumericFactory inst{ Numeric::Create<T>() };
		return inst;
	}

	auto& Info() const { return info_; }

private:
	Numeric info_;

	NumericFactory(Numeric&& info) : info_(std::move(info)) {}
};

template<typename T>
class EnumFactory final
{
public:
	static EnumFactory& Instance()
	{
		static EnumFactory inst;
		return inst;
	}

	auto& Info() const { return info_; }

	EnumFactory& Regist(const std::string& name)
	{
		info_.name_ = name;
		return *this;
	}

	template<typename U>
	EnumFactory& Add(const std::string& name, U value)
	{
		info_.Add(name, value);
		return *this;
	}

	void UnRegist()
	{
		info_ = Enum();
	}

private:
	Enum info_;
};

template<typename T>
class ClassFactory final
{
public:
	static ClassFactory& Instance()
	{
		static ClassFactory inst;
		return inst;
	}

	auto& Info() const { return info_; }

	ClassFactory& Regist(const std::string& name)
	{
		info_.name_ = name;
		return *this;
	}

	template<typename U>
	ClassFactory& AddVariable(const std::string& name)
	{
		info_.AddVar(MemberVariable::Create<U>(name));
		return *this;
	}

	template<typename U>
	ClassFactory& AddFunction(const std::string& name)
	{
		info_.AddFunc(MemberFunction::Create<U>(name));
		return *this;
	}

	void UnRegist()
	{
		info_ = Class();
	}

private:
	Class info_;
};

class TrivialFactory
{
public:
	static TrivialFactory& Instance()
	{
		static TrivialFactory inst;
		return inst;
	}
};

template<typename T>
class Factory final
{
public:
	static auto& GetFactory()
	{
		using type = std::remove_reference_t<T>;

		if constexpr (std::is_fundamental_v<type>)
		{
			return NumericFactory<type>::Instance();
		}
		else if constexpr (std::is_enum_v<type>)
		{
			return EnumFactory<type>::Instance();
		}
		else if constexpr (std::is_class_v<type>)
		{
			return ClassFactory<type>::Instance();
		}
	}
};


template<typename T>
auto& Registrar()
{
	return Factory<T>::GetFactory();
}

template<typename T>
const Type* GetType()
{
	return &Factory<T>::GetFactory().Info();
}

enum class MyEnum
{
	value1 = 1,
	value2 = 2,
};

template<typename T>
MemberVariable MemberVariable::Create(const std::string& name)
{
	using type = typename variable_traits<T>::type;
	return MemberVariable{ name, GetType<type>() };
}

template<typename T>
MemberFunction MemberFunction::Create(const std::string& name)
{
	using traits = function_traits<T>;
	using args = typename traits::args;
	return MemberFunction{ name, GetType<typename traits::return_type>(), ConvertTypeList2Vector<args>(std::make_index_sequence<std::tuple_size_v<args>>())};
}

template<typename Params, size_t ...Idx>
std::vector<const Type*> MemberFunction::ConvertTypeList2Vector(std::index_sequence<Idx...>)
{
	return { GetType<std::tuple_element_t<Idx, Params>>() ... };
}

int main()
{
	Registrar<MyEnum>().Regist("MyEnum").Add("Value1", MyEnum::value1).Add("Value2", MyEnum::value2);
	const Enum* typeInfo = GetType<MyEnum>()->AsEnum();
	std::cout << typeInfo->GetName() << std::endl;
	for (auto& item : typeInfo->GetItems())
	{
		std::cout << item.name << ", " << item.value << std::endl;
	}

	Registrar<Person>().Regist("Person")
		.AddVariable<decltype(&Person::height)>("height")
		.AddFunction<decltype(&Person::GetMarried)>("GetMarried");

	auto type = GetType<Person>();
	auto classInfo = type->AsClass();
	for (auto variable : classInfo->GetVariable())
	{
		std::cout << variable.name << std::endl;
		std::cout << variable.type->GetName() << std::endl;
	}
	for (auto func : classInfo->GetFunctions())
	{
		std::cout << func.retType << ". " << func.name;
		for (auto param : func.paramType)
		{
			std::cout << param->GetName() << ". ";
		}
		std::cout << std::endl;
	}

}
#include <type_traits>
#include <tuple>
#include <string>
#include <vector>
#include "function_traits.h"
#include "variable_traits.h"
#include <iostream>

class any
{
public:
	enum class storage_type
	{
		Empty,
		Copy,
		Steal,
		Ref,
		ConstRef,
	};

	struct operations
	{
		any(*copy)(const any&) = {};
		any(*steal)(any&) = {};
		void(*release)(any&) = {};
	};

	any() = default;

	any(const any& o)
		: typeInfo_{ o.typeInfo_ }
		, store_type{ o.store_type }
		, ops{ o.ops }
	{
		if (ops.copy)
		{
			auto new_any = ops.copy(o);
			payload_ = new_any.payload_;
			new_any.payload_ = nullptr;
			new_any.ops.release = nullptr;
		}
		else
		{
			store_type = storage_type::Empty;
			typeInfo_ = nullptr;
		}
	}

	any(any&& o)
		: typeInfo_(o.typeInfo_)
		, payload_(std::move(o.payload_))
		, store_type(std::move(o.store_type))
		, ops(std::move(o.ops))
	{}

	any(any&& o)
		: typeInfo_(o.typeInfo_)
		, payload_(std::move(o.payload_))
		, store_type(std::move(o.store_type))
		, ops(std::move(o.ops))
	{}

	~any()
	{
		if (ops.release &&
			(store_type == storage_type::Copy ||
				store_type == storage_type::Steal))
		{
			ops.release(*this);
		}
	}

	Type* typeInfo_;
	void* payload_;
	storage_type store_type = storage_type::Empty;
	operations ops;
private:
};

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

	void SetValue(double value, any& elem)
	{
		if (elem.typeInfo_->GetKind() == Type::Kind::Numeric)
		{
			switch (elem.typeInfo_->AsNumeric()->GetKind())
			{
			case Kind::Int8:
				*(char*)elem.payload_ = value;
			}
		}
	}

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

class Member 
{
public:
	virtual any call(const std::vector<any>& anies) = 0;
};

template<typename Clazz, typename Type>
class MemberVariable : public Member
{
public:
	std::string name;
	const Type* type;
	Type Clazz::* ptr;

	virtual any call(const std::vector<any>& anies) override
	{
		assert(anies.size() == 1 && anies[0].typeInfo_ == GetType<Clazz>());
		Clazz* instance = (Clazz*)anies[0].payload_;
		auto value = instance->*ptr;
		return make_copy(value);
	}

	template<typename T>
	static MemberVariable Create(const std::string& name);
};

template<typename T, bool IsConst>
std::conditional_t<IsConst, const T&, T&> unwarp(any& value)
{
	if constexpr (IsConst)
	{
		assert(value.typeInfo_ == GetType<T>());
	}
	else
	{

	}

	return *(T*)value.payload_;
}

template<typename Clazz, typename RetType, size_t ...Idx, typename ...Args>
void inner_call(RetType(Clazz::* ptr)(Args...), const std::vector<any>& params, std::index_sequence<Idx...>)
{
	auto return_value = ((Clazz*)params[0].payload_->*ptr)(unwarp<Args>(params[Idx+1])...);
	return make_copy(return_value);
}

template<typename Clazz, typename Type, typename ...Args>
class MemberFunction : public Member
{
public:
	std::string name;
	const Type* retType;
	Type (Clazz::* ptr)(Args...);
	std::vector<const Type*> paramType;

	virtual any call(const std::vector<any>& anies) override
	{
		assert(anies.size() == paramType.size() + 1);

		for (int i = 0; i < paramType.size(); i++)
		{
			assert(paramType[i] == anies[i+1].payload_);
		}

		return inner_call(ptr, anies, std::make_index_sequence<sizeof...(Args)>);
	}

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

template<typename T>
any make_copy(const T& elem);

template<typename T>
any make_steal(T&&);

template<typename T>
any make_ref(T&);

template<typename T>
any make_cref(const T&);



template<typename T>
T* try_cast(any& elem)
{
	if (elem.typeInfo_ == GetType<T>())
	{
		return (T*)(elem.payload_);
	}
	else
	{
		return nullptr;
	}
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

template<typename T>
struct operations_traits
{
	static any copy(const any& elem)
	{
		assert(elem.typeInfo_ == GetType<T>());

		any return_value;

		return_value.payload_   = new T{ *static_cast<T*>(elem.payload_) };
		return_value.typeInfo_  = elem.typeInfo_;
		return_value.store_type = any::store_type::Copy;
		return_value.ops        = elem.ops;

		return return_value;
	}

	static any steal(any& elem)
	{
		assert(elem.typeInfo_ == GetType<T>());

		any return_value;

		return_value.payload_   = new T{ std::move(*static_cast<T*>(elem.payload_)) };
		return_value.typeInfo_  = elem.typeInfo_;
		return_value.store_type = any::store_type::Copy;
		elem.store_type         = any::store_type::Steal;
		return_value.ops        = elem.ops;

		return return_value;
	}

	static void release(any& elem)
	{
		assert(elem.typeInfo_ == GetType<T>());

		delete static_cast<T*>(elem.payload_);
		elem.payload_           = nullptr;
		elem.store_type         = any::store_type::Empty;
		elem.typeInfo_          = nullptr;
	}
};

template<typename T>
any make_copy(const T& elem)
{
	any return_value;
	return_value.payload_    = new T{ elem };
	return_value.typeInfo_   = GetType<T>();
	return_value.store_type  = any::store_type::Copy;

	if constexpr(std::is_copy_constructible_v<T>)
	{
		return_value.ops.copy    = &operations_traits<T>::copy;
	}

	if constexpr (std::is_move_constructible_v<T>)
	{
		return_value.ops.steal   = &operations_traits<T>::steal;
	}

	if constexpr (std::is_destructible_v<T>)
	{
		return_value.ops.release = &operations_traits<T>::release;
	}

	return return_value;
}

template<typename T>
any make_steal(T&& elem)
{
	any return_value;
	return_value.payload_    = new T{ std::move(elem) };
	return_value.typeInfo_   = GetType<T>();
	return_value.store_type  = any::store_type::Steal;

	if constexpr(std::is_copy_constructible_v<T>)
	{
		return_value.ops.copy    = &operations_traits<T>::copy;
	}

	if constexpr (std::is_move_constructible_v<T>)
	{
		return_value.ops.steal   = &operations_traits<T>::steal;
	}

	if constexpr (std::is_destructible_v<T>)
	{
		return_value.ops.release = &operations_traits<T>::release;
	}

	return return_value;
}

template<typename T>
any make_ref(T& elem)
{
	any return_value;
	return_value.payload_    = &elem;
	return_value.typeInfo_   = GetType<T>();
	return_value.store_type  = any::store_type::Ref;

	if constexpr(std::is_copy_constructible_v<T>)
	{
		return_value.ops.copy    = &operations_traits<T>::copy;
	}

	if constexpr (std::is_move_constructible_v<T>)
	{
		return_value.ops.steal   = &operations_traits<T>::steal;
	}

	if constexpr (std::is_destructible_v<T>)
	{
		return_value.ops.release = &operations_traits<T>::release;
	}

	return return_value;
}

template<typename T>
any make_cref(const T& elem)
{
	any return_value;
	return_value.payload_    = &elem;
	return_value.typeInfo_   = GetType<T>();
	return_value.store_type  = any::store_type::ConstRef;

	if constexpr(std::is_copy_constructible_v<T>)
	{
		return_value.ops.copy    = &operations_traits<T>::copy;
	}

	if constexpr (std::is_move_constructible_v<T>)
	{
		return_value.ops.steal   = &operations_traits<T>::steal;
	}

	if constexpr (std::is_destructible_v<T>)
	{
		return_value.ops.release = &operations_traits<T>::release;
	}

	return return_value;
}

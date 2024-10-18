#include <type_traits>
#include <tuple>
#include <string>
#include <vector>
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

class Type
{
public:

	template<typename T>
	friend class EnumFactory;

	virtual ~Type() = default;

	Type(const std::string& name) : name_(name) {}

	auto& GetName() const { return name_; }

private:
	std::string name_;
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

	Numeric(Kind kind, bool isSigned) : Type(getName(kind)),  kind_(kind), isSigned_(isSigned) {}

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

	Enum() : Type("Unknown") {}
	Enum(const std::string& name) : Type(name) {}

	template<typename T>
	void Add(const std::string& name, T value)
	{
		items_.push_back(Item(name, static_cast<typename Item::value_type>(value)));
	}

	auto& GetItems() const { return items_; }

private:
	std::vector<Item> items_;
};

class Class : public Type
{
public:
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
		info_.name = name;
		return *this;
	}

	template<typename U>
	EnumFactory& Add(const std::string& name, U value)
	{
		info_.Add(name, value);
		return *this;
	}

private:
	Enum info_;
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
		if constexpr (std::is_fundamental_v<T>)
		{
			return NumericFactory<T>::Instance();
		}
		else if constexpr (std::is_enum_v<T>)
		{
			return EnumFactory<T>::Instance();
		}
		else if constexpr (std::is_class_v<T>)
		{
			return TrivialFactory::Instance();
		}
	}
};


template<typename T>
auto& Registry()
{
	return Factory<T>::GetFactory();
}

enum class MyEnum
{
	value1 = 1,
	value2 = 2,
};

int main()
{
	Registry<MyEnum>.Regist("MyEnum").Add("Value1", MyEnum::value1).Add("Value2", MyEnum::value2);
}
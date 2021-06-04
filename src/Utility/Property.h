#pragma once

#include "StringUtils.h"
#include <variant>
#undef Bool

namespace slade
{
using Property    = std::variant<bool, int, unsigned int, double, string>;
using PropertyMap = std::map<string, Property, std::less<>>;

namespace property
{
	enum class ValueType
	{
		Bool,
		Int,
		UInt,
		Float,
		String
	};

	// Returns the ValueType of the value currently held in [prop]
	inline ValueType valueType(const Property& prop) { return static_cast<ValueType>(prop.index()); }

	// Returns true if the given [prop] contains a value
	inline bool hasValue(const Property& prop) { return prop.index() > 0; }

	// Returns the value of [prop] if it is of type T, otherwise an empty std::optional
	template<typename T> std::optional<T> value(const Property& prop)
	{
		if (auto val = std::get_if<T>(&prop))
			return *val;

		return {};
	}

	// Returns the value of [prop] if it is of type T, otherwise [default_value]
	template<typename T> T value(const Property& prop, T default_value)
	{
		if (auto val = std::get_if<T>(&prop))
			return *val;

		return default_value;
	}

	// Conversion
	bool         asBool(const Property& prop);
	int          asInt(const Property& prop);
	unsigned int asUInt(const Property& prop);
	double       asFloat(const Property& prop);
	string       asString(const Property& prop, int float_precision = 0);

} // namespace property

class PropertyList
{
public:
	const vector<Named<Property>>& properties() const { return properties_; }
	
	Property& operator[](string_view key)
	{
		for (auto& prop : properties_)
			if (strutil::equalCI(key, prop.name))
				return prop.value;

		properties_.emplace_back(key, Property{});
		return properties_.back().value;
	}

	bool empty() const { return properties_.empty(); }

	bool contains(string_view key) const
	{
		for (const auto& prop : properties_)
			if (strutil::equalCI(key, prop.name))
				return true;

		return false;
	}

	template<typename T> T get(string_view key) const
	{
		for (const auto& prop : properties_)
			if (strutil::equalCI(key, prop.name))
				return std::get<T>(prop.value);

		return T{};
	}
	
	std::optional<Property> getIf(string_view key) const
	{
		for (const auto& prop : properties_)
			if (strutil::equalCI(key, prop.name))
				return prop.value;

		return {};
	}

	template<typename T> std::optional<T> getIf(string_view key) const
	{
		for (const auto& prop : properties_)
			if (strutil::equalCI(key, prop.name))
				return property::value<T>(prop.value);

		return {};
	}

	template<typename T> T getOr(string_view key, T default_val) const
	{
		for (const auto& prop : properties_)
			if (strutil::equalCI(key, prop.name))
				return property::value<T>(prop.value, default_val);

		return default_val;
	}

	void allProperties(vector<Property>& list)
	{
		for (const auto& prop : properties_)
			list.push_back(prop.value);
	}

	void allPropertyNames(vector<string>& list)
	{
		for (const auto& prop : properties_)
			list.push_back(prop.name);
	}

	void clear() { properties_.clear(); }

	bool remove(string_view key)
	{
		const auto count = properties_.size();
		for (unsigned i = 0; i < count; ++i)
			if (strutil::equalCI(key, properties_[i].name))
			{
				properties_.erase(properties_.begin() + i);
				return true;
			}

		return false;
	}

	string toString(bool condensed = false, int float_precision = 0) const;

private:
	vector<Named<Property>> properties_;
};
} // namespace slade










#if 0
namespace slade
{
class Property
{
public:
	enum class Type
	{
		Boolean,
		Int,
		Float,
		String,
		Flag, // The 'flag' property type mimics a boolean property that is always true
		UInt
	};

	union Value {
		bool     Boolean;
		int      Integer;
		double   Floating;
		unsigned Unsigned;
	};

	Property(Type type = Type::Boolean); // Default property type is bool
	Property(const Property& copy) = default;
	Property(bool value);
	Property(int value);
	Property(float value);
	Property(double value);
	Property(string_view value);
	Property(unsigned value);
	~Property() = default;

	Type type() const { return type_; }
	bool isType(Type type) const { return type_ == type; }
	bool hasValue() const { return has_value_; }

	void changeType(Type newtype);
	void setHasValue(bool hv) { has_value_ = hv; }

	operator bool() const { return boolValue(); }
	operator int() const { return intValue(); }
	operator float() const { return (float)floatValue(); }
	operator double() const { return floatValue(); }
	operator string() const { return stringValue(); }
	operator unsigned() const { return unsignedValue(); }

	Property& operator=(bool val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(int val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(float val)
	{
		setValue(static_cast<double>(val));
		return *this;
	}

	Property& operator=(double val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(string_view val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(unsigned val)
	{
		setValue(val);
		return *this;
	}

	bool operator==(string_view cmp) const { return val_string_ == cmp; }

	bool     boolValue(bool warn_wrong_type = false) const;
	int      intValue(bool warn_wrong_type = false) const;
	double   floatValue(bool warn_wrong_type = false) const;
	string   stringValue(bool warn_wrong_type = false) const;
	unsigned unsignedValue(bool warn_wrong_type = false) const;

	void setValue(bool val);
	void setValue(int val);
	void setValue(double val);
	void setValue(string_view val);
	void setValue(unsigned val);

	string typeString() const;

private:
	Type   type_ = Type::Boolean;
	Value  value_;
	string val_string_; // I *would* put this in the union but i'm not sure about using const char* there
	bool   has_value_ = false;
};
} // namespace slade
#endif

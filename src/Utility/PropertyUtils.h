#pragma once

#include "Property.h"
#include "StringUtils.h"

namespace slade::property
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
inline ValueType valueType(const Property& prop)
{
	return static_cast<ValueType>(prop.index());
}

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
} // namespace slade::property

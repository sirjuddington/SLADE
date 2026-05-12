#pragma once

#include "General/JsonFwd.h"
#include "Utility/Property.h"

namespace slade::game
{
class UDMFProperty
{
public:
	enum class Type
	{
		Boolean = 0,
		Int,
		Float,
		String,
		Colour,
		ActionSpecial,
		SectorSpecial,
		ThingType,
		Angle,
		TextureWall,
		TextureFlat,
		ID,
		Unknown
	};

	UDMFProperty();
	~UDMFProperty() = default;

	int                       order() const { return order_; }
	const string&             propName() const { return property_; }
	const string&             name() const { return name_; }
	const string&             group() const { return group_; }
	Type                      type() const { return type_; }
	const Property&           defaultValue() const { return default_value_; }
	bool                      hasDefaultValue() const { return has_default_; }
	bool                      hasPossibleValues() const { return !values_.empty(); }
	const vector<Property>&   possibleValues() const { return values_; }
	bool                      isFlag() const { return flag_; }
	bool                      isTrigger() const { return trigger_; }
	bool                      showAlways() const { return show_always_; }
	bool                      internalOnly() const { return internal_only_; }
	template<typename T> bool isDefault(T value) const;

	void parse(const ParseTreeNode* node, string_view group);
	void fromJson(const Json& j, string_view group);

	string getStringRep();

private:
	int              order_;
	string           property_;
	string           name_;
	string           group_;
	Type             type_        = Type::Unknown;
	bool             flag_        = false;
	bool             trigger_     = false;
	bool             has_default_ = false;
	Property         default_value_;
	vector<Property> values_;
	bool             show_always_   = false;
	bool             internal_only_ = false;
};

template<typename T> bool UDMFProperty::isDefault(T value) const
{
	if (has_default_)
	{
		auto val = std::get_if<T>(&default_value_);
		if (val && *val == value)
			return true;
	}

	return false;
}
} // namespace slade::game

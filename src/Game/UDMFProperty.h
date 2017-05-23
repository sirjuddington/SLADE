#pragma once

#include "Utility/PropertyList/Property.h"

class ParseTreeNode;
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
	~UDMFProperty();

	const string&			propName() const { return property_; }
	const string&			name() const { return name_; }
	const string&			group() const { return group_; }
	Type					type() const { return type_; }
	const Property&			defaultValue() const { return default_value_; }
	bool					hasDefaultValue() const { return has_default_; }
	bool					hasPossibleValues() const { return !values_.empty(); }
	const vector<Property>&	possibleValues() const { return values_; }
	bool					isFlag() const { return flag_; }
	bool					isTrigger() const { return trigger_; }
	bool					showAlways() const { return show_always_; }
	bool					internalOnly() const { return internal_only_; }

	void	parse(ParseTreeNode* node, string group);

	string	getStringRep();

private:
	string				property_;
	string				name_;
	string				group_;
	Type				type_;
	bool				flag_;
	bool				trigger_;
	bool				has_default_;
	Property			default_value_;
	vector<Property>	values_;
	bool				show_always_;
	bool				internal_only_;
};

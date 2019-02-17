#pragma once

#include "Utility/PropertyList/Property.h"

class MobjPropertyList
{
public:
	struct Prop
	{
		std::string name;
		Property    value;

		Prop(std::string_view name) : name{ name } {}
		Prop(std::string_view name, const Property& value) : name{ name }, value{ value } {}
	};

	MobjPropertyList()  = default;
	~MobjPropertyList() = default;

	// Operator for direct access to property by name
	Property& operator[](std::string_view key)
	{
		for (auto& prop : properties_)
		{
			if (prop.name == key)
				return prop.value;
		}

		properties_.emplace_back(key);
		return properties_.back().value;
	}

	vector<Prop>& allProperties() { return properties_; }

	void clear() { properties_.clear(); }
	bool propertyExists(std::string_view key);
	bool removeProperty(std::string_view key);
	void copyTo(MobjPropertyList& list);
	void addFlag(std::string_view key);
	bool isEmpty() const { return properties_.empty(); }

	std::string toString(bool condensed = false);

private:
	vector<Prop> properties_;
};

#pragma once

#include "Utility/PropertyList/Property.h"

class MobjPropertyList
{
public:
	struct Prop
	{
		string   name;
		Property value;

		Prop(const string& name) : name{ name } {}
		Prop(const string& name, const Property& value) : name{ name }, value{ value } {}
	};

	MobjPropertyList() = default;
	~MobjPropertyList() = default;

	// Operator for direct access to hash map
	Property& operator[](const string& key)
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
	bool propertyExists(const string& key);
	bool removeProperty(string key);
	void copyTo(MobjPropertyList& list);
	void addFlag(string key);
	bool isEmpty() const { return properties_.empty(); }

	string toString(bool condensed = false);

private:
	vector<Prop> properties_;
};

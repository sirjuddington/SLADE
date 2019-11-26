#pragma once

#include "Property.h"

class PropertyList
{
public:
	PropertyList()  = default;
	~PropertyList() = default;

	Property& operator[](string_view key) { return getOrAdd(key); }

	void clear() { properties_.clear(); }
	bool propertyExists(string_view key) const;
	bool removeProperty(string_view key);
	void copyTo(PropertyList& list) const;
	void addFlag(string_view key) { getOrAdd(key) = Property{}; }
	void allProperties(vector<Property>& list, bool ignore_no_value = false) const;
	void allPropertyNames(vector<string>& list, bool ignore_no_value = false) const;
	bool empty() const { return properties_.empty(); }

	string toString(bool condensed = false) const;

private:
	struct Item
	{
		string   key;
		Property prop;

		Item(string_view key) : key{ key } {}
		Item(string_view key, const Property& prop) : key{ key }, prop{ prop } {}
	};
	vector<Item> properties_;

	// Returns the property matching [key] or adds a new prop and returns it
	Property& getOrAdd(string_view key)
	{
		for (auto& prop : properties_)
			if (prop.key == key)
				return prop.prop;

		properties_.emplace_back(key);
		return properties_.back().prop;
	}
};

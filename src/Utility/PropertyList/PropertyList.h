#pragma once

#include "Property.h"

class PropertyList
{
public:
	PropertyList()  = default;
	~PropertyList() = default;

	// Operator for direct access to hash map
	Property& operator[](const std::string& key) { return properties_[key]; }

	void clear() { properties_.clear(); }
	bool propertyExists(const std::string& key);
	bool removeProperty(const std::string& key);
	void copyTo(PropertyList& list);
	void addFlag(const std::string& key);
	void allProperties(vector<Property>& list, bool ignore_no_value = false);
	void allPropertyNames(vector<std::string>& list, bool ignore_no_value = false);
	bool empty() const { return properties_.empty(); }

	std::string toString(bool condensed = false) const;

private:
	std::map<std::string, Property> properties_;
};

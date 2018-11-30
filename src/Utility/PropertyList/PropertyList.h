#pragma once

#include "Property.h"

class PropertyList
{
public:
	PropertyList();
	~PropertyList();

	// Operator for direct access to hash map
	Property& operator[](const string& key) { return properties_[key]; }

	void clear() { properties_.clear(); }
	bool propertyExists(string key);
	bool removeProperty(string key);
	void copyTo(PropertyList& list);
	void addFlag(string key);
	void allProperties(vector<Property>& list);
	void allPropertyNames(vector<string>& list);
	bool isEmpty() { return properties_.empty(); }

	string toString(bool condensed = false);

private:
	std::map<string, Property> properties_;
};

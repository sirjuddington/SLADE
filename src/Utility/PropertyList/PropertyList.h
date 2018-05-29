
#ifndef __PROPERTY_LIST_H__
#define __PROPERTY_LIST_H__

#include "common.h"
#include "Property.h"

class PropertyList
{
private:
	std::map<string, Property>	properties;

public:
	PropertyList();
	~PropertyList();

	// Operator for direct access to hash map
	Property& operator[](const string& key) { return properties[key]; }

	void	clear() { properties.clear(); }
	bool	propertyExists(string key);
	bool	removeProperty(string key);
	void	copyTo(PropertyList& list);
	void	addFlag(string key);
	void	allProperties(vector<Property>& list);
	void	allPropertyNames(vector<string>& list);
	bool	isEmpty() { return properties.empty(); }

	string	toString(bool condensed = false);
};

#endif//__PROPERTY_LIST_H__

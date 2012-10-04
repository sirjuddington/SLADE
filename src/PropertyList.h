
#ifndef __PROPERTY_LIST_H__
#define __PROPERTY_LIST_H__

#include "Property.h"
#include <wx/hashmap.h>

// Declare hash map class to hold properties
WX_DECLARE_STRING_HASH_MAP(Property, PropertyHashMap);

class PropertyList {
private:
	PropertyHashMap	properties;

public:
	PropertyList();
	~PropertyList();

	// Operator for direct access to hash map
	Property& operator[](string key) { return properties[key]; }

	void	clear() { properties.clear(); }
	bool	propertyExists(string key);
	bool	removeProperty(string key);
	void	copyTo(PropertyList& list);
	void	addFlag(string key);
	void	allProperties(vector<Property>& list);
	void	allPropertyNames(vector<string>& list);
	bool	isEmpty() { return properties.size() == 0; }

	string	toString(bool condensed = false);
};

#endif//__PROPERTY_LIST_H__

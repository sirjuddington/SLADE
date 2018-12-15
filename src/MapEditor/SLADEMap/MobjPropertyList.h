
#ifndef __MOBJ_PROPERTY_LIST_H__
#define __MOBJ_PROPERTY_LIST_H__

#include "Utility/PropertyList/Property.h"

class MobjPropertyList
{
public:
	struct prop_t
	{
		string		name;
		Property	value;

		prop_t(const string& name) { this->name = name; }
		prop_t(const string& name, const Property& value)
		{
			this->name = name;
			this->value = value;
		}
	};

	MobjPropertyList();
	~MobjPropertyList();

	// Operator for direct access to hash map
	Property& operator[](const string& key)
	{
		for (auto& prop : properties)
		{
			if (prop.name == key)
				return prop.value;
		}

		properties.emplace_back(key);
		return properties.back().value;
	}

	vector<prop_t>&	allProperties() { return properties; }

	void	clear() { properties.clear(); }
	bool	propertyExists(const string& key);
	bool	removeProperty(string key);
	void	copyTo(MobjPropertyList& list);
	void	addFlag(string key);
	bool	isEmpty() { return properties.empty(); }

	string	toString(bool condensed = false);

private:
	vector<prop_t>	properties;
};

#endif//__MOBJ_PROPERTY_LIST_H__

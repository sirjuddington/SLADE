
#ifndef __MOBJ_PROPERTY_LIST_H__
#define __MOBJ_PROPERTY_LIST_H__

#include "Property.h"

class MobjPropertyList
{
private:
	struct prop_t
	{
		string		name;
		Property	value;

		prop_t(string name)
		{
			this->name = name;
		}

		prop_t(string name, Property value)
		{
			this->name = name;
			this->value = value;
		}
	};
	vector<prop_t>	properties;

public:
	MobjPropertyList();
	~MobjPropertyList();

	// Operator for direct access to hash map
	Property& operator[](string key)
	{
		for (unsigned a = 0; a < properties.size(); ++a)
		{
			if (properties[a].name == key)
				return properties[a].value;
		}

		properties.push_back(prop_t(key));
		return properties.back().value;
	}

	void	clear() { properties.clear(); }
	bool	propertyExists(string key);
	bool	removeProperty(string key);
	void	copyTo(MobjPropertyList& list);
	void	addFlag(string key);
	void	allProperties(vector<Property>& list);
	void	allPropertyNames(vector<string>& list);
	bool	isEmpty() { return properties.empty(); }

	string	toString(bool condensed = false);
};

#endif//__MOBJ_PROPERTY_LIST_H__

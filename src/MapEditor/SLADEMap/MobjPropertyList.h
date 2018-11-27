#pragma once

#include "Utility/PropertyList/Property.h"

class MobjPropertyList
{
public:
	struct Prop
	{
		string   name;
		Property value;

		Prop(string name) { this->name = name; }
		Prop(string name, Property value)
		{
			this->name  = name;
			this->value = value;
		}
	};

	MobjPropertyList();
	~MobjPropertyList();

	// Operator for direct access to hash map
	Property& operator[](string key)
	{
		for (unsigned a = 0; a < properties_.size(); ++a)
		{
			if (properties_[a].name == key)
				return properties_[a].value;
		}

		properties_.push_back(Prop(key));
		return properties_.back().value;
	}

	vector<Prop>& allProperties() { return properties_; }

	void clear() { properties_.clear(); }
	bool propertyExists(string key);
	bool removeProperty(string key);
	void copyTo(MobjPropertyList& list);
	void addFlag(string key);
	bool isEmpty() { return properties_.empty(); }

	string toString(bool condensed = false);

private:
	vector<Prop> properties_;
};

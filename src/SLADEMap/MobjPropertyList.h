#pragma once

#include "Utility/PropertyList/Property.h"

class MobjPropertyList
{
public:
	struct Prop
	{
		wxString name;
		Property value;

		Prop(const wxString& name) : name{ name } {}
		Prop(const wxString& name, const Property& value) : name{ name }, value{ value } {}
	};

	MobjPropertyList()  = default;
	~MobjPropertyList() = default;

	// Operator for direct access to hash map
	Property& operator[](const wxString& key)
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
	bool propertyExists(const wxString& key);
	bool removeProperty(wxString key);
	void copyTo(MobjPropertyList& list);
	void addFlag(wxString key);
	bool isEmpty() const { return properties_.empty(); }

	wxString toString(bool condensed = false);

private:
	vector<Prop> properties_;
};

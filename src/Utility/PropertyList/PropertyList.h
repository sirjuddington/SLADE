#pragma once

#include "Property.h"

class PropertyList
{
public:
	PropertyList()  = default;
	~PropertyList() = default;

	// Operator for direct access to hash map
	Property& operator[](const wxString& key) { return properties_[key]; }

	void clear() { properties_.clear(); }
	bool propertyExists(const wxString& key);
	bool removeProperty(const wxString& key);
	void copyTo(PropertyList& list);
	void addFlag(const wxString& key);
	void allProperties(vector<Property>& list, bool ignore_no_value = false);
	void allPropertyNames(vector<wxString>& list, bool ignore_no_value = false);
	bool empty() const { return properties_.empty(); }

	wxString toString(bool condensed = false) const;

private:
	std::map<wxString, Property> properties_;
};

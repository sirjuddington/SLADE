#pragma once

#include "Property.h"

namespace slade
{
class PropertyList
{
public:
	const vector<Named<Property>>& properties() const { return properties_; }

	Property& operator[](string_view key);

	bool empty() const { return properties_.empty(); }
	bool contains(string_view key) const;

	template<typename T> T                get(string_view key) const;
	std::optional<Property>               getIf(string_view key) const;
	template<typename T> std::optional<T> getIf(string_view key) const;
	template<typename T> T                getOr(string_view key, T default_val) const;

	void   allProperties(vector<Property>& list) const;
	void   allPropertyNames(vector<string>& list) const;
	void   clear();
	bool   remove(string_view key);
	string toString(bool condensed = false, int float_precision = 0) const;

private:
	vector<Named<Property>> properties_;
};


// Template function implementations -------------------------------------------

namespace strutil
{
	bool equalCI(string_view, string_view);
}

template<typename T> T PropertyList::get(string_view key) const
{
	for (const auto& prop : properties_)
		if (strutil::equalCI(key, prop.name))
			return std::get<T>(prop.value);

	return T{};
}

template<typename T> std::optional<T> PropertyList::getIf(string_view key) const
{
	for (const auto& prop : properties_)
		if (strutil::equalCI(key, prop.name))
			if (auto val = std::get_if<T>(&prop.value))
				return *val;

	return {};
}

template<typename T> T PropertyList::getOr(string_view key, T default_val) const
{
	for (const auto& prop : properties_)
		if (strutil::equalCI(key, prop.name))
		{
			if (auto val = std::get_if<T>(&prop.value))
				return *val;

			return default_val;
		}

	return default_val;
}
} // namespace slade

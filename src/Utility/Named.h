#pragma once

namespace slade
{
// Simple templated struct for string+value pairs
template<typename T> struct Named
{
	string name;
	T      value;

	Named(string_view name, const T& value) : name{ name }, value{ value } {}

	// For sorting
	bool operator<(const Named<T>& rhs) { return name < rhs.name; }
	bool operator<=(const Named<T>& rhs) { return name <= rhs.name; }
	bool operator>(const Named<T>& rhs) { return name > rhs.name; }
	bool operator>=(const Named<T>& rhs) { return name >= rhs.name; }
};
} // namespace slade

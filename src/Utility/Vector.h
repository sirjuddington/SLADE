#pragma once

namespace slade
{
// Check if a value exists in a vector
template<typename T> constexpr auto vectorContains(const vector<T>& vec, T val)
{
	return find(vec.begin(), vec.end(), val) != vec.end();
}

// Add a value to a vector if the value doesn't already exist in the vector
template<typename T> constexpr auto vectorAddUnique(vector<T>& vec, T val)
{
	if (!vectorContains(vec, val))
		vec.push_back(val);
}

// Remove the first item with a given value from a vector
template<typename T> constexpr auto vectorRemoveVal(vector<T>& vec, T val)
{
	return vec.erase(find(vec.begin(), vec.end(), val));
}

// Remove an item at the given index from a vector
template<typename T> constexpr auto vectorRemoveAt(vector<T>& vec, unsigned index)
{
	return vec.erase(vec.begin() + index);
}

// Add the contents of [v2] to the end of [v1]
template<typename T> constexpr auto vectorConcat(vector<T>& v1, const vector<T>& v2)
{
	return v1.insert(v1.end(), v2.begin(), v2.end());
}
} // namespace slade

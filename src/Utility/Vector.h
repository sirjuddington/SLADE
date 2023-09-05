#pragma once

namespace slade
{
// Check if a value exists in a vector
template<typename TVec, typename TVal> constexpr auto vectorContains(TVec vec, TVal val)
{
	return find(vec.begin(), vec.end(), val) != (vec).end();
}

// Add a value to a vector if the value doesn't already exist in the vector
template<typename TVec, typename TVal> constexpr auto vectorAddUnique(TVec vec, TVal val)
{
	if (!vectorContains(vec, val))
		vec.push_back(val);
}

// Remove the first item with a given value from a vector
template<typename TVec, typename TVal> constexpr auto vectorRemoveVal(TVec vec, TVal val)
{
	return vec.erase(find(vec.begin(), vec.end(), val));
}

// Remove an item at the given index from a vector
template<typename TVec> constexpr auto vectorRemoveAt(TVec vec, unsigned index)
{
	return vec.erase(vec.begin() + index);
}

// Add the contents of [v2] to the end of [v1]
template<typename TVec> constexpr auto vectorConcat(TVec v1, TVec v2)
{
	return v1.insert(v1.end(), v2.begin(), v2.end());
}
} // namespace slade

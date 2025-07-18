#pragma once

#include <nlohmann/json.hpp>
using json         = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

namespace slade
{
using Json = ordered_json;
}

namespace slade::jsonutil
{
Json parse(string_view json);
Json parse(const MemChunk& mc);

Json parseFile(string_view path);
Json parseFile(const SFile& file);

bool writeFile(const Json& j, string_view path, int indent = 2);

// Sets [target] to the value of [key] in [j] if it exists, otherwise does nothing
template<typename J, typename T> void getIf(const J& j, string_view key, T& target)
{
	if (j.contains(key))
		target = j.at(key);
}
} // namespace slade::jsonutil

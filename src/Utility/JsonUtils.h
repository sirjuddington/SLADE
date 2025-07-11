#pragma once

#include <nlohmann/json.hpp>
using json         = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

namespace slade::jsonutil
{
json parse(string_view json);
json parse(const MemChunk& mc);

ordered_json parseOrdered(string_view json);
ordered_json parseOrdered(const MemChunk& mc);

json parseFile(string_view path);
json parseFile(const SFile& file);

ordered_json parseFileOrdered(string_view path);
ordered_json parseFileOrdered(const SFile& file);

bool writeFile(const json& j, string_view path);
bool writeFile(const ordered_json& j, string_view path);
} // namespace slade::jsonutil

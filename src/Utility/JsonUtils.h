#pragma once

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace slade::jsonutil
{
json parse(string_view json);
json parse(const MemChunk& mc);
json parseFile(string_view path);
json parseFile(const SFile& file);

bool writeFile(const json& j, string_view path);
} // namespace slade::jsonutil

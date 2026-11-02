#pragma once

#include <nlohmann/json_fwd.hpp>

namespace slade
{
using Json = nlohmann::ordered_json; // Use ordered_json by default
}

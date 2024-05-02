#pragma once

namespace slade
{
using Property    = std::variant<bool, int, unsigned int, double, string>;
using PropertyMap = std::map<string, Property, std::less<>>;
} // namespace slade

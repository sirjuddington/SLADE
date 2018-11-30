#pragma once

namespace CodePages
{
string fromASCII(uint8_t val);
string fromCP437(uint8_t val);
rgba_t ansiColor(uint8_t val);
}; // namespace CodePages

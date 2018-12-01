#pragma once

namespace CodePages
{
string fromASCII(uint8_t val);
string fromCP437(uint8_t val);
ColRGBA ansiColor(uint8_t val);
}; // namespace CodePages

#pragma once

namespace slade
{
struct ColRGBA;
}

namespace slade::codepages
{
wxString fromASCII(uint8_t val);
wxString fromCP437(uint8_t val);
ColRGBA  ansiColor(uint8_t val);
}; // namespace slade::codepages

#pragma once

namespace slade::codepages
{
string   fromASCII(uint8_t val);
wxString fromCP437(uint8_t val);
ColRGBA  ansiColor(uint8_t val);
}; // namespace slade::codepages

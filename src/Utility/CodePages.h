#pragma once

#include "Colour.h"

namespace CodePages
{
wxString fromASCII(uint8_t val);
wxString fromCP437(uint8_t val);
ColRGBA  ansiColor(uint8_t val);
}; // namespace CodePages

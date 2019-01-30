#pragma once

#include "Colour.h"

namespace CIE
{
double CIE76(const ColLAB& col1, const ColLAB& col2);
double CIE94(const ColLAB& col1, const ColLAB& col2);
double CIEDE2000(const ColLAB& col1, const ColLAB& col2);
} // namespace CIE

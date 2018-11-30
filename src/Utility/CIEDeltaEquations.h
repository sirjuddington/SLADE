#pragma once

namespace CIE
{
double CIE76(lab_t& col1, lab_t& col2);
double CIE94(lab_t& col1, lab_t& col2);
double CIEDE2000(lab_t& col1, lab_t& col2);
} // namespace CIE

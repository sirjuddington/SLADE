
#ifndef CIEDELTAEQ_H
#define	CIEDELTAEQ_H

#include "Main.h"

namespace CIE {
	double CIE76 (lab_t& col1, lab_t& col2);
	double CIE94 (lab_t& col1, lab_t& col2);
	double CIEDE2000(lab_t& col1, lab_t& col2);
}

#endif//CIEDELTAEQ_H

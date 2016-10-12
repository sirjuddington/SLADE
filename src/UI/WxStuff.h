
#ifndef __WXSTUFF_H__
#define __WXSTUFF_H__

#undef MIN
#undef MAX

#include "common.h"

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

// Some misc wx-related functions
wxMenuItem* createMenuItem(wxMenu* menu, int id, string label, string help = wxEmptyString, string icon = wxEmptyString);
wxFont		getMonospaceFont(wxFont base);

#endif //__WXSTUFF_H__

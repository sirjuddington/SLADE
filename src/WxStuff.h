
#ifndef __WXSTUFF_H__
#define __WXSTUFF_H__

#undef MIN
#undef MAX

#ifdef __WXMSW__
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#else
#include <wx/wx.h>
#endif

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

// Some misc wx-related defines
#define WXCOL(rgba) wxColor(rgba.r, rgba.g, rgba.b, rgba.a)

// Some misc wx-related functions
wxMenuItem* createMenuItem(wxMenu* menu, int id, string label, string help = wxEmptyString, string icon = wxEmptyString);

#endif //__WXSTUFF_H__

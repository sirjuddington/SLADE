
#ifndef __OPENGL_PREFS_PANEL_H__
#define __OPENGL_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class NumberTextCtrl;
class OpenGLPrefsPanel : public PrefsPanelBase
{
private:
	wxCheckBox*	    cb_gl_np2;
	wxCheckBox*	    cb_gl_point_sprite;
	wxCheckBox*     cb_gl_use_vbo;
	NumberTextCtrl* ntc_font_size;

public:
	OpenGLPrefsPanel(wxWindow* parent);
	~OpenGLPrefsPanel();

	void	init();
	void	applyPreferences();
};

#endif//__OPENGL_PREFS_PANEL_H__

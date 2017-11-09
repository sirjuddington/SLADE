#pragma once

#include "PrefsPanelBase.h"

class NumberTextCtrl;
class OpenGLPrefsPanel : public PrefsPanelBase
{
public:
	OpenGLPrefsPanel(wxWindow* parent);
	~OpenGLPrefsPanel();

	void	init() override;
	void	applyPreferences() override;

private:
	wxCheckBox*	    cb_gl_np2_;
	wxCheckBox*	    cb_gl_point_sprite_;
	wxCheckBox*     cb_gl_use_vbo_;
	NumberTextCtrl* ntc_font_size_;
	int             last_font_size_;
};

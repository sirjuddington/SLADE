#pragma once

#include "PrefsPanelBase.h"

namespace slade
{
class NumberTextCtrl;

class OpenGLPrefsPanel : public PrefsPanelBase
{
public:
	OpenGLPrefsPanel(wxWindow* parent);
	~OpenGLPrefsPanel() = default;

	void init() override;
	void applyPreferences() override;

private:
	wxCheckBox*     cb_gl_np2_          = nullptr;
	wxCheckBox*     cb_gl_point_sprite_ = nullptr;
	wxCheckBox*     cb_gl_use_vbo_      = nullptr;
	NumberTextCtrl* ntc_font_size_      = nullptr;
	int             last_font_size_     = 0;
};
} // namespace slade

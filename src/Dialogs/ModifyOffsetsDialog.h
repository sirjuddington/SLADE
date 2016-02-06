
#ifndef __MODIFYOFFSETSDIALOG_H__
#define	__MODIFYOFFSETSDIALOG_H__

#include <wx/dialog.h>
#include "UI/WxBasicControls.h"

class ModifyOffsetsDialog : public wxDialog
{
private:
	wxChoice*	combo_aligntype;

	wxTextCtrl*	entry_xoff;
	wxTextCtrl*	entry_yoff;
	wxCheckBox*	cbox_relative;

	wxRadioButton*	opt_set;
	wxRadioButton*	opt_auto;

public:
	ModifyOffsetsDialog();
	~ModifyOffsetsDialog();

	point2_t	getOffset();
	int			getAlignType();
	bool		autoOffset();
	bool		relativeOffset();
	bool		xOffChange();
	bool		yOffChange();
	point2_t	calculateOffsets(int xoff, int yoff, int width, int height);

	// Events
	void		onOptSet(wxCommandEvent& e);
	void		onOptAuto(wxCommandEvent& e);
};

#endif//__MODIFYOFFSETSDIALOG_H__

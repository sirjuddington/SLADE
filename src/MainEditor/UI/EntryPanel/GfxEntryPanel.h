
#ifndef __GFXENTRYPANEL_H__
#define	__GFXENTRYPANEL_H__

#include "EntryPanel.h"
#include "UI/Canvas/GfxCanvas.h"
#include "General/SAction.h"
#include "Graphics/Translation.h"
#include "UI/ColourBox.h"

class GfxEntryPanel : public EntryPanel, public SActionHandler
{
private:
	bool			alph;
	bool			trns;
	bool			image_data_modified;
	GfxCanvas*		gfx_canvas;
	wxSlider*		slider_zoom;
	wxStaticText*	label_current_zoom;
	ColourBox*		cb_colour;
	wxChoice*		choice_offset_type;
	wxChoice*		choice_brush;
	wxSpinCtrl*		spin_xoffset;
	wxSpinCtrl*		spin_yoffset;
	wxCheckBox*		cb_tile;
	wxCheckBox*		cb_arc;
	wxButton*		btn_auto_offset;

	wxButton*		btn_nextimg;
	wxButton*		btn_previmg;
	wxStaticText*	text_curimg;
	int				cur_index;
	Translation		prev_translation;
	bool			offset_changing;
	bool			editing;
	Translation		edit_translation;
	SToolBarButton*	button_brush;
	wxMenu*			menu_brushes;

public:
	GfxEntryPanel(wxWindow* parent);
	~GfxEntryPanel();

	Translation&	prevTranslation() { return prev_translation; }

	bool	loadEntry(ArchiveEntry* entry); // override for EntryPanel::loadEntry
	bool	loadEntry(ArchiveEntry* entry, int index);
	bool	saveEntry();
	void	setupToolbar();
	void	fillBrushMenu(wxMenu* bm);
	void	updateImagePalette();
	int		detectOffsetType();
	void	applyViewType();
	void	refresh();
	void	refreshPanel();
	string	statusString();
	bool	extractAll();

	bool	iconChanger(wxCommandEvent& e);

	// SAction handler
	bool	handleAction(string id);
	bool	fillCustomMenu(wxMenu* custom);

	void	onZoomChanged(wxCommandEvent& e);
	void	onPaintColourChanged(wxEvent& e);
	void	onXOffsetChanged(wxCommandEvent& e);
	void	onYOffsetChanged(wxCommandEvent& e);
	void	onOffsetTypeChanged(wxCommandEvent& e);
	void	onTileChanged(wxCommandEvent& e);
	void	onARCChanged(wxCommandEvent& e);
	void	onGfxOffsetChanged(wxEvent& e);
	void	onGfxPixelsChanged(wxEvent& e);
	void	onBtnNextImg(wxCommandEvent& e);
	void	onBtnPrevImg(wxCommandEvent& e);
	void	onBtnAutoOffset(wxCommandEvent& e);
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

	SImage*	getImage() { if (gfx_canvas) return gfx_canvas->getImage(); else return NULL; }
};

#endif //__GFXENTRYPANEL_H__

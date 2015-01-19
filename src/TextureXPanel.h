
#ifndef __TEXTUREX_PANEL_H__
#define __TEXTUREX_PANEL_H__

#include "TextureXList.h"
#include "TextureEditorPanel.h"
#include "VirtualListView.h"
#include "MainApp.h"

class TextureXEditor;

class TextureXListView : public VirtualListView
{
private:
	TextureXList*	texturex;

protected:
	string	getItemText(long item, long column) const;
	void	updateItemAttr(long item, long column) const;

public:
	TextureXListView(wxWindow* parent, TextureXList* texturex);
	~TextureXListView();

	void	updateList(bool clear = false);
};

class TextureXPanel : public wxPanel, SActionHandler
{
private:
	TextureXList		texturex;
	TextureXEditor*		tx_editor;
	ArchiveEntry*		tx_entry;
	CTexture*			tex_current;
	bool				modified;

	TextureXListView*	list_textures;
	TextureEditorPanel*	texture_editor;
	wxButton*			btn_new_texture;
	wxButton*			btn_remove_texture;
	wxButton*			btn_new_from_patch;
	wxButton*			btn_new_from_file;
	wxButton*			btn_move_up;
	wxButton*			btn_move_down;
	wxStaticText*		label_tx_format;
	wxButton*			btn_save;

public:
	TextureXPanel(wxWindow* parent, TextureXEditor* tx_editor);
	~TextureXPanel();

	TextureXList&	txList() { return texturex; }
	ArchiveEntry*	txEntry() { return tx_entry; }
	bool			isModified() { return modified; }

	bool	openTEXTUREX(ArchiveEntry* texturex);
	bool	saveTEXTUREX();
	void	setPalette(Palette8bit* pal);
	void	applyChanges();

	// Texture operations
	CTexture*	newTextureFromPatch(string name, string patch);
	void		newTexture();
	void		newTextureFromPatch();
	void		newTextureFromFile();
	void		removeTexture();
	void		renameTexture(bool each = false);
	void		exportTexture();
	bool		exportAsPNG(CTexture* texture, string filename, bool force_rgba);
	void		extractTexture();
	bool		modifyOffsets();
	void		moveUp();
	void		moveDown();
	void		sort();
	void		copy();
	void		paste();

	// SAction handler
	bool	handleAction(string id);

	// Events
	void	onTextureListSelect(wxListEvent& e);
	void	onTextureListRightClick(wxListEvent& e);
	void	onTextureListKeyDown(wxKeyEvent& e);
	void	onBtnNewTexture(wxCommandEvent& e);
	void	onBtnNewTextureFromPatch(wxCommandEvent& e);
	void	onBtnNewTextureFromFile(wxCommandEvent& e);
	void	onBtnRemoveTexture(wxCommandEvent& e);
	void	onBtnMoveUp(wxCommandEvent& e);
	void	onBtnMoveDown(wxCommandEvent& e);
	void	onBtnCopy(wxCommandEvent& e);
	void	onBtnPaste(wxCommandEvent& e);
	void	onShow(wxShowEvent& e);
	void	onBtnSave(wxCommandEvent& e);
};

#endif//__TEXTUREX_PANEL_H__

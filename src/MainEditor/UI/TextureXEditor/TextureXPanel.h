
#ifndef __TEXTUREX_PANEL_H__
#define __TEXTUREX_PANEL_H__

#include "common.h"
#include "General/SAction.h"
#include "Graphics/CTexture/TextureXList.h"
#include "UI/Lists/VirtualListView.h"
#include "UI/WxBasicControls.h"

class TextureXEditor;

class TextureXListView : public VirtualListView
{
private:
	TextureXList*	texturex;

protected:
	string	getItemText(long item, long column, long index) const;
	void	updateItemAttr(long item, long column, long index) const;

public:
	TextureXListView(wxWindow* parent, TextureXList* texturex);
	~TextureXListView();

	TextureXList*	txList() { return texturex; }

	void		updateList(bool clear = false);
	static bool	sizeSort(long left, long right);
	void		sortItems();
	void		setFilter(string filter) { filter_text = filter; updateList(); }
	void		applyFilter() override;
};

class UndoManager;
class TextureEditorPanel;
class TextureXPanel : public wxPanel, SActionHandler
{
private:
	TextureXList		texturex;
	TextureXEditor*		tx_editor;
	ArchiveEntry*		tx_entry;
	CTexture*			tex_current;
	bool				modified;
	UndoManager*		undo_manager;

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
	wxTextCtrl*			text_filter;
	wxButton*			btn_clear_filter;

public:
	TextureXPanel(wxWindow* parent, TextureXEditor* tx_editor);
	~TextureXPanel();

	TextureXList&		txList() { return texturex; }
	ArchiveEntry*		txEntry() { return tx_entry; }
	bool				isModified() { return modified; }
	CTexture*			currentTexture() { return tex_current; }
	TextureEditorPanel*	textureEditor() { return texture_editor; }

	bool	openTEXTUREX(ArchiveEntry* texturex);
	bool	saveTEXTUREX();
	void	setPalette(Palette8bit* pal);
	void	applyChanges();
	void	updateTextureList() { list_textures->updateList(); }

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

	// Undo/Redo
	void	onUndo(string undo_action);
	void	onRedo(string undo_action);

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
	void	onTextFilterChanged(wxCommandEvent& e);
	void	onBtnClearFitler(wxCommandEvent& e);
};

#endif//__TEXTUREX_PANEL_H__

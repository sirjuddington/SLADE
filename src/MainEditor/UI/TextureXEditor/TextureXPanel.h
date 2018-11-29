#pragma once

#include "General/SAction.h"
#include "Graphics/CTexture/TextureXList.h"
#include "UI/Lists/VirtualListView.h"

class TextureXEditor;
class TextureEditorPanel;
class UndoManager;

class TextureXListView : public VirtualListView
{
public:
	TextureXListView(wxWindow* parent, TextureXList* texturex);
	~TextureXListView() {}

	TextureXList* txList() { return texturex_; }

	void        updateList(bool clear = false) override;
	static bool sizeSort(long left, long right);
	void        sortItems() override;
	void        setFilter(string filter)
	{
		filter_text_ = filter;
		updateList();
	}
	void applyFilter() override;

protected:
	string getItemText(long item, long column, long index) const override;
	void   updateItemAttr(long item, long column, long index) const override;

private:
	TextureXList* texturex_;
};

class TextureXPanel : public wxPanel, SActionHandler
{
public:
	TextureXPanel(wxWindow* parent, TextureXEditor& tx_editor);
	~TextureXPanel();

	TextureXList&       txList() { return texturex_; }
	ArchiveEntry*       txEntry() { return tx_entry_; }
	bool                isModified() { return modified_; }
	CTexture*           currentTexture() { return tex_current_; }
	TextureEditorPanel* textureEditor() { return texture_editor_; }

	bool openTEXTUREX(ArchiveEntry* texturex);
	bool saveTEXTUREX();
	void setPalette(Palette* pal);
	void applyChanges();
	void updateTextureList() { list_textures_->updateList(); }

	// Texture operations
	CTexture* newTextureFromPatch(string name, string patch);
	void      newTexture();
	void      newTextureFromPatch();
	void      newTextureFromFile();
	void      removeTexture();
	void      renameTexture(bool each = false);
	void      exportTexture();
	bool      exportAsPNG(CTexture* texture, string filename, bool force_rgba);
	void      extractTexture();
	bool      modifyOffsets();
	void      moveUp();
	void      moveDown();
	void      sort();
	void      copy();
	void      paste();

	// Undo/Redo
	void onUndo(string undo_action);
	void onRedo(string undo_action);

	// SAction handler
	bool handleAction(string id) override;

private:
	TextureXList    texturex_;
	TextureXEditor* tx_editor_    = nullptr;
	ArchiveEntry*   tx_entry_     = nullptr;
	CTexture*       tex_current_  = nullptr;
	bool            modified_     = false;
	UndoManager*    undo_manager_ = nullptr;

	// Controls
	TextureXListView*   list_textures_      = nullptr;
	TextureEditorPanel* texture_editor_     = nullptr;
	wxButton*           btn_new_texture_    = nullptr;
	wxButton*           btn_remove_texture_ = nullptr;
	wxButton*           btn_new_from_patch_ = nullptr;
	wxButton*           btn_new_from_file_  = nullptr;
	wxButton*           btn_move_up_        = nullptr;
	wxButton*           btn_move_down_      = nullptr;
	wxStaticText*       label_tx_format_    = nullptr;
	wxButton*           btn_save_           = nullptr;
	wxTextCtrl*         text_filter_        = nullptr;
	wxButton*           btn_clear_filter_   = nullptr;

	// Events
	void onTextureListSelect(wxListEvent& e);
	void onTextureListRightClick(wxListEvent& e);
	void onTextureListKeyDown(wxKeyEvent& e);
	void onTextFilterChanged(wxCommandEvent& e);
	void onBtnClearFitler(wxCommandEvent& e);
};

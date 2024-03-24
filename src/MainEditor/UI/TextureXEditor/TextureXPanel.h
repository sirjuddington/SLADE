#pragma once

#include "General/SAction.h"

namespace slade
{
class CTexture;
class SToolBar;
class TextureEditorPanel;
class TextureXEditor;
class TextureXList;
class TextureXListView;
class UndoManager;

class TextureXPanel : public wxPanel, SActionHandler
{
public:
	TextureXPanel(wxWindow* parent, TextureXEditor& tx_editor);
	~TextureXPanel() override;

	TextureXList&       txList() const { return *texturex_; }
	ArchiveEntry*       txEntry() const { return tx_entry_; }
	bool                isModified() const { return modified_; }
	CTexture*           currentTexture() const { return tex_current_; }
	TextureEditorPanel* textureEditor() const { return texture_editor_; }

	bool openTEXTUREX(ArchiveEntry* entry);
	bool saveTEXTUREX();
	void setPalette(const Palette* pal) const;
	void applyChanges();
	void updateTextureList() const;

	// Texture operations
	unique_ptr<CTexture> newTextureFromPatch(const wxString& name, const wxString& patch);
	void                 newTexture();
	void                 newTextureFromPatch();
	void                 newTextureFromFile();
	void                 removeTexture();
	void                 renameTexture(bool each = false);
	void                 exportTexture();
	bool                 exportAsPNG(CTexture* texture, const wxString& filename, bool force_rgba) const;
	void                 extractTexture();
	bool                 modifyOffsets();
	void                 moveUp();
	void                 moveDown();
	void                 sort();
	void                 copy() const;
	void                 paste();

	// Undo/Redo
	void onUndo(const wxString& undo_action) const;
	void onRedo(const wxString& undo_action) const;

	// SAction handler
	bool handleAction(string_view id) override;

private:
	unique_ptr<TextureXList> texturex_;
	TextureXEditor*          tx_editor_    = nullptr;
	ArchiveEntry*            tx_entry_     = nullptr;
	CTexture*                tex_current_  = nullptr;
	bool                     modified_     = false;
	UndoManager*             undo_manager_ = nullptr;

	// Controls
	TextureXListView*   list_textures_    = nullptr;
	TextureEditorPanel* texture_editor_   = nullptr;
	wxStaticBox*        frame_textures_   = nullptr;
	wxTextCtrl*         text_filter_      = nullptr;
	wxButton*           btn_clear_filter_ = nullptr;
	SToolBar*           toolbar_          = nullptr;

	// Events
	void onTextureListSelect(wxListEvent& e);
	void onTextureListRightClick(wxListEvent& e);
	void onTextureListKeyDown(wxKeyEvent& e);
	void onTextFilterChanged(wxCommandEvent& e);
	void onBtnClearFitler(wxCommandEvent& e);
};
} // namespace slade

#pragma once

#include "General/SAction.h"
#include "Graphics/CTexture/TextureXList.h"
#include "UI/Lists/VirtualListView.h"

namespace slade
{
class TextureXEditor;
class TextureEditorPanel;
class UndoManager;
class SToolBar;

class TextureXListView : public VirtualListView
{
public:
	TextureXListView(wxWindow* parent, TextureXList* texturex);
	~TextureXListView() = default;

	TextureXList* txList() const { return texturex_; }

	void        updateList(bool clear = false) override;
	static bool sizeSort(long left, long right);
	void        sortItems() override;
	void        setFilter(const string& filter)
	{
		filter_text_ = filter;
		updateList();
	}
	void applyFilter() override;

protected:
	string itemText(long item, long column, long index) const override;
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
	ArchiveEntry*       txEntry() const { return tx_entry_; }
	bool                isModified() const { return modified_; }
	CTexture*           currentTexture() const { return tex_current_; }
	TextureEditorPanel* textureEditor() const { return texture_editor_; }

	bool openTEXTUREX(ArchiveEntry* entry);
	bool saveTEXTUREX();
	void setPalette(Palette* pal) const;
	void applyChanges();
	void updateTextureList() const { list_textures_->updateList(); }

	// Texture operations
	unique_ptr<CTexture> newTextureFromPatch(string_view name, string_view patch);
	void                 newTexture();
	void                 newTextureFromPatch();
	void                 newTextureFromFile();
	void                 removeTexture();
	void                 renameTexture(bool each = false);
	void                 exportTexture();
	bool                 exportAsPNG(CTexture* texture, const string& filename, bool force_rgba) const;
	void                 extractTexture();
	bool                 modifyOffsets();
	void                 moveUp();
	void                 moveDown();
	void                 sort();
	void                 copy();
	void                 paste();

	// Undo/Redo
	void onUndo(const string& undo_action) const;
	void onRedo(const string& undo_action) const;

	// SAction handler
	bool handleAction(string_view id) override;

private:
	TextureXList    texturex_;
	TextureXEditor* tx_editor_    = nullptr;
	ArchiveEntry*   tx_entry_     = nullptr;
	CTexture*       tex_current_  = nullptr;
	bool            modified_     = false;
	UndoManager*    undo_manager_ = nullptr;

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

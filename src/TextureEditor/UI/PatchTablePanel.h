#pragma once

#include "General/SActionHandler.h"

// Forward declarations
namespace slade
{
class GfxCanvas;
class SAuiToolBar;
} // namespace slade
namespace slade::texeditor
{
class PatchTableList;
class TextureEditor;
} // namespace slade::texeditor

namespace slade::texeditor
{
class PatchTablePanel : public wxPanel, SActionHandler
{
public:
	PatchTablePanel(wxWindow* parent, TextureEditor& editor);
	~PatchTablePanel() override = default;

	const string& draggingPatch() const { return dragging_patch_; }

private:
	TextureEditor*  editor_           = nullptr;
	PatchTableList* patch_list_       = nullptr;
	SAuiToolBar*    toolbar_          = nullptr;
	GfxCanvas*      preview_          = nullptr;
	wxStaticText*   info_text_        = nullptr;
	wxListBox*      list_in_textures_ = nullptr;
	string          dragging_patch_;

	void updatePatchTablePreview() const;

	bool handleAction(string_view id) override;

	void addPatch();
	void addPatchFromFile();
	void removePatch();
	void changePatch();

	// Events
	void onPatchTableSelectionChanged(wxDataViewEvent& e);
	void onPatchTableBeginDrag(wxDataViewEvent& e);
};
} // namespace slade::texeditor

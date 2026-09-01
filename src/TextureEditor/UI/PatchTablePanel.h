#pragma once

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
class PatchTablePanel : public wxPanel
{
public:
	PatchTablePanel(wxWindow* parent, TextureEditor& editor);
	~PatchTablePanel() override = default;

	const string& draggingPatch() const { return dragging_patch_; }

private:
	TextureEditor*  editor_     = nullptr;
	PatchTableList* patch_list_ = nullptr;
	SAuiToolBar*    toolbar_    = nullptr;
	GfxCanvas*      preview_    = nullptr;
	wxTextCtrl*     info_text_  = nullptr;
	string          dragging_patch_;

	void updatePatchTablePreview() const;

	// Events
	void onPatchTableSelectionChanged(wxDataViewEvent& e);
	void onPatchTableBeginDrag(wxDataViewEvent& e);
};
} // namespace slade::texeditor

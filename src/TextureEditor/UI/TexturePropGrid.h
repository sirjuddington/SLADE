#pragma once

namespace slade
{
class CTexture;
class CTPatch;
} // namespace slade

namespace slade::texeditor
{
class TextureEditor;
class TexturePropGrid : public wxPropertyGrid
{
public:
	TexturePropGrid(wxWindow* parent, const TextureEditor& editor);

	void openTexture(CTexture* texture);
	void openPatches(const vector<CTPatch*>& patches);
	void refreshPatchProperties();

private:
	CTexture*        tex_ = nullptr;
	vector<CTPatch*> patches_;
	vector<unsigned> patch_indices_;

	void updateColouringPropsVisibility();

	void onPropertyChanged(wxPropertyGridEvent& e);
};
} // namespace slade::texeditor

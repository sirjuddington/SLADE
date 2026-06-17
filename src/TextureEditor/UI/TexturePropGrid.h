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
	TexturePropGrid(wxWindow* parent, TextureEditor& editor);

	void textureChanged();
	void patchesChanged();
	void refreshPatchProperties();

private:
	TextureEditor* editor_ = nullptr;

	void updateColouringPropsVisibility();

	void onPropertyChanged(wxPropertyGridEvent& e);
};
} // namespace slade::texeditor

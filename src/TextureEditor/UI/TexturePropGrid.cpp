
#include "Main.h"
#include "TexturePropGrid.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/Translation.h"
#include "MainEditor/MainEditor.h"
#include "TextureEditor/TextureEditor.h"
#include "UI/Dialogs/TranslationEditorDialog.h"

using namespace slade;
using namespace texeditor;


namespace
{
const wxArrayString type_names  = { wxS("Texture"), wxS("WallTexture"), wxS("Flat"), wxS("Sprite"), wxS("Graphic") };
const wxArrayInt    type_values = { static_cast<int>(CTexture::Type::Texture),
									static_cast<int>(CTexture::Type::WallTexture),
									static_cast<int>(CTexture::Type::Flat),
									static_cast<int>(CTexture::Type::Sprite),
									static_cast<int>(CTexture::Type::Graphic) };

const wxArrayString rotation_names  = { wxS("None"), wxS("90°"), wxS("180°"), wxS("270°") };
const wxArrayInt    rotation_values = { 0, 90, 180, 270 };

const wxArrayString alphastyle_names = { wxS("Copy"),      wxS("Translucent"),     wxS("Add"),
										 wxS("Subtract"),  wxS("ReverseSubtract"), wxS("Modulate"),
										 wxS("CopyAlpha"), wxS("CopyNewAlpha"),    wxS("Overlay") };

const wxArrayString colouring_names = { wxS("None"), wxS("Translation"), wxS("Blend"), wxS("Tint") };
} // namespace


namespace
{
class TranslationProperty : public wxStringProperty
{
public:
	TranslationProperty(
		const TextureEditor& editor,
		const wxString&      label = wxPG_LABEL,
		const wxString&      name  = wxPG_LABEL,
		const wxString&      value = wxString()) :
		wxStringProperty(label, name, value),
		editor_{ &editor }
	{
		// Set to text+button editor
		SetEditor(wxPGEditor_TextCtrlAndButton);
	}

	void openPatch(CTexture* texture, int patch_index)
	{
		texture_     = texture;
		patch_index_ = patch_index;

		if (texture_ && patch_index_ >= 0)
		{
			auto patchx = dynamic_cast<CTPatchEx*>(texture_->patch(patch_index_));
			if (patchx && patchx->hasTranslation())
				SetValue(wxString::FromUTF8(patchx->translation()->asText()));
			else
				SetValue(wxEmptyString);
		}
		else
			SetValue(wxEmptyString);
	}

	bool OnEvent(wxPropertyGrid* propgrid, wxWindow* wnd_primary, wxEvent& e) override
	{
		// '...' button clicked
		if (e.GetEventType() == wxEVT_BUTTON)
		{
			// Create patch image
			SImage image(SImage::Type::PalMask);
			texture_->loadPatchImage(patch_index_, image, editor_->archive(), maineditor::currentPalette());

			// Build translation from current value
			Translation trans;
			if (!GetValueAsString().empty())
				trans.parse(GetValueAsString().utf8_string());

			// Add palette range if no translation ranges exist
			if (trans.nRanges() == 0)
				trans.addRange(TransRange::Type::Palette, 0);

			// Open translation editor dialog
			TranslationEditorDialog ted(
				maineditor::windowWx(), *maineditor::currentPalette(), "Edit Translation", &image);
			ted.openTranslation(trans);
			if (ted.ShowModal() == wxID_OK)
			{
				SetValue(wxString::FromUTF8(ted.getTranslation().asText()));

				// Send changed event (SetValue does not)
				wxPropertyGridEvent evt(wxEVT_PG_CHANGED, GetGrid()->GetId());
				evt.SetProperty(this);
				evt.SetPropertyValue(GetValue());
				GetGrid()->GetEventHandler()->ProcessEvent(evt);

				return true;
			}
		}

		return wxStringProperty::OnEvent(propgrid, wnd_primary, e);
	}

private:
	const TextureEditor* editor_;
	CTexture*            texture_     = nullptr;
	int                  patch_index_ = -1;
};
} // namespace


namespace
{
wxPGProperty* createUIntSpinProp(const string& label, const string& name, int step = 1)
{
	auto prop = new wxUIntProperty(wxString::FromUTF8(label), wxString::FromUTF8(name), 0);
	prop->SetEditor(new wxPGSpinCtrlEditor());
	prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, step);
	return prop;
}

wxPGProperty* createIntSpinProp(
	const string& label,
	const string& name,
	int           step = 1,
	int           min  = std::numeric_limits<int>::lowest(),
	int           max  = std::numeric_limits<int>::max())
{
	auto prop = new wxIntProperty(wxString::FromUTF8(label), wxString::FromUTF8(name), 0);
	prop->SetEditor(new wxPGSpinCtrlEditor());
	prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, step);
	prop->SetAttribute(wxPG_ATTR_MIN, min);
	prop->SetAttribute(wxPG_ATTR_MAX, max);
	return prop;
}

wxPGProperty* createDoubleSpinProp(
	const string& label,
	const string& name,
	double        step = 0.1,
	double        min  = std::numeric_limits<double>::lowest(),
	double        max  = std::numeric_limits<double>::max())
{
	auto prop = new wxFloatProperty(wxString::FromUTF8(label), wxString::FromUTF8(name), 0.0);
	prop->SetEditor(new wxPGSpinCtrlEditor());
	prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, step);
	prop->SetAttribute(wxPG_ATTR_MIN, min);
	prop->SetAttribute(wxPG_ATTR_MAX, max);
	return prop;
}
} // namespace


TexturePropGrid::TexturePropGrid(wxWindow* parent, TextureEditor& editor) : wxPropertyGrid(parent), editor_{ &editor }
{
	// Texture Properties
	Append(new wxPropertyCategory(wxS("Texture Properties"), wxS("texture")));
	Append(createUIntSpinProp("Width", "tex_width"));
	Append(createUIntSpinProp("Height", "tex_height"));
	Append(createUIntSpinProp("X Scale", "tex_scale_x"));
	Append(createUIntSpinProp("Y Scale", "tex_scale_y"));
	Append(createDoubleSpinProp("X Scale", "tex_scale_xd"));
	Append(createDoubleSpinProp("Y Scale", "tex_scale_yd"));
	Append(new wxEnumProperty(wxS("Type"), wxS("tex_type"), type_names, type_values));
	Append(new wxBoolProperty(wxS("World Panning"), wxS("world_panning"), false));
	Append(new wxBoolProperty(wxS("Optional"), wxS("optional"), false));
	Append(new wxBoolProperty(wxS("No Decals"), wxS("no_decals"), false));
	Append(new wxBoolProperty(wxS("Null Texture"), wxS("null_texture"), false));
	Append(new wxBoolProperty(wxS("No Trim"), wxS("no_trim"), false));

	// Patch properties
	Append(new wxPropertyCategory(wxS("Patch Properties"), wxS("patch")));
	Append(createIntSpinProp("X Position", "patch_x", 1, -32768, 32767));
	Append(createIntSpinProp("Y Position", "patch_y", 1, -32768, 32767));
	Append(new wxBoolProperty(wxS("Use Source Offsets"), wxS("patch_use_offsets"), false));
	Append(new wxBoolProperty(wxS("Flip X"), wxS("patch_flip_x"), false));
	Append(new wxBoolProperty(wxS("Flip Y"), wxS("patch_flip_y"), false));
	Append(new wxEnumProperty(wxS("Rotation"), wxS("patch_rotation"), rotation_names, rotation_values));
	Append(createDoubleSpinProp("Alpha", "patch_alpha", 0.1, 0.0, 1.0));
	Append(new wxEnumProperty(wxS("Alpha Style"), wxS("patch_alpha_style"), alphastyle_names));
	Append(new wxEnumProperty(wxS("Colouring"), wxS("patch_colouring"), colouring_names));
	Append(new wxColourProperty(wxS("Colour"), wxS("patch_colour")));
	Append(createDoubleSpinProp("Amount", "patch_tint_amount", 0.1, 0.0, 1.0));
	Append(new TranslationProperty(editor, wxS("Translation"), wxS("patch_translation")));

	// Set all bool properties to use checkboxes
	SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	// Hide all properties initially
	HideProperty(wxS("texture"), true);
	HideProperty(wxS("patch"), true);

	Bind(wxEVT_PG_CHANGED, &TexturePropGrid::onPropertyChanged, this);
}

void TexturePropGrid::textureChanged()
{
	Freeze();

	// Need to clear the selection otherwise any focused editor will remain
	// visible even if the property itself is hidden
	ClearSelection();

	if (auto tex = editor_->currentTexture())
	{
		// Show texture props group
		HideProperty(wxS("texture"), false);
		HideProperty(wxS("patch"), true);

		// Set texture properties visibility
		HideProperty(wxS("tex_type"), !tex->isExtended());
		HideProperty(wxS("optional"), !tex->isExtended());
		HideProperty(wxS("no_decals"), !tex->isExtended());
		HideProperty(wxS("null_texture"), !tex->isExtended());
		HideProperty(wxS("no_trim"), !tex->isExtended());
		HideProperty(wxS("tex_scale_x"), tex->isExtended());
		HideProperty(wxS("tex_scale_y"), tex->isExtended());
		HideProperty(wxS("tex_scale_xd"), !tex->isExtended());
		HideProperty(wxS("tex_scale_yd"), !tex->isExtended());

		// Set basic texture properties
		SetPropertyValue(wxS("tex_width"), tex->width());
		SetPropertyValue(wxS("tex_height"), tex->height());
		SetPropertyValue(wxS("world_panning"), tex->worldPanning());

		// Set extended texture properties
		if (tex->isExtended())
		{
			SetPropertyValue(wxS("tex_scale_xd"), tex->scaleX());
			SetPropertyValue(wxS("tex_scale_yd"), tex->scaleY());
			SetPropertyValue(wxS("tex_type"), static_cast<int>(tex->typeEnum()));
			SetPropertyValue(wxS("optional"), tex->isOptional());
			SetPropertyValue(wxS("no_decals"), tex->noDecals());
			SetPropertyValue(wxS("null_texture"), tex->nullTexture());
			SetPropertyValue(wxS("no_trim"), tex->noTrim());
		}
		else
		{
			// Non-extended textures use a different scale property
			SetPropertyValue(wxS("tex_scale_x"), static_cast<int>(tex->scaleX() * 8));
			SetPropertyValue(wxS("tex_scale_y"), static_cast<int>(tex->scaleY() * 8));
		}
	}
	else
	{
		// No texture loaded, hide all properties
		HideProperty(wxS("texture"), true);
		HideProperty(wxS("patch"), true);
	}

	Thaw();
}

void TexturePropGrid::patchesChanged()
{
	Freeze();

	// Need to clear the selection otherwise any focused editor will remain
	// visible even if the property itself is hidden
	ClearSelection();

	// Setup property visibility
	if (!editor_->selectedPatches().empty())
	{
		auto tex = editor_->currentTexture();
		HideProperty(wxS("patch"), false);
		HideProperty(wxS("patch_use_offsets"), !tex->isExtended());
		HideProperty(wxS("patch_flip_x"), !tex->isExtended());
		HideProperty(wxS("patch_flip_y"), !tex->isExtended());
		HideProperty(wxS("patch_rotation"), !tex->isExtended());
		HideProperty(wxS("patch_alpha"), !tex->isExtended());
		HideProperty(wxS("patch_alpha_style"), !tex->isExtended());
		HideProperty(wxS("patch_colouring"), !tex->isExtended());
		HideProperty(wxS("patch_colour"), true);
		HideProperty(wxS("patch_tint_amount"), true);
		HideProperty(wxS("patch_translation"), true);
	}
	else
		HideProperty(wxS("patch"), true);

	// Load patch properties
	refreshPatchProperties();

	Thaw();
}

void TexturePropGrid::refreshPatchProperties()
{
	if (editor_->selectedPatches().size() == 1)
	{
		auto index = editor_->selectedPatches()[0];
		auto patch = editor_->currentTexture()->patch(index);
		SetPropertyValue(wxS("patch_x"), patch->xOffset());
		SetPropertyValue(wxS("patch_y"), patch->yOffset());
		if (editor_->currentTexture()->isExtended())
		{
			auto     ex_patch = dynamic_cast<CTPatchEx*>(patch);
			wxColour wx_col   = ex_patch->colour();

			// Ensure positive rotation value
			int rotation = ex_patch->rotation() % 360;
			if (rotation < 0)
				rotation += 360;

			SetPropertyValue(wxS("patch_use_offsets"), ex_patch->useOffsets());
			SetPropertyValue(wxS("patch_flip_x"), ex_patch->flipX());
			SetPropertyValue(wxS("patch_flip_y"), ex_patch->flipY());
			SetPropertyValue(wxS("patch_rotation"), rotation);
			SetPropertyValue(wxS("patch_alpha"), ex_patch->alpha());
			SetPropertyValue(wxS("patch_alpha_style"), wxString::FromUTF8(ex_patch->style()));
			SetPropertyValue(wxS("patch_colouring"), static_cast<int>(ex_patch->blendType()));
			SetPropertyValue(wxS("patch_colour"), wx_col);
			SetPropertyValue(wxS("patch_tint_amount"), ex_patch->tintAmount());
			auto trans_prop = dynamic_cast<TranslationProperty*>(GetProperty(wxS("patch_translation")));
			trans_prop->openPatch(editor_->currentTexture(), index);
			updateColouringPropsVisibility();
		}
	}
	else
	{
		// TODO: Set common properties
		SetPropertyValueUnspecified(wxS("patch_x"));
		SetPropertyValueUnspecified(wxS("patch_y"));
		SetPropertyValueUnspecified(wxS("patch_use_offsets"));
		SetPropertyValueUnspecified(wxS("patch_flip_x"));
		SetPropertyValueUnspecified(wxS("patch_flip_y"));
		SetPropertyValueUnspecified(wxS("patch_rotation"));
		SetPropertyValueUnspecified(wxS("patch_alpha"));
		SetPropertyValueUnspecified(wxS("patch_alpha_style"));
		SetPropertyValueUnspecified(wxS("patch_colouring"));
		SetPropertyValueUnspecified(wxS("patch_colour"));
		SetPropertyValueUnspecified(wxS("patch_tint_amount"));
		SetPropertyValueUnspecified(wxS("patch_translation"));
	}
}

void TexturePropGrid::updateColouringPropsVisibility()
{
	switch (GetPropertyValue(wxS("patch_colouring")).GetInteger())
	{
	case 1: // Translation
		HideProperty(wxS("patch_colour"), true);
		HideProperty(wxS("patch_tint_amount"), true);
		HideProperty(wxS("patch_translation"), false);
		break;
	case 2: // Blend
		HideProperty(wxS("patch_colour"), false);
		HideProperty(wxS("patch_tint_amount"), true);
		HideProperty(wxS("patch_translation"), true);
		break;
	case 3: // Tint
		HideProperty(wxS("patch_colour"), false);
		HideProperty(wxS("patch_tint_amount"), false);
		HideProperty(wxS("patch_translation"), true);
		break;
	default: // None / other (invalid)
		HideProperty(wxS("patch_colour"), true);
		HideProperty(wxS("patch_tint_amount"), true);
		HideProperty(wxS("patch_translation"), true);
		break;
	}
}

void TexturePropGrid::onPropertyChanged(wxPropertyGridEvent& e)
{
	auto tex = editor_->currentTexture();
	if (!tex)
		return;

	// Colouring type
	if (e.GetPropertyName() == wxS("patch_colouring"))
	{
		updateColouringPropsVisibility();
		editor_->setPatchBlendType(static_cast<CTPatchEx::BlendType>(e.GetValue().GetInteger()));
	}

	// Texture width
	else if (e.GetPropertyName() == wxS("tex_width"))
		editor_->setTextureSize(e.GetValue().GetInteger(), -1);

	// Texture height
	else if (e.GetPropertyName() == wxS("tex_height"))
		editor_->setTextureSize(-1, e.GetValue().GetInteger());

	// World panning flag
	else if (e.GetPropertyName() == wxS("world_panning"))
		editor_->setTextureFlag("worldpanning", e.GetValue().GetBool());

	// X Scale (non-extended)
	else if (e.GetPropertyName() == wxS("tex_scale_x"))
	{
		auto val = e.GetValue().GetInteger();
		editor_->setTextureScaleX(val == 0 ? 1.0 : val / 8.0);
	}

	// Y Scale (non-extended)
	else if (e.GetPropertyName() == wxS("tex_scale_y"))
	{
		auto val = e.GetValue().GetInteger();
		editor_->setTextureScaleY(val == 0 ? 1.0 : val / 8.0);
	}

	// X Scale (extended)
	else if (e.GetPropertyName() == wxS("tex_scale_xd"))
		editor_->setTextureScaleX(e.GetValue().GetDouble());

	// Y Scale (extended)
	else if (e.GetPropertyName() == wxS("tex_scale_yd"))
		editor_->setTextureScaleY(e.GetValue().GetDouble());

	// Texture type
	else if (e.GetPropertyName() == wxS("tex_type"))
		editor_->setTextureType(static_cast<CTexture::Type>(e.GetValue().GetInteger()));

	// Optional flag
	else if (e.GetPropertyName() == wxS("optional"))
		editor_->setTextureFlag("optional", e.GetValue().GetBool());

	// No decals flag
	else if (e.GetPropertyName() == wxS("no_decals"))
		editor_->setTextureFlag("nodecals", e.GetValue().GetBool());

	// Null texture flag
	else if (e.GetPropertyName() == wxS("null_texture"))
		editor_->setTextureFlag("nulltexture", e.GetValue().GetBool());

	// NoTrim flag
	else if (e.GetPropertyName() == wxS("no_trim"))
		editor_->setTextureFlag("notrim", e.GetValue().GetBool());

	// Patch X position
	else if (e.GetPropertyName() == wxS("patch_x"))
		editor_->setPatchOffsetX(e.GetValue().GetInteger());

	// Patch Y position
	else if (e.GetPropertyName() == wxS("patch_y"))
		editor_->setPatchOffsetY(e.GetValue().GetInteger());

	// Use source offsets
	else if (e.GetPropertyName() == wxS("patch_use_offsets"))
		editor_->setPatchFlag("UseOffsets", e.GetValue().GetBool());

	// Flip X
	else if (e.GetPropertyName() == wxS("patch_flip_x"))
		editor_->setPatchFlag("FlipX", e.GetValue().GetBool());

	// Flip Y
	else if (e.GetPropertyName() == wxS("patch_flip_y"))
		editor_->setPatchFlag("FlipY", e.GetValue().GetBool());

	// Rotation
	else if (e.GetPropertyName() == wxS("patch_rotation"))
		editor_->setPatchRotation(e.GetValue().GetInteger());

	// Alpha
	else if (e.GetPropertyName() == wxS("patch_alpha"))
		editor_->setPatchAlpha(e.GetValue().GetDouble());

	// Alpha style
	else if (e.GetPropertyName() == wxS("patch_alpha_style"))
		editor_->setPatchAlphaStyle(alphastyle_names[e.GetValue().GetInteger()].utf8_string());

	// Colour
	else if (e.GetPropertyName() == wxS("patch_colour"))
	{
		wxColour col;
		col << e.GetPropertyValue();
		editor_->setPatchColour(ColRGBA{ col });
	}

	// Tint amount
	else if (e.GetPropertyName() == wxS("patch_tint_amount"))
		editor_->setPatchTintAmount(e.GetValue().GetDouble());

	// Translation
	else if (e.GetPropertyName() == wxS("patch_translation"))
		editor_->setPatchTranslation(e.GetValue().GetString().utf8_string());

	else
		return; // Unhandled property

	editor_->setTexModified();
}

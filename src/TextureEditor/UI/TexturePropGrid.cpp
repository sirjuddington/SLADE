
#include "Main.h"
#include "TexturePropGrid.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/Translation.h"

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
	int           min  = INT_MIN,
	int           max  = INT_MAX)
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
	double        min  = DBL_MIN,
	double        max  = DBL_MAX)
{
	auto prop = new wxFloatProperty(wxString::FromUTF8(label), wxString::FromUTF8(name), 0.0);
	prop->SetEditor(new wxPGSpinCtrlEditor());
	prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, step);
	prop->SetAttribute(wxPG_ATTR_MIN, min);
	prop->SetAttribute(wxPG_ATTR_MAX, max);
	return prop;
}
} // namespace


TexturePropGrid::TexturePropGrid(wxWindow* parent) : wxPropertyGrid(parent)
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

	// TODO: Custom wxPGProperty for translation
	Append(new wxStringProperty(wxS("Translation"), wxS("patch_translation")));

	// Set all bool properties to use checkboxes
	SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	// Hide all properties initially
	HideProperty(wxS("texture"), true);
	HideProperty(wxS("patch"), true);

	Bind(wxEVT_PG_CHANGED, &TexturePropGrid::onPropertyChanged, this);
}

void TexturePropGrid::openTexture(CTexture* texture)
{
	tex_ = texture;
	patches_.clear();

	Freeze();

	// Need to clear the selection otherwise any focused editor will remain
	// visible even if the property itself is hidden
	ClearSelection();

	if (tex_)
	{
		// Show texture props group
		HideProperty(wxS("texture"), false);
		HideProperty(wxS("patch"), true);

		// Set texture properties visibility
		HideProperty(wxS("tex_type"), !tex_->isExtended());
		HideProperty(wxS("optional"), !tex_->isExtended());
		HideProperty(wxS("no_decals"), !tex_->isExtended());
		HideProperty(wxS("null_texture"), !tex_->isExtended());
		HideProperty(wxS("tex_scale_x"), tex_->isExtended());
		HideProperty(wxS("tex_scale_y"), tex_->isExtended());
		HideProperty(wxS("tex_scale_xd"), !tex_->isExtended());
		HideProperty(wxS("tex_scale_yd"), !tex_->isExtended());

		// Set basic texture properties
		SetPropertyValue(wxS("tex_width"), tex_->width());
		SetPropertyValue(wxS("tex_height"), tex_->height());
		SetPropertyValue(wxS("world_panning"), tex_->worldPanning());

		// Set extended texture properties
		if (tex_->isExtended())
		{
			SetPropertyValue(wxS("tex_scale_xd"), tex_->scaleX());
			SetPropertyValue(wxS("tex_scale_yd"), tex_->scaleY());
			SetPropertyValue(wxS("tex_type"), static_cast<int>(tex_->typeEnum()));
			SetPropertyValue(wxS("optional"), tex_->isOptional());
			SetPropertyValue(wxS("no_decals"), tex_->noDecals());
			SetPropertyValue(wxS("null_texture"), tex_->nullTexture());
		}
		else
		{
			// Non-extended textures use a different scale property
			SetPropertyValue(wxS("tex_scale_x"), static_cast<int>(tex_->scaleX() * 8));
			SetPropertyValue(wxS("tex_scale_y"), static_cast<int>(tex_->scaleY() * 8));
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

void TexturePropGrid::openPatches(const vector<CTPatch*>& patches)
{
	patches_ = patches;

	// Get patch indices for all patches
	patch_indices_.clear();
	for (auto patch : patches)
		patch_indices_.push_back(tex_->patchIndex(patch));

	Freeze();

	// Need to clear the selection otherwise any focused editor will remain
	// visible even if the property itself is hidden
	ClearSelection();

	// Setup property visibility
	if (!patches_.empty())
	{
		HideProperty(wxS("patch"), false);
		HideProperty(wxS("patch_use_offsets"), !tex_->isExtended());
		HideProperty(wxS("patch_flip_x"), !tex_->isExtended());
		HideProperty(wxS("patch_flip_y"), !tex_->isExtended());
		HideProperty(wxS("patch_rotation"), !tex_->isExtended());
		HideProperty(wxS("patch_alpha"), !tex_->isExtended());
		HideProperty(wxS("patch_alpha_style"), !tex_->isExtended());
		HideProperty(wxS("patch_colouring"), !tex_->isExtended());
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
	if (patches_.size() == 1)
	{
		auto patch = patches_[0];
		SetPropertyValue(wxS("patch_x"), patch->xOffset());
		SetPropertyValue(wxS("patch_y"), patch->yOffset());
		if (tex_->isExtended())
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
			SetPropertyValue(
				wxS("patch_translation"),
				ex_patch->hasTranslation() ? wxString::FromUTF8(ex_patch->translation()->asText()) : wxString());
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
	if (!tex_)
		return;

	bool refresh_texture = false;
	bool refresh_patches = false;

	// Colouring type
	if (e.GetPropertyName() == wxS("patch_colouring"))
	{
		updateColouringPropsVisibility();

		for (auto& patch : patches_)
			if (auto ex_patch = dynamic_cast<CTPatchEx*>(patch))
				ex_patch->setBlendType(static_cast<CTPatchEx::BlendType>(e.GetValue().GetInteger()));

		refresh_patches = true;
	}

	// Texture width
	else if (e.GetPropertyName() == wxS("tex_width"))
	{
		tex_->setWidth(e.GetValue().GetInteger());
		refresh_texture = true;
	}

	// Texture height
	else if (e.GetPropertyName() == wxS("tex_height"))
	{
		tex_->setHeight(e.GetValue().GetInteger());
		refresh_texture = true;
	}

	// World panning flag
	else if (e.GetPropertyName() == wxS("world_panning"))
		tex_->setWorldPanning(e.GetValue().GetBool());

	// X Scale (non-extended)
	else if (e.GetPropertyName() == wxS("tex_scale_x"))
	{
		auto val = e.GetValue().GetInteger();
		if (val == 0)
			tex_->setScaleX(1.0);
		else
			tex_->setScaleX(val / 8.0);
	}

	// Y Scale (non-extended)
	else if (e.GetPropertyName() == wxS("tex_scale_y"))
	{
		auto val = e.GetValue().GetInteger();
		if (val == 0)
			tex_->setScaleY(1.0);
		else
			tex_->setScaleY(val / 8.0);
	}

	// X Scale (extended)
	else if (e.GetPropertyName() == wxS("tex_scale_xd"))
		tex_->setScaleX(e.GetValue().GetDouble());

	// Y Scale (extended)
	else if (e.GetPropertyName() == wxS("tex_scale_yd"))
		tex_->setScaleY(e.GetValue().GetDouble());

	// Texture type
	else if (e.GetPropertyName() == wxS("tex_type"))
		tex_->setType(static_cast<CTexture::Type>(e.GetValue().GetInteger()));

	// Optional flag
	else if (e.GetPropertyName() == wxS("optional"))
		tex_->setOptional(e.GetValue().GetBool());

	// No decals flag
	else if (e.GetPropertyName() == wxS("no_decals"))
		tex_->setNoDecals(e.GetValue().GetBool());

	// Null texture flag
	else if (e.GetPropertyName() == wxS("null_texture"))
		tex_->setNullTexture(e.GetValue().GetBool());

	// Patch X position
	else if (e.GetPropertyName() == wxS("patch_x") && !patches_.empty())
	{
		for (auto& patch : patches_)
			patch->setOffsetX(e.GetValue().GetInteger());
		refresh_patches = true;
	}

	// Patch Y position
	else if (e.GetPropertyName() == wxS("patch_y") && !patches_.empty())
	{
		for (auto& patch : patches_)
			patch->setOffsetY(e.GetValue().GetInteger());
		refresh_patches = true;
	}

	// Use source offsets
	else if (e.GetPropertyName() == wxS("patch_use_offsets"))
	{
		for (auto& patch : patches_)
			if (auto ex_patch = dynamic_cast<CTPatchEx*>(patch))
				ex_patch->setUseOffsets(e.GetValue().GetBool());
		refresh_patches = true;
	}

	// Flip X
	else if (e.GetPropertyName() == wxS("patch_flip_x"))
	{
		for (auto& patch : patches_)
			if (auto ex_patch = dynamic_cast<CTPatchEx*>(patch))
				ex_patch->setFlipX(e.GetValue().GetBool());
		refresh_patches = true;
	}

	// Flip Y
	else if (e.GetPropertyName() == wxS("patch_flip_y"))
	{
		for (auto& patch : patches_)
			if (auto ex_patch = dynamic_cast<CTPatchEx*>(patch))
				ex_patch->setFlipY(e.GetValue().GetBool());
		refresh_patches = true;
	}

	// Rotation
	else if (e.GetPropertyName() == wxS("patch_rotation"))
	{
		for (auto& patch : patches_)
			if (auto ex_patch = dynamic_cast<CTPatchEx*>(patch))
				ex_patch->setRotation(e.GetValue().GetInteger());
		refresh_patches = true;
	}

	// Alpha
	else if (e.GetPropertyName() == wxS("patch_alpha"))
	{
		for (auto& patch : patches_)
			if (auto ex_patch = dynamic_cast<CTPatchEx*>(patch))
				ex_patch->setAlpha(e.GetValue().GetDouble());
		refresh_patches = true;
	}

	// Alpha style
	else if (e.GetPropertyName() == wxS("patch_alpha_style"))
	{
		for (auto& patch : patches_)
			if (auto ex_patch = dynamic_cast<CTPatchEx*>(patch))
				ex_patch->setStyle(alphastyle_names[e.GetValue().GetInteger()].utf8_string());
		refresh_patches = true;
	}

	// Colour
	else if (e.GetPropertyName() == wxS("patch_colour"))
	{
		wxColour col;
		col << e.GetPropertyValue();
		for (auto& patch : patches_)
			if (auto ex_patch = dynamic_cast<CTPatchEx*>(patch))
				ex_patch->setColour(ColRGBA{ col });
		refresh_patches = true;
	}

	// Tint amount
	else if (e.GetPropertyName() == wxS("patch_tint_amount"))
	{
		for (auto& patch : patches_)
			if (auto ex_patch = dynamic_cast<CTPatchEx*>(patch))
				ex_patch->setTintAmount(e.GetValue().GetDouble());
		refresh_patches = true;
	}

	// Translation
	else if (e.GetPropertyName() == wxS("patch_translation"))
	{
		Translation t;
		if (!e.GetValue().GetString().empty())
			t.parse(e.GetValue().GetString().utf8_string());

		for (auto& patch : patches_)
			if (auto ex_patch = dynamic_cast<CTPatchEx*>(patch))
				ex_patch->setTranslation(t);

		refresh_patches = true;
	}

	if (refresh_texture)
		tex_->signals().texture_modified();
	if (refresh_patches)
		tex_->signals().patches_modified(patch_indices_);
}

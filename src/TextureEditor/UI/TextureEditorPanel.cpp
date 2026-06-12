
#include "Main.h"
#include "TextureEditorPanel.h"
#include "Archive/Archive.h"
#include "General/KeyBind.h"
#include "General/SAction.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/Translation.h"
#include "MainEditor/MainEditor.h"
#include "OpenGL/View.h"
#include "TextureEditor/TextureEditor.h"
#include "TextureTreeView.h"
#include "UI/Canvas/CTextureCanvasBase.h"
#include "UI/Canvas/Canvas.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Controls/Splitter.h"
#include "UI/Controls/ZoomControl.h"
#include "UI/Layout.h"
#include "UI/SAuiToolBar.h"
#include "UI/State.h"
#include "UI/WxUtils.h"
#include <utility>

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
wxPGProperty* createUIntSpinProp(string_view label, string_view name, int step = 1)
{
	auto prop = new wxUIntProperty(wxString::FromUTF8(label), wxString::FromUTF8(name), 0);
	prop->SetEditor(new wxPGSpinCtrlEditor());
	prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, step);
	return prop;
}

wxPGProperty* createIntSpinProp(string_view label, string_view name, int step = 1)
{
	auto prop = new wxIntProperty(wxString::FromUTF8(label), wxString::FromUTF8(name), 0);
	prop->SetEditor(new wxPGSpinCtrlEditor());
	prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, step);
	return prop;
}

wxPGProperty* createDoubleSpinProp(string_view label, string_view name, double step = 0.1)
{
	auto prop = new wxFloatProperty(wxString::FromUTF8(label), wxString::FromUTF8(name), 0.0);
	prop->SetEditor(new wxPGSpinCtrlEditor());
	prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, step);
	return prop;
}
} // namespace


TextureEditorPanel::TextureEditorPanel(wxWindow* parent, shared_ptr<Archive> archive) : wxPanel(parent, wxID_ANY)
{
	editor_        = std::make_unique<TextureEditor>(archive);
	splitter_left_ = new ui::Splitter(this, -1, wxSP_3DSASH | wxSP_LIVE_UPDATE);

	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Setup left splitter
	auto lh = ui::LayoutHelper(this);
	splitter_left_->SetMinimumPaneSize(FromDIP(300));
	sizer->Add(splitter_left_, lh.sfWithBorder(1, wxTOP | wxBOTTOM).Expand());
	auto split_pos = ui::getStateInt(ui::TEXEDITOR_SPLIT_POS, archive.get());
	splitter_left_->SplitVertically(
		createTextureListPanel(splitter_left_), createMainPanel(splitter_left_), FromDIP(split_pos));

	// Bind Events
	tree_view_->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &TextureEditorPanel::onTextureSelectionChanged, this);
	list_patches_->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &TextureEditorPanel::onPatchSelectionChanged, this);
	tex_canvas_->window()->Bind(wxEVT_LEFT_DOWN, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->window()->Bind(wxEVT_LEFT_DCLICK, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->window()->Bind(wxEVT_LEFT_UP, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->window()->Bind(wxEVT_RIGHT_UP, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->window()->Bind(wxEVT_MOTION, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->window()->Bind(EVT_DRAG_END, &TextureEditorPanel::onTexCanvasDragEnd, this);
	tex_canvas_->window()->Bind(wxEVT_KEY_DOWN, &TextureEditorPanel::onTexCanvasKeyDown, this);

	// Init UI (expandAll must be deferred until the native window exists)
	CallAfter([this]() { tree_view_->expandAll(); });
	updateUI();
}

TextureEditorPanel::~TextureEditorPanel() = default;

wxPanel* TextureEditorPanel::createTextureListPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = ui::LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wxSizerFlags(1).Expand());

	// Toolbar
	toolbar_texlist_ = new SAuiToolBar(panel, true);
	toolbar_texlist_->loadLayoutFromResource("texturex_list");
	hbox->Add(toolbar_texlist_, lh.sfWithSmallBorder(0, wxLEFT | wxRIGHT).Expand());

	// Texture tree
	tree_view_ = new TextureTreeView(panel, *editor_);
	hbox->Add(tree_view_, wxSizerFlags(1).Expand());

	return panel;
}

wxPanel* TextureEditorPanel::createMainPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(sizer);

	// Split (texture view | properties)
	splitter_right_ = new ui::Splitter(panel, -1, wxSP_3DSASH | wxSP_LIVE_UPDATE);
	splitter_right_->SetSashGravity(1.0);
	splitter_right_->SplitVertically(
		createTextureViewPanel(splitter_right_), createPatchPropertiesPanel(splitter_right_), FromDIP(-250));
	sizer->Add(splitter_right_, wxSizerFlags(1).Expand());

	return panel;
}

wxPanel* TextureEditorPanel::createTextureViewPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = ui::LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Top toolbar (nothing yet)

	// Canvas
	tex_canvas_ = ui::createCTextureCanvas(panel);
	tex_canvas_->setPalette(maineditor::currentPalette()); // TODO: Update when main palette is changed
	sizer->Add(tex_canvas_->window(), wxSizerFlags(1).Expand());

	// Bottom toolbar
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithBorder(0, wxTOP).Expand());

	// Offsets
	spin_offset_x_ = new wxSpinCtrl(
		panel,
		-1,
		wxEmptyString,
		wxDefaultPosition,
		wxDefaultSize,
		wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
		SHRT_MIN,
		SHRT_MAX,
		0);
	spin_offset_y_ = new wxSpinCtrl(
		panel,
		-1,
		wxEmptyString,
		wxDefaultPosition,
		wxDefaultSize,
		wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
		SHRT_MIN,
		SHRT_MAX,
		0);
	spin_offset_x_->SetMinSize(lh.spinSize());
	spin_offset_y_->SetMinSize(lh.spinSize());
	hbox->Add(new wxStaticText(panel, -1, wxS("Offsets:")), wxSizerFlags().CenterVertical());
	hbox->Add(spin_offset_x_, lh.sfWithBorder(0, wxLEFT | wxRIGHT).CenterVertical());
	hbox->Add(spin_offset_y_, lh.sfWithBorder(0, wxRIGHT).CenterVertical());

	// Offset view type
	vector<string> offset_types = { "Auto", "Graphic", "Sprite", "HUD" };
	choice_offset_type_         = new wxChoice(
        panel, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(offset_types));
	choice_offset_type_->SetSelection(0);
	hbox->Add(choice_offset_type_, lh.sfWithBorder(0, wxRIGHT).CenterVertical());

	// Auto offset
	btn_auto_offset_ = new SIconButton(panel, "offset", "Modify Offsets...");
	hbox->Add(btn_auto_offset_, wxSizerFlags().CenterVertical());

	hbox->AddStretchSpacer();

	// Zoom
	zc_zoom_ = new ui::ZoomControl(panel, tex_canvas_);
	hbox->Add(zc_zoom_, wxSizerFlags().Expand());

	return panel;
}

wxPanel* TextureEditorPanel::createPatchPropertiesPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = ui::LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Patch list
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(new wxStaticText(panel, wxID_ANY, wxS("Patches")), lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	sizer->Add(hbox, lh.sfWithBorder(0, wxBOTTOM).Expand());
	list_patches_ = new wxDataViewListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE);
	list_patches_->AppendTextColumn(wxS("#"));
	list_patches_->AppendTextColumn(wxS("Name"));
	hbox->Add(list_patches_, wxSizerFlags(1).Expand());

	// Patches toolbar
	toolbar_patches_ = new SAuiToolBar(panel, true);
	toolbar_patches_->loadLayoutFromResource("texturex_patches");
	hbox->Add(toolbar_patches_, lh.sfWithSmallBorder(0, wxLEFT | wxRIGHT).Expand());

	// Texture/Patch properties grid
	pg_properties_ = new wxPropertyGrid(panel);
	setupPropertyGrid();
	sizer->Add(pg_properties_, lh.sfWithBorder(1, wxRIGHT).Expand());

	// Set all bool properties to use checkboxes
	pg_properties_->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);

	return panel;
}

void TextureEditorPanel::setupPropertyGrid() const
{
	// Texture Properties
	pg_properties_->Append(new wxPropertyCategory(wxS("Texture Properties"), wxS("texture")));
	pg_properties_->Append(createUIntSpinProp("Width", "tex_width"));
	pg_properties_->Append(createUIntSpinProp("Height", "tex_height"));
	pg_properties_->Append(createUIntSpinProp("X Scale", "tex_scale_x"));
	pg_properties_->Append(createUIntSpinProp("Y Scale", "tex_scale_y"));
	pg_properties_->Append(createDoubleSpinProp("X Scale", "tex_scale_xd"));
	pg_properties_->Append(createDoubleSpinProp("Y Scale", "tex_scale_yd"));
	pg_properties_->Append(new wxEnumProperty(wxS("Type"), wxS("tex_type"), type_names, type_values))->Hide(true);
	pg_properties_->Append(new wxBoolProperty(wxS("World Panning"), wxS("world_panning"), false));
	pg_properties_->Append(new wxBoolProperty(wxS("Optional"), wxS("optional"), false))->Hide(true);
	pg_properties_->Append(new wxBoolProperty(wxS("No Decals"), wxS("no_decals"), false))->Hide(true);
	pg_properties_->Append(new wxBoolProperty(wxS("Null Texture"), wxS("null_texture"), false))->Hide(true);

	// Patch properties
	pg_properties_->Append(new wxPropertyCategory(wxS("Patch Properties"), wxS("patch")));
	pg_properties_->Append(createIntSpinProp("X Position", "patch_x"));
	pg_properties_->Append(createIntSpinProp("Y Position", "patch_y"));
	pg_properties_->Append(new wxBoolProperty(wxS("Use Source Offsets"), wxS("patch_use_offsets"), false))->Hide(true);
	pg_properties_->Append(new wxBoolProperty(wxS("Flip X"), wxS("patch_flip_x"), false))->Hide(true);
	pg_properties_->Append(new wxBoolProperty(wxS("Flip Y"), wxS("patch_flip_y"), false))->Hide(true);
	pg_properties_->Append(new wxEnumProperty(wxS("Rotation"), wxS("patch_rotation"), rotation_names, rotation_values))
		->Hide(true);
	pg_properties_->Append(createDoubleSpinProp("Alpha", "patch_alpha"))->Hide(true);
	pg_properties_->Append(new wxEnumProperty(wxS("Alpha Style"), wxS("patch_alpha_style"), alphastyle_names))
		->Hide(true);
	pg_properties_->Append(new wxEnumProperty(wxS("Colouring"), wxS("patch_colouring"), colouring_names))->Hide(true);
	pg_properties_->Append(new wxColourProperty(wxS("Colour"), wxS("patch_colour")))->Hide(true);
	pg_properties_->Append(createDoubleSpinProp("Amount", "patch_tint_amount"))->Hide(true);

	// TODO: Custom wxPGProperty for translation
	pg_properties_->Append(new wxStringProperty(wxS("Translation"), wxS("patch_translation")))->Hide(true);
}

void TextureEditorPanel::openTexture(CTexture& tex) const
{
	tex_canvas_->openTexture(&tex, editor_->archive());
	int patch_index = 0;
	for (auto& p : tex.patches())
		list_patches_->AppendItem({ WX_FMT("{}", patch_index++), wxString::FromUTF8(p->name()) });
	list_patches_->GetColumn(0)->SetWidth(list_patches_->GetBestColumnWidth(0));

	// Set basic properties
	pg_properties_->SetPropertyValue(wxS("tex_width"), tex.width());
	pg_properties_->SetPropertyValue(wxS("tex_height"), tex.height());
	pg_properties_->SetPropertyValue(wxS("world_panning"), tex.worldPanning());

	// Set extended properties
	if (tex.isExtended())
	{
		pg_properties_->SetPropertyValue(wxS("tex_scale_xd"), tex.scaleX());
		pg_properties_->SetPropertyValue(wxS("tex_scale_yd"), tex.scaleY());
		pg_properties_->SetPropertyValue(wxS("tex_type"), static_cast<int>(tex.typeEnum()));
		pg_properties_->SetPropertyValue(wxS("optional"), tex.isOptional());
		pg_properties_->SetPropertyValue(wxS("no_decals"), tex.noDecals());
		pg_properties_->SetPropertyValue(wxS("null_texture"), tex.nullTexture());
	}
	else
	{
		pg_properties_->SetPropertyValue(wxS("tex_scale_x"), static_cast<int>(tex.scaleX() * 8));
		pg_properties_->SetPropertyValue(wxS("tex_scale_y"), static_cast<int>(tex.scaleY() * 8));
	}
}

void TextureEditorPanel::updateUI() const
{
	pg_properties_->ClearSelection();

	if (auto tex = tex_canvas_->texture())
	{
		pg_properties_->HideProperty(wxS("texture"), false);
		pg_properties_->HideProperty(wxS("patch"), list_patches_->GetSelectedItemsCount() == 0);
		toolbar_patches_->enableGroup("Patch");

		// Set texture properties visibility
		pg_properties_->HideProperty(wxS("tex_type"), !tex->isExtended());
		pg_properties_->HideProperty(wxS("optional"), !tex->isExtended());
		pg_properties_->HideProperty(wxS("no_decals"), !tex->isExtended());
		pg_properties_->HideProperty(wxS("null_texture"), !tex->isExtended());
		pg_properties_->HideProperty(wxS("tex_scale_x"), tex->isExtended());
		pg_properties_->HideProperty(wxS("tex_scale_y"), tex->isExtended());
		pg_properties_->HideProperty(wxS("tex_scale_xd"), !tex->isExtended());
		pg_properties_->HideProperty(wxS("tex_scale_yd"), !tex->isExtended());

		// Set patch properties visibility
		if (list_patches_->GetSelectedItemsCount() > 0)
		{
			pg_properties_->HideProperty(wxS("patch_use_offsets"), !tex->isExtended());
			pg_properties_->HideProperty(wxS("patch_flip_x"), !tex->isExtended());
			pg_properties_->HideProperty(wxS("patch_flip_y"), !tex->isExtended());
			pg_properties_->HideProperty(wxS("patch_rotation"), !tex->isExtended());
			pg_properties_->HideProperty(wxS("patch_alpha"), !tex->isExtended());
			pg_properties_->HideProperty(wxS("patch_alpha_style"), !tex->isExtended());
			pg_properties_->HideProperty(wxS("patch_colouring"), !tex->isExtended());

			// Colouring properties visibility depends on colouring type
			auto colouring = pg_properties_->GetPropertyValue(wxS("patch_colouring")).GetInteger();
			pg_properties_->HideProperty(wxS("patch_colour"), !(colouring == 2 || colouring == 3));
			pg_properties_->HideProperty(wxS("patch_tint_amount"), colouring != 3);
			pg_properties_->HideProperty(wxS("patch_translation"), colouring != 1);
		}
	}
	else
	{
		pg_properties_->HideProperty(wxS("texture"), true);
		pg_properties_->HideProperty(wxS("patch"), true);
		toolbar_patches_->enableGroup("Patch", false);
	}
}

void TextureEditorPanel::onTextureSelectionChanged(wxDataViewEvent& e)
{
	list_patches_->DeleteAllItems();

	wxDataViewItemArray selection;
	tree_view_->GetSelections(selection);

	pg_properties_->Freeze();

	if (selection.Count() == 1)
	{
		// Single selection, open texture if one is selected
		if (auto ctex = tree_view_->textureForItem(e.GetItem()))
			openTexture(*ctex);
		else
			tex_canvas_->clearTexture();
	}
	else
		tex_canvas_->clearTexture();

	updateUI();

	pg_properties_->Thaw();
}

void TextureEditorPanel::onPatchSelectionChanged(wxDataViewEvent& e)
{
	pg_properties_->Freeze();

	if (list_patches_->GetSelectedItemsCount() == 1)
	{
		// Single patch selected, show properties
		auto sel = list_patches_->GetSelection();
		if (sel.IsOk())
		{
			int index = list_patches_->ItemToRow(sel);
			if (auto patch = tex_canvas_->texture()->patch(index))
			{
				pg_properties_->SetPropertyValue(wxS("patch_x"), patch->xOffset());
				pg_properties_->SetPropertyValue(wxS("patch_y"), patch->yOffset());
				if (tex_canvas_->texture()->isExtended())
				{
					auto ex_patch = dynamic_cast<CTPatchEx*>(patch);
					wxColour wx_col   = ex_patch->colour();
					pg_properties_->SetPropertyValue(wxS("patch_use_offsets"), ex_patch->useOffsets());
					pg_properties_->SetPropertyValue(wxS("patch_flip_x"), ex_patch->flipX());
					pg_properties_->SetPropertyValue(wxS("patch_flip_y"), ex_patch->flipY());
					pg_properties_->SetPropertyValue(wxS("patch_rotation"), ex_patch->rotation());
					pg_properties_->SetPropertyValue(wxS("patch_alpha"), ex_patch->alpha());
					pg_properties_->SetPropertyValue(wxS("patch_alpha_style"), wxString::FromUTF8(ex_patch->style()));
					pg_properties_->SetPropertyValue(wxS("patch_colouring"), static_cast<int>(ex_patch->blendType()));
					pg_properties_->SetPropertyValue(wxS("patch_colour"), wx_col);
					pg_properties_->SetPropertyValue(wxS("patch_tint_amount"), wx_col.Alpha() / 255.0);
					pg_properties_->SetPropertyValue(
						wxS("patch_translation"),
						ex_patch->hasTranslation() ? wxString::FromUTF8(ex_patch->translation()->asText())
												   : wxString());
				}
			}
		}
	}
	else if (list_patches_->GetSelectedItemsCount() > 1)
	{
		// Multiple patches selected
		// TODO: Keep common values
		pg_properties_->GetProperty(wxS("patch_x"))->SetValueToUnspecified();
		pg_properties_->GetProperty(wxS("patch_y"))->SetValueToUnspecified();
	}

	// Update selection on canvas
	for (unsigned i = 0; std::cmp_less(i, list_patches_->GetItemCount()); ++i)
	{
		auto item = list_patches_->RowToItem(i);
		if (item.IsOk() && list_patches_->IsSelected(item))
			tex_canvas_->selectPatch(i);
		else
			tex_canvas_->deSelectPatch(i);
	}
	tex_canvas_->window()->Refresh();

	updateUI();

	pg_properties_->Thaw();
}

// -----------------------------------------------------------------------------
// Called on any mouse event within the texture canvas
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexCanvasMouseEvent(wxMouseEvent& e)
{
	auto tex_current = tex_canvas_->texture();

	// Get mouse position relative to texture
	auto pos = tex_canvas_->view().canvasPos(
		{ e.GetX() * GetContentScaleFactor(), e.GetY() * GetContentScaleFactor() });

	// Get patch that the mouse is over (if any)
	int patch = tex_canvas_->patchAt(pos.x, pos.y);

	// Left click
	if (e.LeftDown() && tex_current)
	{
		if (patch != -1)
		{
			// Clicked on a patch, select it
			// (and clear other selections if shift is not held)
			if (!e.ShiftDown())
				list_patches_->UnselectAll();
			list_patches_->SelectRow(patch);

			// Send selection changed event (SelectRow does not)
			wxDataViewEvent de;
			de.SetEventType(wxEVT_DATAVIEW_SELECTION_CHANGED);
			list_patches_->ProcessWindowEvent(de);
		}
		else
		{
			// Clicked on empty space, deselect all
			list_patches_->UnselectAll();

			// Send selection changed event (UnselectAll does not)
			wxDataViewEvent de;
			de.SetEventType(wxEVT_DATAVIEW_SELECTION_CHANGED);
			list_patches_->ProcessWindowEvent(de);
		}
	}

	// Right click
	if (e.RightUp() && tex_current)
	{
		// Create context menu
		wxMenu popup;
		SAction::fromId("txed_patch_add")->addToMenu(&popup, 1);
		if (list_patches_->GetSelectedItemsCount() > 0)
		{
			SAction::fromId("txed_patch_remove")->addToMenu(&popup, 1);
			SAction::fromId("txed_patch_replace")->addToMenu(&popup, 1);
			SAction::fromId("txed_patch_back")->addToMenu(&popup, 1);
			SAction::fromId("txed_patch_forward")->addToMenu(&popup, 1);
			SAction::fromId("txed_patch_duplicate")->addToMenu(&popup, 1);
		}
		PopupMenu(&popup);
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a mouse drag action ends on the texture canvas
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexCanvasDragEnd(wxCommandEvent& e)
{
	// If patch dragging ended (left button)
	if (e.GetInt() == wxMOUSE_BTN_LEFT)
	{
		// Update patch controls
		// updatePatchControls();
	}
}

// -----------------------------------------------------------------------------
// Called when a key is pressed within the texture canvas
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexCanvasKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	auto binds = KeyBind::bindsForKey(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// TODO: Check for alt key
	// if (e.GetKeyCode() == WXK_ALT)
	//	alt_press_ = true;

	// Go through matching binds
	int  x_movement = 0;
	int  y_movement = 0;
	bool handled    = false;
	for (const auto& name : binds)
	{
		// Move patch left
		if (name == "txed_patch_left")
			x_movement = -1;
		else if (name == "txed_patch_left8")
			x_movement = -8;

		// Move patch up
		else if (name == "txed_patch_up")
			y_movement = -1;
		else if (name == "txed_patch_up8")
			y_movement = -8;

		// Move patch right
		else if (name == "txed_patch_right")
			x_movement = 1;
		else if (name == "txed_patch_right8")
			x_movement = 8;

		// Move patch down
		else if (name == "txed_patch_down")
			y_movement = 1;
		else if (name == "txed_patch_down8")
			y_movement = 8;

		// Add patch
		else if (name == "txed_patch_add")
		{
			// TODO:
			// ignore_drag_ = true;
			// addPatch();
			handled = true;
		}

		// Delete patch
		else if (name == "txed_patch_delete")
		{
			// TODO:
			// removePatch();
			handled = true;
		}

		// Replace patch
		else if (name == "txed_patch_replace")
		{
			// TODO:
			// ignore_drag_ = true;
			// replacePatch();
			handled = true;
		}

		// Duplicate patch
		else if (name == "txed_patch_duplicate")
		{
			// TODO:
			// duplicatePatch();
			handled = true;
		}

		// Bring patch forward
		else if (name == "txed_patch_forward")
		{
			// TODO:
			// patchForward();
			handled = true;
		}

		// Send patch back
		else if (name == "txed_patch_back")
		{
			// TODO:
			// patchBack();
			handled = true;
		}
	}

	// Move patches if needed
	if (x_movement != 0 || y_movement != 0)
	{
		// TODO: Do patch duplicate if alt is pressed
		// if (e.GetModifiers() == wxMOD_ALT && alt_press_)
		// {
		// 	duplicatePatch(0, 0);
		// 	alt_press_ = false;
		// }

		// auto selected_patches = list_patches_->selectedItems();
		wxDataViewItemArray selected_patches;
		list_patches_->GetSelections(selected_patches);
		for (auto item : selected_patches)
		{
			auto patch = tex_canvas_->texture()->patch(list_patches_->ItemToRow(item));
			if (!patch)
				continue;
			int16_t cx = patch->xOffset();
			int16_t cy = patch->yOffset();
			patch->setOffsetX(cx + x_movement);
			patch->setOffsetY(cy + y_movement);
			// TODO: tex_modified_ = true;
		}

		tex_canvas_->redraw(true);
		handled = true;
	}

	// TODO:
	// if (!e.AltDown())
	//	alt_press_ = false;

	if (!handled)
		e.Skip();
}

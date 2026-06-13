
#include "Main.h"
#include "TextureEditorPanel.h"
#include "Archive/Archive.h"
#include "General/KeyBind.h"
#include "General/SAction.h"
#include "Graphics/CTexture/CTexture.h"
#include "MainEditor/MainEditor.h"
#include "OpenGL/View.h"
#include "TextureEditor/TextureEditor.h"
#include "TexturePropGrid.h"
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


TextureEditorPanel::TextureEditorPanel(wxWindow* parent, shared_ptr<Archive> archive) : wxPanel(parent, wxID_ANY)
{
	editor_        = std::make_unique<TextureEditor>(archive);
	splitter_left_ = new ui::Splitter(this, -1, wxSP_3DSASH | wxSP_LIVE_UPDATE);

	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Setup left splitter
	auto lh = ui::LayoutHelper(this);
	splitter_left_->SetMinimumPaneSize(FromDIP(200));
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
	toolbar_patches_->enableGroup("Patch", false);
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
	hbox->Add(tree_view_, lh.sfWithSmallBorder(1, wxRIGHT).Expand());

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
	sizer->Add(tex_canvas_->window(), lh.sfWithSmallBorder(1, wxLEFT | wxRIGHT).Expand());

	// Bottom toolbar
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithBorder(0, wxTOP).Expand());
	hbox->AddSpacer(lh.padSmall());

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
	hbox->Add(zc_zoom_, lh.sfWithSmallBorder(0, wxRIGHT).Expand());

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
	sizer->Add(new wxStaticText(panel, wxID_ANY, wxS("Patches")), lh.sfWithSmallBorder(0, wxBOTTOM | wxLEFT).Expand());
	sizer->Add(hbox, lh.sfWithSmallBorder(0, wxLEFT).Expand());
	sizer->AddSpacer(lh.pad());
	list_patches_ = new wxDataViewListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE);
	list_patches_->AppendTextColumn(wxS("#"));
	list_patches_->AppendTextColumn(wxS("Name"));
	hbox->Add(list_patches_, wxSizerFlags(1).Expand());

	// Patches toolbar
	toolbar_patches_ = new SAuiToolBar(panel, true);
	toolbar_patches_->loadLayoutFromResource("texturex_patches");
	hbox->Add(toolbar_patches_, lh.sfWithSmallBorder(0, wxLEFT | wxRIGHT).Expand());

	// Texture/Patch properties grid
	pg_properties_ = new TexturePropGrid(panel);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithSmallBorder(1, wxLEFT).Expand());
	hbox->Add(pg_properties_, lh.sfWithBorder(1, wxRIGHT).Expand());

	return panel;
}

void TextureEditorPanel::openTexture(CTexture& tex) const
{
	tex_canvas_->openTexture(&tex, editor_->archive());

	int patch_index = 0;
	for (auto& p : tex.patches())
		list_patches_->AppendItem({ WX_FMT("{}", patch_index++), wxString::FromUTF8(p->name()) });
	list_patches_->GetColumn(0)->SetWidth(FromDIP(30));

	pg_properties_->openTexture(&tex);
}

void TextureEditorPanel::clearTexture() const
{
	tex_canvas_->clearTexture();
	list_patches_->DeleteAllItems();
	pg_properties_->openTexture(nullptr);
	toolbar_patches_->enableGroup("Patch", false);
}

void TextureEditorPanel::onTextureSelectionChanged(wxDataViewEvent& e)
{
	list_patches_->DeleteAllItems();

	wxDataViewItemArray selection;
	tree_view_->GetSelections(selection);

	if (selection.Count() == 1)
	{
		// Single selection, open texture if one is selected
		if (auto ctex = tree_view_->textureForItem(e.GetItem()))
			openTexture(*ctex);
		else
			clearTexture();
	}
	else
		clearTexture();

	toolbar_patches_->enableGroup("Patch", false);
}

void TextureEditorPanel::onPatchSelectionChanged(wxDataViewEvent& e)
{
	vector<CTPatch*> selected_patches;
	for (unsigned i = 0; std::cmp_less(i, list_patches_->GetItemCount()); ++i)
	{
		auto item = list_patches_->RowToItem(i);
		if (item.IsOk() && list_patches_->IsSelected(item))
		{
			selected_patches.push_back(tex_canvas_->texture()->patch(i));
			tex_canvas_->selectPatch(i);
		}
		else
			tex_canvas_->deSelectPatch(i);
	}

	pg_properties_->openPatches(selected_patches);
	tex_canvas_->window()->Refresh();
	toolbar_patches_->enableGroup("Patch", !selected_patches.empty());
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

		pg_properties_->refreshPatchProperties();
		tex_canvas_->redraw(true);
		handled = true;
	}

	// TODO:
	// if (!e.AltDown())
	//	alt_press_ = false;

	if (!handled)
		e.Skip();
}

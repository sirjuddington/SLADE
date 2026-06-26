
#include "Main.h"
#include "TextureEditorPanel.h"
#include "Archive/Archive.h"
#include "Archive/EntryType/EntryType.h"
#include "General/KeyBind.h"
#include "General/SAction.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/TextureXList.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MainEditor/UI/TextureXEditor/PatchBrowser.h"
#include "NewTextureDialog.h"
#include "OpenGL/View.h"
#include "TextureEditor/TextureEditor.h"
#include "TexturePropGrid.h"
#include "TextureTreeView.h"
#include "UI/Browser/BrowserItem.h"
#include "UI/Canvas/CTextureCanvasBase.h"
#include "UI/Canvas/Canvas.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Controls/Splitter.h"
#include "UI/Controls/ZoomControl.h"
#include "UI/Layout.h"
#include "UI/SAuiToolBar.h"
#include "UI/State.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"
#include <utility>

using namespace slade;
using namespace texeditor;


TextureEditorPanel::TextureEditorPanel(wxWindow* parent, shared_ptr<Archive> archive) : wxPanel(parent, wxID_ANY)
{
	wxWindowBase::SetName(wxS("texture"));

	editor_        = std::make_unique<TextureEditor>(archive);
	splitter_left_ = new ui::Splitter(this, -1, wxSP_3DSASH | wxSP_LIVE_UPDATE);

	// Create texture menu
	menu_texture_ = new wxMenu();
	SAction::fromId("txed_new")->addToMenu(menu_texture_);
	SAction::fromId("txed_new_file")->addToMenu(menu_texture_);
	SAction::fromId("txed_delete")->addToMenu(menu_texture_);
	menu_texture_->AppendSeparator();
	SAction::fromId("txed_rename")->addToMenu(menu_texture_);
	SAction::fromId("txed_rename_each")->addToMenu(menu_texture_);
	auto menu_export = new wxMenu();
	SAction::fromId("txed_export")->addToMenu(menu_export, true, "Archive (as image)");
	SAction::fromId("txed_extract")->addToMenu(menu_export, true, "File");
	menu_texture_->AppendSubMenu(menu_export, wxS("&Export To"));
	menu_texture_->AppendSeparator();
	SAction::fromId("txed_copy")->addToMenu(menu_texture_);
	SAction::fromId("txed_cut")->addToMenu(menu_texture_);
	SAction::fromId("txed_paste")->addToMenu(menu_texture_);
	menu_texture_->AppendSeparator();
	SAction::fromId("txed_up")->addToMenu(menu_texture_);
	SAction::fromId("txed_down")->addToMenu(menu_texture_);
	SAction::fromId("txed_sort")->addToMenu(menu_texture_);
	auto menu_patch = new wxMenu();
	SAction::fromId("txed_patch_add")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_remove")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_replace")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_back")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_forward")->addToMenu(menu_patch);
	SAction::fromId("txed_patch_duplicate")->addToMenu(menu_patch);
	menu_texture_->AppendSubMenu(menu_patch, wxS("&Patch"));

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
	tree_view_->GetMainWindow()->Bind(wxEVT_KEY_DOWN, &TextureEditorPanel::onTreeViewKeyDown, this);
	list_patches_->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &TextureEditorPanel::onPatchSelectionChanged, this);
	tex_canvas_->window()->Bind(wxEVT_LEFT_DOWN, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->window()->Bind(wxEVT_LEFT_DCLICK, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->window()->Bind(wxEVT_LEFT_UP, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->window()->Bind(wxEVT_RIGHT_UP, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->window()->Bind(wxEVT_MOTION, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->window()->Bind(EVT_DRAG_END, &TextureEditorPanel::onTexCanvasDragEnd, this);
	tex_canvas_->window()->Bind(wxEVT_KEY_DOWN, &TextureEditorPanel::onTexCanvasKeyDown, this);
	spin_offset_x_->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onTexOffsetXChanged, this);
	spin_offset_y_->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onTexOffsetYChanged, this);
	spin_offset_x_->Bind(wxEVT_TEXT_ENTER, &TextureEditorPanel::onTexOffsetXChanged, this);
	spin_offset_y_->Bind(wxEVT_TEXT_ENTER, &TextureEditorPanel::onTexOffsetYChanged, this);
	choice_offset_type_->Bind(wxEVT_CHOICE, &TextureEditorPanel::onChoiceOffsetTypeSelected, this);
	Bind(wxEVT_MENU, &TextureEditorPanel::onToolbarButton, this);

	// Init UI (expandAll must be deferred until the native window exists)
	CallAfter([this]() { tree_view_->expandAll(); });
	updateUI(true);
}

TextureEditorPanel::~TextureEditorPanel()
{
	delete menu_texture_;
}

Archive* TextureEditorPanel::archive() const
{
	return editor_->archive();
}

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
	splitter_right_->SetMinimumPaneSize(FromDIP(200));
	splitter_right_->SplitVertically(
		createTextureViewPanel(splitter_right_), createRightPanel(splitter_right_), FromDIP(-250));
	sizer->Add(splitter_right_, wxSizerFlags(1).Expand());

	return panel;
}

wxPanel* TextureEditorPanel::createTextureViewPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = ui::LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Top toolbar
	toolbar_texture_ = new SAuiToolBar(panel);
	toolbar_texture_->loadLayoutFromResource("texturex_top");
	sizer->Add(toolbar_texture_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());

	// Canvas
	tex_canvas_ = ui::createCTextureCanvas(panel);
	tex_canvas_->setPalette(maineditor::currentPalette()); // TODO: Update when main palette is changed
	sizer->Add(tex_canvas_->window(), lh.sfWithSmallBorder(1, wxLEFT | wxRIGHT).Expand());

	// Bottom toolbar
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithBorder(0, wxTOP).Expand());
	hbox->AddSpacer(lh.padSmall());

	// Offsets
	panel_offsets_ = createOffsetsPanel(panel);
	hbox->Add(panel_offsets_, wxSizerFlags().Expand());

	hbox->AddStretchSpacer();

	// Zoom
	zc_zoom_ = new ui::ZoomControl(panel, tex_canvas_);
	hbox->Add(zc_zoom_, lh.sfWithSmallBorder(0, wxRIGHT).Expand());

	return panel;
}

wxPanel* TextureEditorPanel::createRightPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = ui::LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Setup splitter
	splitter_props_ = new ui::Splitter(panel, -1, wxSP_3DSASH | wxSP_LIVE_UPDATE);
	sizer->Add(splitter_props_, lh.sfWithSmallBorder(1, wxLEFT).Expand());

	// Patch list
	auto patch_list_panel = createPatchListPanel(splitter_props_);

	// We need odd borders around the property grid so create a temp panel
	auto props_panel = new wxPanel(splitter_props_);
	props_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	pg_properties_ = new TexturePropGrid(props_panel, *editor_);
	props_panel->GetSizer()->AddSpacer(lh.padSmall());
	props_panel->GetSizer()->Add(pg_properties_, lh.sfWithBorder(1, wxRIGHT).Expand());

	// Split
	splitter_props_->SplitHorizontally(patch_list_panel, props_panel, patch_list_panel->GetBestSize().y);
	splitter_props_->SetMinimumPaneSize(FromDIP(150));

	return panel;
}

wxPanel* TextureEditorPanel::createPatchListPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = ui::LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Patch list
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(new wxStaticText(panel, wxID_ANY, wxS("Patches")), lh.sfWithSmallBorder(0, wxBOTTOM).Expand());
	sizer->Add(hbox, wxSizerFlags(1).Expand());
	list_patches_ = new wxDataViewListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE);
	list_patches_->AppendTextColumn(wxS("#"));
	list_patches_->AppendTextColumn(wxS("Name"));
	hbox->Add(list_patches_, wxSizerFlags(1).Expand());

	// Patches toolbar
	toolbar_patches_ = new SAuiToolBar(panel, true);
	toolbar_patches_->loadLayoutFromResource("texturex_patches");
	hbox->Add(toolbar_patches_, lh.sfWithSmallBorder(0, wxLEFT | wxRIGHT).Expand());

	sizer->AddSpacer(lh.padSmall());

	return panel;
}

wxPanel* TextureEditorPanel::createOffsetsPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = ui::LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(sizer);

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
	sizer->Add(new wxStaticText(panel, -1, wxS("Offsets:")), wxSizerFlags().CenterVertical());
	sizer->Add(spin_offset_x_, lh.sfWithBorder(0, wxLEFT | wxRIGHT).CenterVertical());
	sizer->Add(spin_offset_y_, lh.sfWithBorder(0, wxRIGHT).CenterVertical());

	// Offset view type
	vector<string> offset_types = { "Auto", "Texture", "Sprite", "HUD" };
	choice_offset_type_         = new wxChoice(
        panel, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(offset_types));
	choice_offset_type_->SetSelection(0);
	sizer->Add(choice_offset_type_, lh.sfWithBorder(0, wxRIGHT).CenterVertical());

	// Auto offset
	btn_auto_offset_ = new SIconButton(panel, "offset", "Modify Offsets...");
	sizer->Add(btn_auto_offset_, wxSizerFlags().CenterVertical());

	return panel;
}

void TextureEditorPanel::updateUI(bool texture_changed)
{
	auto ctex = editor_->currentTexture();

	// No texture open
	if (!ctex)
	{
		if (texture_changed)
		{
			tex_canvas_->clearTexture();
			list_patches_->DeleteAllItems();
			pg_properties_->textureChanged();
		}

		toolbar_patches_->enableGroup("Patch", false);
		toolbar_texture_->showItem("txed_toggle_truecolour", false);
		toolbar_texture_->enableItem("txed_save", false);
		toolbar_texture_->enableItem("revert", false);
		toolbar_patches_->enableItem("txed_patch_add", false);
		panel_offsets_->Show(false);
	}
	else
	{
		if (texture_changed)
		{
			tex_canvas_->openTexture(ctex, editor_->archive());
			pg_properties_->textureChanged();
			populatePatchesList();

			// Update toolbar buttons when texture state changes
			sc_tex_state_changed_ = ctex->signals().state_changed.connect_scoped(
				[this]()
				{
					toolbar_texture_->enableItem(
						"txed_save", editor_->currentTexture()->state() == CTexture::State::Modified);
					toolbar_texture_->enableItem(
						"revert", editor_->currentTexture()->state() == CTexture::State::Modified);
				});
		}

		toolbar_texture_->showItem("txed_toggle_truecolour", ctex->isExtended());
		toolbar_texture_->enableItem("txed_save", ctex->state() == CTexture::State::Modified);
		toolbar_texture_->enableItem("revert", ctex->state() == CTexture::State::Modified);
		toolbar_patches_->enableItem("txed_patch_add", true);
		toolbar_patches_->enableGroup("Patch", !editor_->selectedPatches().empty());
		panel_offsets_->Show(ctex->isExtended());
		panel_offsets_->GetParent()->Layout();
	}

	Refresh();
	tex_canvas_->window()->Refresh();
}

void TextureEditorPanel::populatePatchesList() const
{
	list_patches_->DeleteAllItems();

	auto ctex = editor_->currentTexture();
	if (!ctex)
		return;

	// Add patches to list
	int patch_index = 0;
#if wxCHECK_VERSION(3, 3, 0)
	for (auto& p : ctex->patches())
		list_patches_->AppendItem({ WX_FMT("{}", patch_index++), wxString::FromUTF8(p->name()) });
#else
	for (auto& p : ctex->patches())
	{
		wxVector<wxVariant> data;
		data.push_back(WX_FMT("{}", patch_index++));
		data.push_back(wxString::FromUTF8(p->name()));
		list_patches_->AppendItem(data);
	}
#endif

	// Just set # column to a fixed width
	list_patches_->GetColumn(0)->SetWidth(FromDIP(30));

	// Update patch selection
	list_patches_->SetSelections(wxDataViewItemArray());
	tex_canvas_->deselectAll();
	for (auto i : editor_->selectedPatches())
	{
		list_patches_->SelectRow(i);
		tex_canvas_->selectPatch(i);
	}
}

void TextureEditorPanel::initPatchBrowser()
{
	patch_browser_ = new PatchBrowser(this);
	patch_browser_->setPalette(maineditor::currentPalette());

	if (editor_->currentTexture()->isExtended())
	{
		// TEXTURES, load patches from the archive and resources, and any
		// texture lists in the archive
		patch_browser_->openArchive(editor_->archive());
		for (auto i = 0; i < editor_->nTextureLists(); ++i)
		{
			if (auto tl = editor_->textureList(i))
				patch_browser_->openTextureXList(tl, editor_->archive());
		}
		patch_browser_->setFullPath(true);
	}
	else
	{
		// TEXTUREx, load patches from the patch table
		patch_browser_->openPatchTable(editor_->patchTable());
		patch_browser_->setFullPath(false);
	}
}

string TextureEditorPanel::browsePatch(string_view initial)
{
	// Create patch browser if needed
	if (!patch_browser_)
		initPatchBrowser();

	// Select initial patch if specified
	if (!initial.empty())
		patch_browser_->selectPatch(initial);

	// Open browser and return selected patch name (empty if cancelled)
	if (patch_browser_->ShowModal() == wxID_OK && patch_browser_->selectedItem())
		return patch_browser_->selectedItem()->name();
	else
		return "";
}

void TextureEditorPanel::addPatch()
{
	if (auto patch = browsePatch(); !patch.empty())
	{
		editor_->addPatch(patch);
		updateUI(true);
	}
}

void TextureEditorPanel::removePatch()
{
	editor_->removePatch();
	updateUI(true);
}

void TextureEditorPanel::replacePatch()
{
	// Get first selected patch to use as initial selection in browser
	string initial_patch;
	for (unsigned i : editor_->selectedPatches())
		if (auto p = editor_->currentTexture()->patch(i))
		{
			initial_patch = p->name();
			break;
		}

	if (auto patch = browsePatch(initial_patch); !patch.empty())
	{
		editor_->replacePatch(patch);
		updateUI(true);
	}
}

void TextureEditorPanel::duplicatePatch()
{
	editor_->duplicatePatch(8, 8); // TODO: use grid size
	updateUI(true);
}

void TextureEditorPanel::pushPatch(bool forward)
{
	if (forward)
		editor_->patchForward();
	else
		editor_->patchBack();

	updateUI(true);
}

void TextureEditorPanel::newTexture()
{
	// Determine index to insert new texture at
	int  index         = -1;
	auto last_selected = tree_view_->lastSelectedItem();
	auto ctex          = tree_view_->textureForItem(last_selected);
	auto list          = tree_view_->textureListForItem(last_selected);

	// Do nothing if no texture or texture list is selected
	if (!list)
		return;

	// Insert after selected texture (if any)
	if (ctex)
		index = ctex->index() + 1;

	// Init patch browser if needed
	if (!patch_browser_)
		initPatchBrowser();

	auto dlg = new NewTextureDialog(this, patch_browser_);
	if (dlg->ShowModal() == wxID_OK)
	{
		if (dlg->blankSelected())
			editor_->newTexture(list, dlg->texName(), index, dlg->texWidth(), dlg->texHeight());
		else
			editor_->newTexture(list, dlg->texName(), index, 0, 0, dlg->patch());
	}
}

void TextureEditorPanel::newTextureFromFile()
{
	// Determine index to insert new texture at
	int  index         = -1;
	auto last_selected = tree_view_->lastSelectedItem();
	auto ctex          = tree_view_->textureForItem(last_selected);
	auto list          = tree_view_->textureListForItem(last_selected);

	// Do nothing if no texture or texture list is selected
	if (!list)
		return;

	// Insert after selected texture (if any)
	if (ctex)
		index = ctex->index() + 1;

	// Get all entry types
	auto etypes = EntryType::allTypes();

	// Go through types
	string ext_filter = "All files (*.*)|*|";
	for (auto& etype : etypes)
	{
		// If the type is a valid image type, add its extension filter
		if (etype->extraProps().contains("image"))
		{
			ext_filter += etype->fileFilterString();
			ext_filter += "|";
		}
	}
	if (ext_filter.ends_with('|'))
		ext_filter.pop_back();

	// Popup a file dialog to choose patch file(s)
	auto fd_info = filedialog::openFiles("Choose file(s) to open", ext_filter, this);

	// Run the dialog & check that the user didn't cancel
	if (!fd_info.filenames.empty())
	{
		// Go through file selection
		for (const auto& file : fd_info.filenames)
		{
			if (auto name = editor_->importPatchFile(file, list->format() != TextureXList::Format::Textures);
				!name.empty())
				editor_->newTexture(list, name, index, 0, 0, name);
		}
	}
}

void TextureEditorPanel::deleteTexture() const
{
	for (auto ctex : tree_view_->selectedTextures())
		editor_->deleteTexture(*ctex);
}

void TextureEditorPanel::moveTexture(Direction direction) const
{
	wxDataViewItemArray sel_items;
	tree_view_->GetSelections(sel_items);
	auto selection = tree_view_->selectedTextures();

	// Sort in ascending/descending index order depending on direction
	if (direction == Direction::Up)
		std::ranges::sort(selection, [](const CTexture* a, const CTexture* b) { return a->index() < b->index(); });
	else
		std::ranges::sort(selection, [](const CTexture* a, const CTexture* b) { return a->index() > b->index(); });

	tree_view_->Freeze();

	for (auto ctex : selection)
		editor_->moveTexture(*ctex, direction);

	// Restore selection
	tree_view_->SetSelections(sel_items);
	tree_view_->GetModel()->Resort();

	tree_view_->Thaw();
}

void TextureEditorPanel::sortTextures() const
{
	// Get selected textures
	auto selection = tree_view_->selectedTextures();
	if (selection.empty())
		return;

	auto list = tree_view_->textureListForItem(tree_view_->lastSelectedItem());

	// Without selection of multiple textures, sort everything instead
	if (selection.size() < 2)
	{
		selection.clear();
		selection.resize(list->size());
		for (unsigned i = 0; i < list->size(); ++i)
			selection[i] = list->texture(i);
	}

	// No sorting needed even after adding everything
	if (selection.size() < 2)
		return;

	editor_->sortTextures(selection);

	wxDataViewItemArray items;
	for (auto ctex : selection)
		items.Add(wxDataViewItem(ctex));
	tree_view_->GetModel()->ItemsChanged(items);

	tree_view_->GetModel()->Resort();
}

bool TextureEditorPanel::handleAction(string_view id)
{
	if (id == "txed_new")
		newTexture();

	else if (id == "txed_new_file")
		newTextureFromFile();

	else if (id == "txed_delete")
		deleteTexture();

	else if (id == "txed_up")
		moveTexture(Direction::Up);
	else if (id == "txed_down")
		moveTexture(Direction::Down);

	else if (id == "txed_sort")
		sortTextures();

	else if (id == "txed_save")
	{
		editor_->saveTexture();
		toolbar_texture_->enableItem("txed_save", false);
		toolbar_texture_->enableItem("revert", false);
	}

	else if (id == "txed_toggle_truecolour")
	{
		tex_canvas_->setBlendRGBA(CVar::getBool("tx_truecolour"));
		tex_canvas_->redraw(true);
	}

	else if (id == "txed_apply_scale")
	{
		tex_canvas_->applyTexScale(CVar::getBool("tx_apply_scale"));
		tex_canvas_->redraw();
	}

	else if (id == "txed_arc")
		tex_canvas_->redraw();

	else if (id == "txed_show_outside")
	{
		tex_canvas_->drawOutside(CVar::getBool("tx_show_outside"));
		tex_canvas_->redraw();
	}

	else if (id == "txed_patch_add")
		addPatch();

	else if (id == "txed_patch_remove")
		removePatch();

	else if (id == "txed_patch_replace")
		replacePatch();

	else if (id == "txed_patch_duplicate")
		duplicatePatch();

	else if (id == "txed_patch_forward")
		pushPatch(true);

	else if (id == "txed_patch_back")
		pushPatch(false);

	else
		return false; // Not handled

	return true;
}

void TextureEditorPanel::onTextureSelectionChanged(wxDataViewEvent& e)
{
	wxDataViewItemArray selection;
	tree_view_->GetSelections(selection);
	if (selection.Count() == 1)
	{
		// Single selection, open texture if one is selected
		if (auto ctex = tree_view_->textureForItem(e.GetItem()))
			editor_->openTexture(*ctex);
		else
			editor_->closeTexture();
	}
	else
		editor_->closeTexture();

	updateUI(true);

	e.Skip();
}

void TextureEditorPanel::onPatchSelectionChanged(wxDataViewEvent& e)
{
	// Update patch selection in editor
	for (unsigned i = 0; std::cmp_less(i, list_patches_->GetItemCount()); ++i)
	{
		auto item = list_patches_->RowToItem(i);
		if (item.IsOk() && list_patches_->IsSelected(item))
		{
			editor_->selectPatch(i);
			tex_canvas_->selectPatch(i);
		}
		else
		{
			editor_->selectPatch(i, false);
			tex_canvas_->deselectPatch(i);
		}
	}

	updateUI(false);
	pg_properties_->patchesChanged();
}

void TextureEditorPanel::onTreeViewKeyDown(wxKeyEvent& e)
{
	auto binds = KeyBind::bindsForKey(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	for (const auto& name : binds)
	{
		if (name == "select_all")
		{
			tree_view_->SelectAll();

			// Trigger selection change event (since SelectAll doesn't trigger it)
			wxDataViewEvent de;
			de.SetEventType(wxEVT_DATAVIEW_SELECTION_CHANGED);
			tree_view_->ProcessWindowEvent(de);

			return;
		}

		if (name == "txed_tex_new")
		{
			newTexture();
			return;
		}

		if (name == "txed_tex_new_file")
		{
			newTextureFromFile();
			return;
		}

		if (name == "txed_tex_delete")
		{
			deleteTexture();
			return;
		}

		if (name == "txed_tex_up")
		{
			moveTexture(Direction::Up);
			return;
		}

		if (name == "txed_tex_down")
		{
			moveTexture(Direction::Down);
			return;
		}
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called on any mouse event within the texture canvas
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexCanvasMouseEvent(wxMouseEvent& e)
{
	auto tex_current = editor_->currentTexture();

	// Get mouse position relative to texture
	Vec2i pos        = { tex_canvas_->window()->ToPhys(e.GetX()), tex_canvas_->window()->ToPhys(e.GetY()) };
	auto  canvas_pos = tex_canvas_->view().canvasPos({ pos.x, pos.y });

	// Get patch that the mouse is over (if any)
	int patch = tex_canvas_->patchAt(canvas_pos.x, canvas_pos.y);

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
			list_patches_->EnsureVisible(list_patches_->RowToItem(patch));

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

		tex_canvas_->onMouseEvent(e);
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
		// Move selected patches by the drag amount
		auto drag_offset = tex_canvas_->dragOffset(false);
		if (drag_offset.x != 0 || drag_offset.y != 0)
		{
			editor_->movePatch(drag_offset);
			pg_properties_->refreshPatchProperties();
			tex_canvas_->redraw(true);
		}
	}
}

// -----------------------------------------------------------------------------
// Called when a key is pressed within the texture canvas
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexCanvasKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	auto binds = KeyBind::bindsForKey(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

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
			addPatch();
			handled = true;
		}

		// Delete patch
		else if (name == "txed_patch_delete")
		{
			removePatch();
			handled = true;
		}

		// Replace patch
		else if (name == "txed_patch_replace")
		{
			replacePatch();
			handled = true;
		}

		// Duplicate patch
		else if (name == "txed_patch_duplicate")
		{
			duplicatePatch();
			handled = true;
		}

		// Bring patch forward
		else if (name == "txed_patch_forward")
		{
			pushPatch(true);
			handled = true;
		}

		// Send patch back
		else if (name == "txed_patch_back")
		{
			pushPatch(false);
			handled = true;
		}
	}

	// Move patches if needed
	if (x_movement != 0 || y_movement != 0)
	{
		wxDataViewItemArray selected_patches;
		list_patches_->GetSelections(selected_patches);
		for (auto item : selected_patches)
		{
			auto patch = editor_->currentTexture()->patch(list_patches_->ItemToRow(item));
			if (!patch)
				continue;
			int16_t cx = patch->xOffset();
			int16_t cy = patch->yOffset();
			patch->setOffsetX(cx + x_movement);
			patch->setOffsetY(cy + y_movement);
			editor_->currentTexture()->setState(CTexture::State::Modified);
		}

		pg_properties_->refreshPatchProperties();
		tex_canvas_->redraw(true);
		handled = true;
	}

	if (!handled)
		e.Skip();
}

void TextureEditorPanel::onToolbarButton(wxCommandEvent& e)
{
	string button;
	if (e.GetEventObject() == toolbar_texture_)
		button = toolbar_texture_->actionFromWxId(e.GetId());
	else if (e.GetEventObject() == toolbar_patches_)
		button = toolbar_patches_->actionFromWxId(e.GetId());
	else if (e.GetEventObject() == toolbar_texlist_)
		button = toolbar_texlist_->actionFromWxId(e.GetId());

	if (button == "revert")
	{
		editor_->revertTexture();
		updateUI(true);
	}

	else
		e.Skip();
}

void TextureEditorPanel::onTexOffsetXChanged(wxCommandEvent& e)
{
	editor_->setTextureOffsetX(spin_offset_x_->GetValue());
	tex_canvas_->redraw();
}

void TextureEditorPanel::onTexOffsetYChanged(wxCommandEvent& e)
{
	editor_->setTextureOffsetY(spin_offset_y_->GetValue());
	tex_canvas_->redraw();
}

void TextureEditorPanel::onChoiceOffsetTypeSelected(wxCommandEvent& e)
{
	switch (choice_offset_type_->GetSelection())
	{
	case 0: // Auto
		tex_canvas_->setViewType(tex_canvas_->autoDetectViewType());
		break;
	case 1: // Texture
		tex_canvas_->setViewType(CTextureView::Normal);
		break;
	case 2: // Sprite
		tex_canvas_->setViewType(CTextureView::Sprite);
		break;
	case 3: // HUD
		tex_canvas_->setViewType(CTextureView::HUD);
		break;
	}

	tex_canvas_->redraw();
}

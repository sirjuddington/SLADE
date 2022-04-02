
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextureXPanel.cpp
// Description: The UI for viewing/editing a texture definitions entry
//              (TEXTURE1/2/S)
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "TextureXPanel.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/Clipboard.h"
#include "General/ColourConfiguration.h"
#include "General/KeyBind.h"
#include "General/Misc.h"
#include "General/ResourceManager.h"
#include "General/UI.h"
#include "General/UndoRedo.h"
#include "MainEditor/MainEditor.h"
#include "TextureXEditor.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Dialogs/GfxConvDialog.h"
#include "UI/Dialogs/ModifyOffsetsDialog.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"
#include "Utility/StringUtils.h"
#include "ZTextureEditorPanel.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, dir_last)
EXTERN_CVAR(Bool, wad_force_uppercase)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
bool TxListIsTextures(const TextureXList& tx)
{
	return tx.format() == TextureXList::Format::Textures;
}
} // namespace


// -----------------------------------------------------------------------------
// TextureClipboardItem Class
//
// ClipboardItem for copy+paste of composite textures
// -----------------------------------------------------------------------------
class TextureClipboardItem : public ClipboardItem
{
public:
	TextureClipboardItem(const CTexture& texture, Archive* parent) :
		ClipboardItem(Type::CompositeTexture), texture_{ new CTexture() }
	{
		// Create/copy texture
		texture_->copyTexture(texture);

		// Copy patch entries if possible
		for (unsigned a = 0; a < texture.nPatches(); a++)
		{
			auto entry = texture.patch(a)->patchEntry(parent);

			// FIXME/TODO: Do something to handle patches that are defined in TEXTURES rather than a discrete entry!
			if (!entry)
				continue;

			// Don't copy patch if it has been already
			bool there = false;
			for (auto& patch_entry : patch_entries_)
			{
				if (patch_entry->name() == entry->name())
				{
					there = true;
					break;
				}
			}
			if (there)
				continue;

			// Copy patch entry
			if (entry)
				patch_entries_.emplace_back(new ArchiveEntry(*entry));
		}
	}
	~TextureClipboardItem() override = default;

	const CTexture& texture() const { return *texture_; }

	// -------------------------------------------------------------------------
	// Returns the entry copy for the patch at [index] in the texture
	// -------------------------------------------------------------------------
	ArchiveEntry* patchEntry(const wxString& patch) const
	{
		// Find copied patch entry with matching name
		for (auto& entry : patch_entries_)
		{
			if (strutil::equalCI(strutil::truncate(entry->nameNoExt(), 8), patch.ToStdString()))
				return entry.get();
		}

		// Not found
		return nullptr;
	}

private:
	unique_ptr<CTexture>             texture_;
	vector<unique_ptr<ArchiveEntry>> patch_entries_;
};


// -----------------------------------------------------------------------------
// NewTextureDialog Class
//
// A simple dialog for creating a new texture that allows the user to enter a
// texture name, and either create a blank texture of a specified size, or
// generate one from an existing patch
// -----------------------------------------------------------------------------
class NewTextureDialog : public wxDialog
{
public:
	NewTextureDialog(wxWindow* parent, TextureXEditor* editor, TextureXList* texturex) :
		wxDialog(parent, -1, "New Texture"), editor_{ editor }, texturex_{ texturex }
	{
		wxutil::setWindowIcon(this, "tex_new");

		SetSizer(new wxBoxSizer(wxVERTICAL));
		auto* sizer = new wxGridBagSizer(ui::pad(), ui::pad());
		GetSizer()->Add(sizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, ui::padLarge());

		// Name
		text_name_ = new wxTextCtrl(this, -1);
		sizer->Add(new wxStaticText(this, -1, "Name:"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		sizer->Add(text_name_, { 0, 1 }, { 1, 4 }, wxEXPAND);

		// Blank
		rb_blank_ = new wxRadioButton(this, -1, "Blank");
		rb_blank_->SetValue(true);
		spin_width_  = wxutil::createSpinCtrl(this, 64, 0, 4096);
		spin_height_ = wxutil::createSpinCtrl(this, 128, 0, 4096);
		sizer->Add(rb_blank_, { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		sizer->Add(new wxStaticText(this, -1, "Size:"), { 1, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
		sizer->Add(spin_width_, { 1, 2 }, { 1, 1 }, wxEXPAND);
		sizer->Add(new wxStaticText(this, -1, "x"), { 1, 3 }, { 1, 1 }, wxALIGN_CENTER);
		sizer->Add(spin_height_, { 1, 4 }, { 1, 1 }, wxEXPAND);

		// From Patch
		rb_patch_         = new wxRadioButton(this, -1, "From Patch");
		text_patch_       = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
		btn_browse_patch_ = new wxButton(this, -1, "Browse");
		btn_browse_patch_->Enable(false);
		sizer->Add(rb_patch_, { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		sizer->Add(text_patch_, { 2, 1 }, { 1, 3 }, wxEXPAND);
		sizer->Add(btn_browse_patch_, { 2, 4 }, { 1, 1 }, wxEXPAND);

		sizer->AddGrowableCol(1, 1);

		// Separator
		GetSizer()->Add(
			new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL),
			0,
			wxEXPAND | wxLEFT | wxRIGHT | wxTOP,
			ui::padLarge());

		// Dialog buttons
		auto* btn_create = new wxButton(this, -1, "Create");
		auto* btn_cancel = new wxButton(this, -1, "Cancel");
		btn_create->SetDefault();
		auto* hbox = new wxBoxSizer(wxHORIZONTAL);
		hbox->AddStretchSpacer(1);
		hbox->Add(btn_create, 0, wxEXPAND | wxRIGHT, ui::pad());
		hbox->Add(btn_cancel, 0, wxEXPAND);
		GetSizer()->Add(hbox, 0, wxEXPAND | wxALL, ui::padLarge());

		// Bind events
		rb_blank_->Bind(
			wxEVT_RADIOBUTTON,
			[this](wxCommandEvent&)
			{
				spin_width_->Enable(rb_blank_->GetValue());
				spin_height_->Enable(rb_blank_->GetValue());
				btn_browse_patch_->Enable(!rb_blank_->GetValue());
			});
		rb_patch_->Bind(
			wxEVT_RADIOBUTTON,
			[this](wxCommandEvent&)
			{
				spin_width_->Enable(rb_blank_->GetValue());
				spin_height_->Enable(rb_blank_->GetValue());
				btn_browse_patch_->Enable(!rb_blank_->GetValue());
			});
		btn_browse_patch_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { browsePatch(); });
		btn_cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
		btn_create->Bind(
			wxEVT_BUTTON,
			[this](wxCommandEvent&)
			{
				if (checkValues())
					EndModal(wxID_OK);
			});

		// Setup dialog size
		SetInitialSize({ ui::scalePx(400), -1 });
		wxWindowBase::Layout();
		wxWindowBase::Fit();
		wxTopLevelWindowBase::SetMinSize(GetBestSize());
		CenterOnParent();
	}

	wxString texName() const { return text_name_->GetValue(); }
	bool     blankSelected() const { return rb_blank_->GetValue(); }
	int      texWidth() const { return spin_width_->GetValue(); }
	int      texHeight() const { return spin_height_->GetValue(); }
	wxString patch() const { return text_patch_->GetValue(); }

private:
	TextureXEditor* editor_   = nullptr;
	TextureXList*   texturex_ = nullptr;

	wxTextCtrl*    text_name_        = nullptr;
	wxRadioButton* rb_blank_         = nullptr;
	wxRadioButton* rb_patch_         = nullptr;
	wxSpinCtrl*    spin_width_       = nullptr;
	wxSpinCtrl*    spin_height_      = nullptr;
	wxTextCtrl*    text_patch_       = nullptr;
	wxButton*      btn_browse_patch_ = nullptr;

	bool checkValues() const
	{
		// Name
		if (text_name_->GetValue().Trim().IsEmpty())
		{
			wxMessageBox("Please enter a texture name", "New Texture");
			return false;
		}

		// Patch
		if (rb_patch_->GetValue() && text_patch_->GetValue().Trim().IsEmpty())
		{
			wxMessageBox("Please select a patch", "New Texture");
			return false;
		}

		return true;
	}

	void browsePatch() const
	{
		// Browse for patch
		wxString patch;
		if (TxListIsTextures(*texturex_))
			patch = editor_->browsePatchEntry();
		else
			patch = editor_->patchTable().patchName(editor_->browsePatchTable());

		if (!patch.IsEmpty())
			text_patch_->SetValue(patch);
	}
};


// -----------------------------------------------------------------------------
//
// TextureXListView Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureXListView class constructor
// -----------------------------------------------------------------------------
TextureXListView::TextureXListView(wxWindow* parent, TextureXList* texturex) :
	VirtualListView{ parent }, texturex_{ texturex }
{
	// Add columns
	InsertColumn(0, "Name");
	InsertColumn(1, "Size");

	// Update
	TextureXListView::updateList();
}

// -----------------------------------------------------------------------------
// Returns the string for [item] at [column]
// -----------------------------------------------------------------------------
wxString TextureXListView::itemText(long item, long column, long index) const
{
	// Check texture list exists
	if (!texturex_)
		return "INVALID INDEX";

	// Check index is ok
	if (index < 0 || (unsigned)index > texturex_->size())
		return "INVALID INDEX";

	// Get associated texture
	auto tex = texturex_->texture(index);

	if (column == 0) // Name column
		return tex->name();
	else if (column == 1) // Size column
		return wxString::Format("%dx%d", tex->width(), tex->height());
	else if (column == 2) // Type column
		return tex->type();
	else
		return "INVALID COLUMN";
}

// -----------------------------------------------------------------------------
// Called when widget requests the attributes
// (text colour / background colour / font) for [item]
// -----------------------------------------------------------------------------
void TextureXListView::updateItemAttr(long item, long column, long index) const
{
	// Check texture list exists
	if (!texturex_)
		return;

	// Check index is ok
	if (index < 0 || (unsigned)index > texturex_->size())
		return;

	// Get associated texture
	auto tex = texturex_->texture(index);

	// Init attributes
	item_attr_->SetTextColour(colourconfig::colour("error").toWx());

	// If texture doesn't exist, return error colour
	if (!tex)
		return;

	// Set colour depending on entry state
	switch (tex->state())
	{
	case 1: item_attr_->SetTextColour(colourconfig::colour("modified").toWx()); break;
	case 2: item_attr_->SetTextColour(colourconfig::colour("new").toWx()); break;
	default: item_attr_->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT)); break;
	};
}

// -----------------------------------------------------------------------------
// Clears the list if [clear] is true, and refreshes it
// -----------------------------------------------------------------------------
void TextureXListView::updateList(bool clear)
{
	if (clear)
		ClearAll();

	// Set list size
	items_.clear();
	if (texturex_)
	{
		unsigned count = texturex_->size();
		for (unsigned a = 0; a < count; a++)
			items_.push_back(a);
		applyFilter();
		SetItemCount(items_.size());
	}
	else
		SetItemCount(0);

	sortItems();
	updateWidth();
	Refresh();
}

// -----------------------------------------------------------------------------
// Returns true if texture at index [left] is smaller than [right]
// -----------------------------------------------------------------------------
bool TextureXListView::sizeSort(long left, long right)
{
	auto tl = dynamic_cast<TextureXListView*>(lv_current_)->txList()->texture(left);
	auto tr = dynamic_cast<TextureXListView*>(lv_current_)->txList()->texture(right);
	int  s1 = tl->width() * tl->height();
	int  s2 = tr->width() * tr->height();

	if (s1 == s2)
		return left < right;
	else
		return lv_current_->sortDescend() ? s1 > s2 : s2 > s1;
}

// -----------------------------------------------------------------------------
// Sorts the list items depending on the current sorting column
// -----------------------------------------------------------------------------
void TextureXListView::sortItems()
{
	lv_current_ = this;
	if (sort_column_ == 1)
		std::sort(items_.begin(), items_.end(), &TextureXListView::sizeSort);
	else
		std::sort(items_.begin(), items_.end(), &VirtualListView::defaultSort);
}

// -----------------------------------------------------------------------------
// Filters items by the current filter text string
// -----------------------------------------------------------------------------
void TextureXListView::applyFilter()
{
	// Show all if no filter
	if (filter_text_.IsEmpty())
		return;

	// Split filter by ,
	auto terms = wxSplit(filter_text_, ',');

	// Process filter strings
	for (auto& term : terms)
	{
		// Remove spaces
		term.Replace(" ", "");

		// Set to lowercase and add * to the end
		if (!term.IsEmpty())
			term = term.Lower() + "*";
	}

	// Go through filtered list
	for (unsigned a = 0; a < items_.size(); a++)
	{
		auto tex = texturex_->texture(items_[a]);

		// Check for name match with filter
		bool match = false;
		for (auto& term : terms)
		{
			if (strutil::matchesCI(tex->name(), wxutil::strToView(term)))
			{
				match = true;
				break;
			}
		}
		if (match)
			continue;

		// No match, remove from filtered list
		items_.erase(items_.begin() + a);
		a--;
	}
}


// -----------------------------------------------------------------------------
//
// Undo Steps
//
// -----------------------------------------------------------------------------

class TextureSwapUS : public UndoStep
{
public:
	TextureSwapUS(TextureXList& texturex, int index1, int index2) :
		texturex_(texturex), index1_(index1), index2_(index2)
	{
	}
	~TextureSwapUS() override = default;

	bool doSwap() const
	{
		// Get parent dir
		texturex_.swapTextures(index1_, index2_);
		return true;
	}

	bool doUndo() override { return doSwap(); }
	bool doRedo() override { return doSwap(); }

private:
	TextureXList& texturex_;
	int           index1_ = -1;
	int           index2_ = -1;
};

class TextureCreateDeleteUS : public UndoStep
{
public:
	// Texture Created
	TextureCreateDeleteUS(TextureXPanel* tx_panel, int created_index) : tx_panel_{ tx_panel }, index_{ created_index }
	{
	}

	// Texture Deleted
	TextureCreateDeleteUS(TextureXPanel* tx_panel, unique_ptr<CTexture> tex_removed, int removed_index) :
		tx_panel_{ tx_panel }, tex_removed_{ std::move(tex_removed) }, index_{ removed_index }, created_{ false }
	{
	}

	~TextureCreateDeleteUS() override = default;

	bool deleteTexture()
	{
		tex_removed_ = tx_panel_->txList().removeTexture(index_);
		if (tx_panel_->currentTexture() == tex_removed_.get())
			tx_panel_->textureEditor()->clearTexture();
		return true;
	}

	bool createTexture()
	{
		tx_panel_->txList().addTexture(std::move(tex_removed_), index_);
		return true;
	}

	bool doUndo() override
	{
		if (created_)
			return deleteTexture();
		else
			return createTexture();
	}

	bool doRedo() override
	{
		if (!created_)
			return deleteTexture();
		else
			return createTexture();
	}

private:
	TextureXPanel*       tx_panel_ = nullptr;
	unique_ptr<CTexture> tex_removed_;
	int                  index_   = -1;
	bool                 created_ = true;
};

class TextureModificationUS : public UndoStep
{
public:
	TextureModificationUS(TextureXPanel* tx_panel, const CTexture& texture) :
		tx_panel_{ tx_panel },
		tex_copy_{ std::make_unique<CTexture>() },
		index_{ tx_panel->txList().textureIndex(tex_copy_->name()) }
	{
		tex_copy_->copyTexture(texture);
		tex_copy_->setState(texture.state());
	}

	~TextureModificationUS() override = default;

	bool swapData()
	{
		auto replaced = tx_panel_->txList().replaceTexture(index_, std::move(tex_copy_));
		if (replaced)
		{
			if (tx_panel_->currentTexture() == replaced.get() || tx_panel_->currentTexture() == tex_copy_.get())
				tx_panel_->textureEditor()->openTexture(tex_copy_.get(), &(tx_panel_->txList()));
			tex_copy_ = std::move(replaced);
		}
		else
			return false;

		return true;
	}

	bool doUndo() override { return swapData(); }
	bool doRedo() override { return swapData(); }

private:
	TextureXPanel*       tx_panel_ = nullptr;
	unique_ptr<CTexture> tex_copy_;
	int                  index_ = -1;
};


// -----------------------------------------------------------------------------
//
// TextureXPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureXPanel class constructor
// -----------------------------------------------------------------------------
TextureXPanel::TextureXPanel(wxWindow* parent, TextureXEditor& tx_editor) :
	wxPanel{ parent, -1 }, tx_editor_{ &tx_editor }, undo_manager_{ tx_editor.undoManager() }
{
	// Setup sizer
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Frame
	frame_textures_ = new wxStaticBox(this, -1, "Textures");
	auto framesizer = new wxStaticBoxSizer(frame_textures_, wxHORIZONTAL);
	sizer->Add(framesizer, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, ui::pad());

	// Toolbar
	toolbar_ = new SToolBar(this, false, wxVERTICAL);
	toolbar_->addActionGroup("_Save", { "txed_savelist" });
	toolbar_->addActionGroup("_New", { "txed_new", "txed_new_file" });
	toolbar_->addActionGroup("_Texture", { "txed_rename", "txed_rename_each", "txed_delete" });
	toolbar_->addActionGroup("_Sorting", { "txed_up", "txed_down", "txed_sort" });
	toolbar_->group("_Texture")->setAllButtonsEnabled(false);
	toolbar_->group("_Sorting")->setAllButtonsEnabled(false);
	toolbar_->findActionButton("txed_sort")->Enable();
	framesizer->Add(toolbar_, 0, wxEXPAND | wxTOP | wxBOTTOM, ui::px(ui::Size::PadMinimum));

	// Textures list + filter
	list_textures_    = new TextureXListView(this, &texturex_);
	text_filter_      = new wxTextCtrl(this, -1);
	btn_clear_filter_ = new SIconButton(this, "close", "Clear Filter");
	auto* vbox        = new wxBoxSizer(wxVERTICAL);
	framesizer->AddSpacer(ui::px(ui::Size::PadMinimum));
	framesizer->Add(vbox, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, ui::pad());
	vbox->Add(list_textures_, 1, wxEXPAND | wxBOTTOM, ui::pad());
	vbox->Add(
		wxutil::layoutHorizontally({ wxutil::createLabelHBox(this, "Filter:", text_filter_), btn_clear_filter_ }, 0),
		0,
		wxEXPAND);

	// Bind events
	list_textures_->Bind(wxEVT_LIST_ITEM_SELECTED, &TextureXPanel::onTextureListSelect, this);
	list_textures_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &TextureXPanel::onTextureListRightClick, this);
	list_textures_->Bind(wxEVT_KEY_DOWN, &TextureXPanel::onTextureListKeyDown, this);
	Bind(wxEVT_SHOW, [&](wxShowEvent&) { tx_editor_->updateMenuStatus(); });
	text_filter_->Bind(wxEVT_TEXT, &TextureXPanel::onTextFilterChanged, this);
	btn_clear_filter_->Bind(wxEVT_BUTTON, &TextureXPanel::onBtnClearFitler, this);
}

// -----------------------------------------------------------------------------
// TextureXPanel class destructor
// -----------------------------------------------------------------------------
TextureXPanel::~TextureXPanel()
{
	if (tx_entry_)
		tx_entry_->unlock();
}

// -----------------------------------------------------------------------------
// Loads a TEXTUREx or TEXTURES format texture list into the editor
// -----------------------------------------------------------------------------
bool TextureXPanel::openTEXTUREX(ArchiveEntry* entry)
{
	// Open texture list (check format)
	if (entry->type()->formatId() == "texturex")
	{
		// TEXTURE1/2 format
		if (!texturex_.readTEXTUREXData(entry, tx_editor_->patchTable()))
			return false;

		// Create default texture editor
		texture_editor_ = new TextureEditorPanel(this, tx_editor_);

		// Update patch table usage info
		for (size_t a = 0; a < texturex_.size(); a++)
		{
			auto tex = texturex_.texture(a);

			// Go through texture's patches
			for (size_t p = 0; p < tex->nPatches(); p++)
				tx_editor_->patchTable().patch(tex->patch(p)->name()).used_in.push_back(tex->name());
		}
	}
	else
	{
		// TEXTURES format
		if (!texturex_.readTEXTURESData(entry))
			return false;

		// Create extended texture editor
		texture_editor_ = new ZTextureEditorPanel(this, tx_editor_);

		// Add 'type' column
		list_textures_->InsertColumn(2, "Type");
	}

	tx_entry_ = entry;

	// Add texture editor area
	GetSizer()->Add(texture_editor_, 1, wxEXPAND | wxALL, ui::pad());
	texture_editor_->setupLayout();

	// Update format label
	frame_textures_->SetLabel("Textures (" + texturex_.textureXFormatString() + " format)");

	// Update texture list
	list_textures_->updateList();
	Layout();
	Update();

	return true;
}

// -----------------------------------------------------------------------------
// Saves a TEXTUREX format texture list
// -----------------------------------------------------------------------------
bool TextureXPanel::saveTEXTUREX()
{
	// Save any changes to current texture
	applyChanges();

	// Write list to entry, in the correct format
	tx_entry_->unlock(); // Have to unlock the entry first
	bool ok = false;
	if (TxListIsTextures(texturex_))
		ok = texturex_.writeTEXTURESData(tx_entry_);
	else
		ok = texturex_.writeTEXTUREXData(tx_entry_, tx_editor_->patchTable());

	// Redetect type and lock it up
	EntryType::detectEntryType(*tx_entry_);
	tx_entry_->lock();

	// Set all textures to unmodified
	for (unsigned a = 0; a < texturex_.size(); a++)
		texturex_.texture(a)->setState(0);
	list_textures_->updateList();

	// Update variables
	modified_ = false;

	return ok;
}

// -----------------------------------------------------------------------------
// Sets the texture editor's palette
// -----------------------------------------------------------------------------
void TextureXPanel::setPalette(Palette* pal) const
{
	texture_editor_->setPalette(pal);
}

// -----------------------------------------------------------------------------
// Applies changes to the current texture, if any
// -----------------------------------------------------------------------------
void TextureXPanel::applyChanges()
{
	if (texture_editor_->texModified() && tex_current_)
	{
		// Clear undo levels for individual edits to the current texture
		undo_manager_->clearToResetPoint();

		// Create single 'modify texture' undo level
		undo_manager_->beginRecord("Modify Texture");
		undo_manager_->recordUndoStep(std::make_unique<TextureModificationUS>(this, *tex_current_));
		undo_manager_->endRecord(true);

		undo_manager_->setResetPoint();

		if (texture_editor_->texture())
			tex_current_->copyTexture(*texture_editor_->texture());
		tex_current_->setState(1);
		tx_editor_->patchTable().updatePatchUsage(tex_current_);
		list_textures_->updateList();
		modified_ = true;
		texture_editor_->openTexture(tex_current_, &texturex_);
	}
}

// -----------------------------------------------------------------------------
// Creates a new texture called [name] from [patch]. The new texture will be
// set to the dimensions of the patch, with the patch added at 0,0
// -----------------------------------------------------------------------------
unique_ptr<CTexture> TextureXPanel::newTextureFromPatch(const wxString& name, const wxString& patch)
{
	// Create new texture
	auto tex = std::make_unique<CTexture>();
	tex->setName(name.ToStdString());
	tex->setState(2);

	// Setup texture scale
	if (TxListIsTextures(texturex_))
	{
		tex->setScale({ 1., 1. });
		tex->setExtended(true);
	}
	else
		tex->setScale({ 0., 0. });

	// Add patch
	tex->addPatch(patch.ToStdString(), 0, 0);

	// Load patch image (to determine dimensions)
	SImage image;
	tex->loadPatchImage(0, image);

	// Set dimensions
	tex->setWidth(image.width());
	tex->setHeight(image.height());

	// Update variables
	modified_ = true;

	// Return the new texture
	return tex;
}

// -----------------------------------------------------------------------------
// Create a new texture. Shows a dialog allowing the user to either create a
// blank texture, or generate one from a single patch
// -----------------------------------------------------------------------------
void TextureXPanel::newTexture()
{
	auto* dialog = new NewTextureDialog(maineditor::windowWx(), tx_editor_, &texturex_);
	if (dialog->ShowModal() != wxID_OK)
		return;

	// Process name
	auto name = dialog->texName();
	if (!TxListIsTextures(texturex_))
		name = name.Upper().Truncate(8);

	// Create new texture
	unique_ptr<CTexture> tex;
	if (dialog->blankSelected())
	{
		// Blank texture
		tex = std::make_unique<CTexture>();
		tex->setName(wxutil::strToView(name));
		tex->setState(2);

		// Size
		tex->setWidth(dialog->texWidth());
		tex->setHeight(dialog->texHeight());

		// Setup texture scale
		if (TxListIsTextures(texturex_))
		{
			tex->setScale({ 1., 1. });
			tex->setExtended(true);
		}
		else
			tex->setScale({ 0., 0. });
	}
	else // From patch
		tex = newTextureFromPatch(name, dialog->patch());

	// Add it after the last selected item
	auto* tex_ptr  = tex.get();
	int   selected = list_textures_->itemIndex(list_textures_->lastSelected());
	if (selected == -1)
		selected = texturex_.size() - 1; // Add to end of the list if nothing selected
	texturex_.addTexture(std::move(tex), selected + 1);

	// Record undo level
	undo_manager_->beginRecord("New Texture");
	undo_manager_->recordUndoStep(std::make_unique<TextureCreateDeleteUS>(this, selected + 1));
	undo_manager_->endRecord(true);

	// Update texture list
	list_textures_->updateList();

	// Select the new texture
	list_textures_->clearSelection();
	list_textures_->selectItem(selected + 1);
	list_textures_->EnsureVisible(selected + 1);

	// Update patch table counts
	if (dialog->blankSelected())
		tx_editor_->patchTable().updatePatchUsage(tex_ptr);

	// Update variables
	modified_ = true;
}

// -----------------------------------------------------------------------------
// Creates a new texture from an existing patch
// -----------------------------------------------------------------------------
void TextureXPanel::newTextureFromPatch()
{
	// Browse for patch
	wxString patch;
	if (TxListIsTextures(texturex_))
		patch = tx_editor_->browsePatchEntry();
	else
		patch = tx_editor_->patchTable().patchName(tx_editor_->browsePatchTable());

	if (!patch.IsEmpty())
	{
		// Prompt for new texture name
		wxString name = wxGetTextFromUser("Enter a texture name:", "New Texture", patch);

		// Do nothing if no name entered
		if (name.IsEmpty())
			return;

		// Process name
		name = name.Upper().Truncate(8);

		// Create new texture from patch
		auto tex     = newTextureFromPatch(name, patch);
		auto tex_ptr = tex.get();

		// Add texture after the last selected item
		int selected = list_textures_->itemIndex(list_textures_->lastSelected());
		if (selected == -1)
			selected = texturex_.size() - 1; // Add to end of the list if nothing selected
		texturex_.addTexture(std::move(tex), selected + 1);

		// Record undo level
		undo_manager_->beginRecord("New Texture from Patch");
		undo_manager_->recordUndoStep(std::make_unique<TextureCreateDeleteUS>(this, selected + 1));
		undo_manager_->endRecord(true);

		// Update texture list
		list_textures_->updateList();

		// Select the new texture
		list_textures_->clearSelection();
		list_textures_->selectItem(selected + 1);
		list_textures_->EnsureVisible(selected + 1);

		// Update patch table counts
		tx_editor_->patchTable().updatePatchUsage(tex_ptr);
	}
}

// -----------------------------------------------------------------------------
// Creates a new texture from an image file. The file will be imported and added
// to the patch table if needed
// -----------------------------------------------------------------------------
void TextureXPanel::newTextureFromFile()
{
	// Get all entry types
	auto etypes = EntryType::allTypes();

	// Go through types
	wxString ext_filter = "All files (*.*)|*.*|";
	for (auto& etype : etypes)
	{
		// If the type is a valid image type, add its extension filter
		if (etype->extraProps().contains("image"))
		{
			ext_filter += etype->fileFilterString();
			ext_filter += "|";
		}
	}

	// Create open file dialog
	wxFileDialog dialog_open(
		this,
		"Choose file(s) to open",
		dir_last,
		wxEmptyString,
		ext_filter,
		wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST,
		wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Get file selection
		wxArrayString files;
		dialog_open.GetPaths(files);

		// Save 'dir_last'
		dir_last = wxutil::strToView(dialog_open.GetDirectory());

		// Go through file selection
		for (const auto& file : files)
		{
			// Load the file into a temporary ArchiveEntry
			auto entry = std::make_shared<ArchiveEntry>();
			entry->importFile(file.ToStdString());

			// Determine type
			EntryType::detectEntryType(*entry);

			// If it's not a valid image type, ignore this file
			if (!entry->type()->extraProps().contains("image"))
			{
				log::warning(wxString::Format("%s is not a valid image file", file));
				continue;
			}

			// Ask for name for texture
			wxFileName fn(file);
			wxString   name = fn.GetName().Upper().Truncate(8);
			name            = wxGetTextFromUser(
                wxString::Format("Enter a texture name for %s:", fn.GetFullName()), "New Texture", name);
			name = name.Truncate(8);

			// Add patch to archive
			entry->setName(name.ToStdString());
			entry->setExtensionByType();
			tx_entry_->parent()->addEntry(entry, "patches");

			// Add patch to patch table if needed
			if (!TxListIsTextures(texturex_))
				tx_editor_->patchTable().addPatch(wxutil::strToView(name));


			// Create new texture from patch
			auto tex     = newTextureFromPatch(name, name);
			auto tex_ptr = tex.get();

			// Add texture after the last selected item
			int selected = list_textures_->itemIndex(list_textures_->lastSelected());
			if (selected == -1)
				selected = texturex_.size() - 1; // Add to end of the list if nothing selected
			texturex_.addTexture(std::move(tex), selected + 1);

			// Record undo level
			undo_manager_->beginRecord("New Texture from File");
			undo_manager_->recordUndoStep(std::make_unique<TextureCreateDeleteUS>(this, selected + 1));
			undo_manager_->endRecord(true);

			// Update texture list
			list_textures_->updateList();

			// Select the new texture
			list_textures_->clearSelection();
			list_textures_->selectItem(selected + 1);
			list_textures_->EnsureVisible(selected + 1);

			// Update patch table counts
			tx_editor_->patchTable().updatePatchUsage(tex_ptr);
		}
	}
}

// -----------------------------------------------------------------------------
// Removes any selected textures
// -----------------------------------------------------------------------------
void TextureXPanel::removeTexture()
{
	// Get selected textures
	auto selection = list_textures_->selection(true);

	// Begin recording undo level
	undo_manager_->beginRecord("Remove Texture(s)");

	// Go through selection backwards
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Remove texture from patch table entries
		auto tex = texturex_.texture(selection[a]);
		for (unsigned p = 0; p < tex->nPatches(); p++)
			tx_editor_->patchTable().patch(tex->patch(p)->name()).removeTextureUsage(tex->name());

		// Remove texture from list
		auto removed = texturex_.removeTexture(selection[a]);

		// Record undo step
		undo_manager_->recordUndoStep(std::make_unique<TextureCreateDeleteUS>(this, std::move(removed), selection[a]));
	}

	// End recording undo level
	undo_manager_->endRecord(true);

	// Clear selection & refresh
	list_textures_->clearSelection();
	list_textures_->updateList();
	texture_editor_->clearTexture();

	// Update variables
	modified_ = true;
}

// -----------------------------------------------------------------------------
// Moves all selected textures up
// -----------------------------------------------------------------------------
void TextureXPanel::moveUp()
{
	// Get selected textures
	vector<long> selection = list_textures_->selection(true);

	// Do nothing if the first selected item is at the top of the list
	if (!selection.empty() && selection[0] == 0)
		return;

	// Begin recording undo level
	undo_manager_->beginRecord("Move Texture(s) Up");

	// Go through selection
	for (long index : selection)
	{
		// Swap selected texture with the one above it
		texturex_.swapTextures(index, index - 1);

		// Record undo step
		undo_manager_->recordUndoStep(std::make_unique<TextureSwapUS>(texturex_, index, index - 1));
	}

	// End recording undo level
	undo_manager_->endRecord(true);

	// Update selection
	list_textures_->clearSelection();
	for (long index : selection)
		list_textures_->selectItem(index - 1);

	// Refresh
	list_textures_->updateList();

	// Update variables
	modified_ = true;
}

// -----------------------------------------------------------------------------
// Moves all selected textures down
// -----------------------------------------------------------------------------
void TextureXPanel::moveDown()
{
	// Get selected textures
	auto selection = list_textures_->selection(true);

	// Do nothing if the last selected item is at the end of the list
	if (!selection.empty() && selection.back() == list_textures_->GetItemCount() - 1)
		return;

	// Begin recording undo level
	undo_manager_->beginRecord("Move Texture(s) Down");

	// Go through selection backwards
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Swap selected texture with the one below it
		texturex_.swapTextures(selection[a], selection[a] + 1);

		// Record undo step
		undo_manager_->recordUndoStep(std::make_unique<TextureSwapUS>(texturex_, selection[a], selection[a] + 1));
	}

	// End recording undo level
	undo_manager_->endRecord(true);

	// Update selection
	list_textures_->clearSelection();
	for (long index : selection)
		list_textures_->selectItem(index + 1);

	// Refresh
	list_textures_->updateList();

	// Update variables
	modified_ = true;
}

// -----------------------------------------------------------------------------
// Sorts all selected textures
// -----------------------------------------------------------------------------
void TextureXPanel::sort()
{
	// Get selected textures
	auto selection = list_textures_->selection(true);

	// Without selection of multiple texture, sort everything instead
	if (selection.size() < 2)
	{
		selection.clear();
		selection.resize(texturex_.size());
		for (size_t i = 0; i < texturex_.size(); ++i)
			selection[i] = i;
	}

	// No sorting needed even after adding everything
	if (selection.size() < 2)
		return;

	// Fill a map with <texture name, texture index> pairs
	vector<size_t>             origindex(texturex_.size());
	std::map<wxString, size_t> tmap;
	tmap.clear();
	for (long index : selection)
	{
		// We want to be sure that each key is unique, so we add the position to the name string
		wxString name = wxString::Format("%-8s%8d", texturex_.texture(index)->name(), index);
		// x keeps the current position, while y keeps the original position
		tmap[name]       = index;
		origindex[index] = index;
	}

	// Begin recording undo level
	undo_manager_->beginRecord("Sort Textures");

	// And now, sort the textures based on the map
	std::map<wxString, size_t>::iterator itr = tmap.begin();
	for (long index : selection)
	{
		// If the texture isn't in its sorted place already
		if (index != itr->second)
		{
			// Swap the texture in the spot with the sorted one
			size_t tmp             = origindex[index];
			origindex[index]       = origindex[itr->second];
			origindex[itr->second] = tmp;
			texturex_.swapTextures(index, itr->second);
			undo_manager_->recordUndoStep(std::make_unique<TextureSwapUS>(texturex_, index, itr->second));
			// Update the position of the displaced texture in the tmap
			wxString name = wxString::Format("%-8s%8d", texturex_.texture(itr->second)->name(), tmp);
			tmap[name]    = itr->second;
		}
		++itr;
	}

	// End recording undo level
	undo_manager_->endRecord(true);

	// Refresh
	list_textures_->updateList();

	// Update variables
	modified_ = true;
}

// -----------------------------------------------------------------------------
// Copies any selected textures to the clipboard
// -----------------------------------------------------------------------------
void TextureXPanel::copy()
{
	// Get selected textures
	auto selection = list_textures_->selection(true);

	// Do nothing if nothing selected
	if (selection.empty())
		return;

	// Create list of textures to copy
	vector<unique_ptr<ClipboardItem>> copy_items;
	for (long index : selection)
		copy_items.emplace_back(new TextureClipboardItem(*texturex_.texture(index), tx_editor_->archive()));

	// Add list to clipboard
	app::clipboard().clear();
	app::clipboard().add(copy_items);
}

// -----------------------------------------------------------------------------
// Pastes any textures on the clipboard after the last selected texture
// -----------------------------------------------------------------------------
void TextureXPanel::paste()
{
	// Check there is anything on the clipboard
	if (app::clipboard().empty())
		return;

	// Get last selected index
	int selected = list_textures_->itemIndex(list_textures_->lastSelected());
	if (selected == -1)
		selected = texturex_.size() - 1; // Add to end of the list if nothing selected

	// Begin recording undo level
	undo_manager_->beginRecord("Paste Texture(s)");

	// Go through clipboard items
	for (unsigned a = 0; a < app::clipboard().size(); a++)
	{
		// Skip if not a texture clipboard item
		if (app::clipboard().item(a)->type() != ClipboardItem::Type::CompositeTexture)
			continue;

		// Get texture item
		auto item = dynamic_cast<TextureClipboardItem*>(app::clipboard().item(a));

		// Add new texture after last selected item
		auto ntex     = std::make_unique<CTexture>(TxListIsTextures(texturex_));
		auto ntex_ptr = ntex.get();
		ntex->copyTexture(item->texture(), true);
		ntex->setState(2);
		texturex_.addTexture(std::move(ntex), ++selected);

		// Record undo step
		undo_manager_->recordUndoStep(std::make_unique<TextureCreateDeleteUS>(this, selected));

		// Deal with patches
		for (unsigned p = 0; p < ntex_ptr->nPatches(); p++)
		{
			auto patch = ntex_ptr->patch(p);

			// Update patch table if necessary
			if (!TxListIsTextures(texturex_))
				tx_editor_->patchTable().addPatch(patch->name());

			// Get the entry for this patch
			auto entry = patch->patchEntry(tx_editor_->archive());

			// If the entry wasn't found in any open archive, try copying it from the clipboard
			// (the user may have closed the archive the original patch was in)
			if (!entry)
			{
				entry = item->patchEntry(patch->name());

				// Copy the copied patch entry over to this archive
				if (entry)
					tx_editor_->archive()->addEntry(std::make_shared<ArchiveEntry>(*entry), "patches");
			}

			// If the entry exists in the base resource archive or this archive, do nothing
			else if (
				entry->parent() == app::archiveManager().baseResourceArchive()
				|| entry->parent() == tx_editor_->archive())
				continue;

			// Otherwise, copy the entry over to this archive
			else
				tx_editor_->archive()->addEntry(std::make_shared<ArchiveEntry>(*entry), "patches");
		}
	}

	// End recording undo level
	undo_manager_->endRecord(true);

	// Refresh
	list_textures_->updateList();

	// Update variables
	modified_ = true;
}

// -----------------------------------------------------------------------------
// Create standalone image entries of any selected textures
// -----------------------------------------------------------------------------
void TextureXPanel::renameTexture(bool each)
{
	// Get selected textures
	auto              selec_num = list_textures_->selection(true);
	vector<CTexture*> selection;

	if (!tx_entry_)
		return;

	// Go through selection
	for (long index : selec_num)
		selection.push_back(texturex_.texture(index));

	// Define alphabet
	static const string alphabet       = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	static const string alphabet_lower = "abcdefghijklmnopqrstuvwxyz";

	// Check any are selected
	if (each || selection.size() == 1)
	{
		// If only one entry is selected, or "rename each" mode is desired, just do basic rename
		for (auto& texture : selection)
		{
			// Prompt for a new name
			wxString new_name = wxGetTextFromUser("Enter new texture name: (* = unchanged)", "Rename", texture->name());
			if (wad_force_uppercase)
				new_name.MakeUpper();

			// Rename entry (if needed)
			if (!new_name.IsEmpty() && texture->name() != new_name)
			{
				texture->setName(wxutil::strToView(new_name));
				texture->setState(1);
				if (texture == tex_current_)
					texture_editor_->updateTextureName(new_name);

				modified_ = true;
			}
		}
	}
	else if (selection.size() > 1)
	{
		// Get a list of entry names
		vector<string> names;
		for (auto& texture : selection)
			names.push_back(texture->name());

		// Get filter string
		auto filter = misc::massRenameFilter(names);

		// Prompt for a new name
		auto new_name = wxGetTextFromUser(
							"Enter new texture name: (* = unchanged, ^ = alphabet letter, ^^ = lower case\n"
							"% = alphabet repeat number, & = texture number, %% or && = n-1)",
							"Rename",
							filter)
							.ToStdString();
		if (wad_force_uppercase)
			strutil::upperIP(new_name);

		// Apply mass rename to list of names
		if (!new_name.empty())
		{
			misc::doMassRename(names, new_name);

			// Go through the list
			for (size_t a = 0; a < selection.size(); a++)
			{
				// Rename the entry (if needed)
				if (selection[a]->name() != names[a])
				{
					auto filename = names[a];
					int  num      = a / alphabet.size();
					int  cn       = a - (num * alphabet.size());
					strutil::replaceIP(filename, "^^", { alphabet_lower.data() + cn, 1 });
					strutil::replaceIP(filename, "^", { alphabet.data() + cn, 1 });
					strutil::replaceIP(filename, "%%", fmt::format("{}", num));
					strutil::replaceIP(filename, "%", fmt::format("{}", num + 1));
					strutil::replaceIP(filename, "&&", fmt::format("{}", a));
					strutil::replaceIP(filename, "&", fmt::format("{}", a + 1));

					selection[a]->setName(filename);
					selection[a]->setState(1);
					if (selection[a] == tex_current_)
						texture_editor_->updateTextureName(filename);
					modified_ = true;
				}
			}
		}
	}
	Refresh();
}

// -----------------------------------------------------------------------------
// Create standalone image entries of any selected textures
// -----------------------------------------------------------------------------
void TextureXPanel::exportTexture()
{
	// Get selected textures
	auto              selec_num = list_textures_->selection(true);
	vector<CTexture*> selection;

	if (!tx_entry_)
		return;

	// saveTEXTUREX();

	auto archive    = tx_entry_->parent();
	bool force_rgba = texture_editor_->blendRGBA();

	// Go through selection
	for (long index : selec_num)
		selection.push_back(texturex_.texture(index));

	// Create gfx conversion dialog
	GfxConvDialog gcd(this);

	// Send selection to the gcd
	gcd.openTextures(selection, texture_editor_->palette(), archive, force_rgba);

	// Run the gcd
	gcd.ShowModal();

	// Show splash window
	ui::showSplash("Writing converted image data...", true);

	// Write any changes
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Update splash window
		ui::setSplashProgressMessage(selection[a]->name());
		ui::setSplashProgress((float)a / (float)selection.size());

		// Skip if the image wasn't converted
		if (!gcd.itemModified(a))
			continue;

		// Get image and conversion info
		auto image  = gcd.itemImage(a);
		auto format = gcd.itemFormat(a);

		// Write converted image back to entry
		MemChunk mc;
		format->saveImage(*image, mc, force_rgba ? nullptr : gcd.itemPalette(a));
		auto lump = std::make_shared<ArchiveEntry>();
		lump->importMemChunk(mc);
		lump->rename(selection[a]->name());
		archive->addEntry(lump, "textures");
		EntryType::detectEntryType(*lump);
		lump->setExtensionByType();
	}

	// Hide splash window
	ui::hideSplash();
}

// -----------------------------------------------------------------------------
// Converts [texture] to a PNG image (if possible) and saves the PNG data to a
// file [filename]. Does not alter the texture data itself
// -----------------------------------------------------------------------------
bool TextureXPanel::exportAsPNG(CTexture* texture, const wxString& filename, bool force_rgba) const
{
	// Check entry was given
	if (!texture)
		return false;

	// Create image from entry
	SImage image;
	if (!texture->toImage(image, nullptr, texture_editor_->palette(), force_rgba))
	{
		log::error(wxString::Format("Error converting %s: %s", texture->name(), global::error));
		return false;
	}

	// Write png data
	MemChunk png;
	auto     fmt_png = SIFormat::getFormat("png");
	if (!fmt_png->saveImage(image, png, texture_editor_->palette()))
	{
		log::error(wxString::Format("Error converting %s", texture->name()));
		return false;
	}

	// Export file
	return png.exportFile(filename.ToStdString());
}

// -----------------------------------------------------------------------------
// Create standalone image entries of any selected textures
// -----------------------------------------------------------------------------
void TextureXPanel::extractTexture()
{
	// Get selected textures
	auto              selec_num = list_textures_->selection(true);
	vector<CTexture*> selection;

	if (!tx_entry_)
		return;

	auto archive    = tx_entry_->parent();
	bool force_rgba = texture_editor_->blendRGBA();

	// Go through selection
	for (long index : selec_num)
		selection.push_back(texturex_.texture(index));


	// If we're just exporting one texture
	if (selection.size() == 1)
	{
		auto          name = misc::lumpNameToFileName(selection[0]->name());
		strutil::Path fn(name);

		// Set extension
		fn.setExtension("png");

		// Run save file dialog
		filedialog::FDInfo info;
		if (filedialog::saveFile(
				info,
				"Export Texture \"" + selection[0]->name() + "\" as PNG",
				"PNG Files (*.png)|*.png",
				this,
				fn.fileName()))
		{
			// If a filename was selected, export it
			if (!exportAsPNG(selection[0], info.filenames[0], force_rgba))
			{
				wxMessageBox(wxString::Format("Error: %s", global::error), "Error", wxOK | wxICON_ERROR);
				return;
			}
		}

		return;
	}
	else
	{
		// Run save files dialog
		filedialog::FDInfo info;
		if (filedialog::saveFiles(
				info, "Export Textures as PNG (Filename will be ignored)", "PNG Files (*.png)|*.png", this))
		{
			// Show splash window
			ui::showSplash("Saving converted image data...", true);

			// Go through the selection
			for (size_t a = 0; a < selection.size(); a++)
			{
				// Update splash window
				ui::setSplashProgressMessage(selection[a]->name());
				ui::setSplashProgress((float)a / (float)selection.size());

				// Setup entry filename
				wxFileName fn(selection[a]->name());
				fn.SetPath(info.path);
				fn.SetExt("png");

				// Do export
				exportAsPNG(selection[a], fn.GetFullPath(), force_rgba);
			}

			// Hide splash window
			ui::hideSplash();
		}
	}
}

// -----------------------------------------------------------------------------
// Changes the offsets for each selected texture. Only for ZDoom!
// -----------------------------------------------------------------------------
bool TextureXPanel::modifyOffsets()
{
	if (!tx_entry_)
		return false;

	// Create modify offsets dialog
	ModifyOffsetsDialog mod;
	mod.SetParent(this);
	mod.CenterOnParent();

	// Run the dialog
	if (mod.ShowModal() == wxID_CANCEL)
		return false;

	// Go through selection
	auto selec_num = list_textures_->selection(true);
	for (long index : selec_num)
	{
		// Get texture
		bool current = false;
		auto ctex    = texturex_.texture(index);
		if (ctex == tex_current_)
		{
			// Texture is currently open in the editor
			ctex    = texture_editor_->texture();
			current = true;
		}

		// Calculate and apply new offsets
		Vec2i offsets = mod.calculateOffsets(ctex->offsetX(), ctex->offsetY(), ctex->width(), ctex->height());
		ctex->setOffsetX(offsets.x);
		ctex->setOffsetY(offsets.y);

		ctex->setState(1);
		modified_ = true;

		// If it was the current texture, update controls
		if (current)
			texture_editor_->updateTextureControls();
	}

	return true;
}

// -----------------------------------------------------------------------------
// Called when an action is undone
// -----------------------------------------------------------------------------
void TextureXPanel::onUndo(const wxString& undo_action) const
{
	list_textures_->updateList();
}

// -----------------------------------------------------------------------------
// Called when an action is redone
// -----------------------------------------------------------------------------
void TextureXPanel::onRedo(const wxString& undo_action) const
{
	list_textures_->updateList();
}

// -----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// -----------------------------------------------------------------------------
bool TextureXPanel::handleAction(string_view id)
{
	// Skip event if this panel is not the current page
	auto parent = dynamic_cast<TabControl*>(GetParent());
	if (parent->GetCurrentPage() != this)
		return false;
	// Skip event if this panel is hidden
	if (!tx_editor_->IsShown() || !IsShown())
		return false;

	// Only interested in "txed_" events
	if (!strutil::startsWith(id, "txed_"))
		return false;

	// Handle action
	if (id == "txed_savelist")
		saveTEXTUREX();
	else if (id == "txed_new")
		newTexture();
	else if (id == "txed_delete")
		removeTexture();
	else if (id == "txed_new_patch")
		newTextureFromPatch();
	else if (id == "txed_new_file")
		newTextureFromFile();
	else if (id == "txed_up")
		moveUp();
	else if (id == "txed_down")
		moveDown();
	else if (id == "txed_sort")
		sort();
	else if (id == "txed_copy")
		copy();
	else if (id == "txed_cut")
	{
		copy();
		removeTexture();
	}
	else if (id == "txed_paste")
		paste();
	else if (id == "txed_export")
		exportTexture();
	else if (id == "txed_extract")
		extractTexture();
	else if (id == "txed_rename")
		renameTexture();
	else if (id == "txed_rename_each")
		renameTexture(true);
	else if (id == "txed_offsets")
		modifyOffsets();
	else if (texture_editor_->handleSAction(id)) // Send to TextureEditorPanel
		return true;
	else
		return false; // Not handled here

	return true;
}


// -----------------------------------------------------------------------------
//
// TextureXPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when an item on the texture list is selected
// -----------------------------------------------------------------------------
void TextureXPanel::onTextureListSelect(wxListEvent& e)
{
	// Update toolbar buttons
	auto selcount = list_textures_->GetSelectedItemCount();
	if (selcount == 0)
	{
		toolbar_->group("_Texture")->setAllButtonsEnabled(false);
		toolbar_->group("_Sorting")->setAllButtonsEnabled(false);
		toolbar_->findActionButton("txed_sort")->Enable();
	}
	if (selcount >= 1)
	{
		toolbar_->group("_Texture")->setAllButtonsEnabled(true);
		toolbar_->group("_Sorting")->setAllButtonsEnabled(true);
	}
	if (selcount == 1)
		toolbar_->findActionButton("txed_rename_each")->Enable(false);


	// Do nothing if multiple textures are selected
	if (list_textures_->GetSelectedItemCount() > 1)
	{
		tex_current_ = nullptr;
		undo_manager_->setResetPoint();
		texture_editor_->openTexture(tex_current_, &texturex_);
		return;
	}

	// Get selected texture
	auto tex = texturex_.texture(list_textures_->itemIndex(e.GetIndex()));

	// Save any changes to previous texture
	applyChanges();

	// Open texture in editor
	undo_manager_->setResetPoint();
	texture_editor_->openTexture(tex, &texturex_);

	// Set current texture
	tex_current_ = tex;
}

// -----------------------------------------------------------------------------
// Called when an item on the texture list is right clicked
// -----------------------------------------------------------------------------
void TextureXPanel::onTextureListRightClick(wxListEvent& e)
{
	// Create context menu
	wxMenu context;
	auto   texport = new wxMenu();
	SAction::fromId("txed_delete")->addToMenu(&context, true);
	context.AppendSeparator();
	SAction::fromId("txed_rename")->addToMenu(&context, true);
	if (list_textures_->GetSelectedItemCount() > 1)
		SAction::fromId("txed_rename_each")->addToMenu(&context, true);
	if (TxListIsTextures(texturex_))
		SAction::fromId("txed_offsets")->addToMenu(&context, true);
	SAction::fromId("txed_export")->addToMenu(texport, true, "Archive (as image)");
	SAction::fromId("txed_extract")->addToMenu(texport, true, "File");
	context.AppendSubMenu(texport, "&Export To");
	context.AppendSeparator();
	SAction::fromId("txed_copy")->addToMenu(&context, true);
	SAction::fromId("txed_cut")->addToMenu(&context, true);
	SAction::fromId("txed_paste")->addToMenu(&context, true);
	context.AppendSeparator();
	SAction::fromId("txed_up")->addToMenu(&context, true);
	SAction::fromId("txed_down")->addToMenu(&context, true);
	SAction::fromId("txed_sort")->addToMenu(&context, true);

	// Pop it up
	PopupMenu(&context);
}

// -----------------------------------------------------------------------------
// Called when a key is pressed in the texture list
// -----------------------------------------------------------------------------
void TextureXPanel::onTextureListKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	auto binds = KeyBind::bindsForKey(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Go through matching binds
	for (auto& name : binds)
	{
		// Copy
		if (name == "copy")
		{
			copy();
			return;
		}

		// Cut
		else if (name == "cut")
		{
			copy();
			removeTexture();
			return;
		}

		// Paste
		else if (name == "paste")
		{
			paste();
			return;
		}

		// Move texture up
		else if (name == "txed_tex_up")
		{
			moveUp();
			return;
		}

		// Move texture down
		else if (name == "txed_tex_down")
		{
			moveDown();
			return;
		}

		// New texture
		else if (name == "txed_tex_new")
		{
			newTexture();
			return;
		}

		// New texture from patch
		else if (name == "txed_tex_new_patch")
		{
			newTextureFromPatch();
			return;
		}

		// New texture from file
		else if (name == "txed_tex_new_file")
		{
			newTextureFromFile();
			return;
		}

		// Delete texture
		else if (name == "txed_tex_delete")
		{
			removeTexture();
			return;
		}
	}

	// Not handled here, send off to be handled by a parent window
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the filter text is changed
// -----------------------------------------------------------------------------
void TextureXPanel::onTextFilterChanged(wxCommandEvent& e)
{
	// Filter the entry list
	list_textures_->setFilter(text_filter_->GetValue());

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the 'Clear Filter' button is clicked
// -----------------------------------------------------------------------------
void TextureXPanel::onBtnClearFitler(wxCommandEvent& e)
{
	text_filter_->SetValue(wxEmptyString);
	list_textures_->setFilter(wxEmptyString);
}

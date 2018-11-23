
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "TextureXPanel.h"
#include "Archive/ArchiveManager.h"
#include "Dialogs/GfxConvDialog.h"
#include "Dialogs/ModifyOffsetsDialog.h"
#include "General/Clipboard.h"
#include "General/ColourConfiguration.h"
#include "General/KeyBind.h"
#include "General/Misc.h"
#include "General/ResourceManager.h"
#include "General/UndoRedo.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "App.h"
#include "TextureXEditor.h"
#include "Utility/SFileDialog.h"
#include "ZTextureEditorPanel.h"
#include "UI/Controls/SIconButton.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
//
// External Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(String, dir_last)
EXTERN_CVAR(Bool, wad_force_uppercase)


// ----------------------------------------------------------------------------
//
// TextureXListView Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// TextureXListView::TextureXListView
//
// TextureXListView class constructor
// ----------------------------------------------------------------------------
TextureXListView::TextureXListView(wxWindow* parent, TextureXList* texturex) :
	VirtualListView{ parent },
	texturex_{ texturex }
{
	// Add columns
	InsertColumn(0, "Name");
	InsertColumn(1, "Size");

	// Update
	updateList();
}

// ----------------------------------------------------------------------------
// TextureXListView::getItemText
//
// Returns the string for [item] at [column]
// ----------------------------------------------------------------------------
string TextureXListView::getItemText(long item, long column, long index) const
{
	// Check texture list exists
	if (!texturex_)
		return "INVALID INDEX";

	// Check index is ok
	if (index < 0 || (unsigned)index > texturex_->nTextures())
		return "INVALID INDEX";

	// Get associated texture
	CTexture* tex = texturex_->getTexture(index);

	if (column == 0)						// Name column
		return tex->getName();
	else if (column == 1)					// Size column
		return S_FMT("%dx%d", tex->getWidth(), tex->getHeight());
	else if (column == 2)					// Type column
		return tex->getType();
	else
		return "INVALID COLUMN";
}

// ----------------------------------------------------------------------------
// TextureXListView::updateItemAttr
//
// Called when widget requests the attributes
// (text colour / background colour / font) for [item]
// ----------------------------------------------------------------------------
void TextureXListView::updateItemAttr(long item, long column, long index) const
{
	// Check texture list exists
	if (!texturex_)
		return;

	// Check index is ok
	if (index < 0 || (unsigned)index > texturex_->nTextures())
		return;

	// Get associated texture
	CTexture* tex = texturex_->getTexture(index);

	// Init attributes
	item_attr->SetTextColour(WXCOL(ColourConfiguration::getColour("error")));

	// If texture doesn't exist, return error colour
	if (!tex)
		return;

	// Set colour depending on entry state
	switch (tex->getState())
	{
	case 1:
		item_attr->SetTextColour(WXCOL(ColourConfiguration::getColour("modified")));
		break;
	case 2:
		item_attr->SetTextColour(WXCOL(ColourConfiguration::getColour("new")));
		break;
	default:
		item_attr->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));
		break;
	};
}

// ----------------------------------------------------------------------------
// TextureXListView::updateList
//
// Clears the list if [clear] is true, and refreshes it
// ----------------------------------------------------------------------------
void TextureXListView::updateList(bool clear)
{
	if (clear)
		ClearAll();

	// Set list size
	items.clear();
	if (texturex_)
	{
		unsigned count = texturex_->nTextures();
		for (unsigned a = 0; a < count; a++)
			items.push_back(a);
		applyFilter();
		SetItemCount(items.size());
	}
	else
		SetItemCount(0);

	sortItems();
	updateWidth();
	Refresh();
}

// ----------------------------------------------------------------------------
// TextureXListView::sizeSort
//
// Returns true if texture at index [left] is smaller than [right]
// ----------------------------------------------------------------------------
bool TextureXListView::sizeSort(long left, long right)
{
	CTexture* tl = ((TextureXListView*)lv_current)->txList()->getTexture(left);
	CTexture* tr = ((TextureXListView*)lv_current)->txList()->getTexture(right);
	int s1 = tl->getWidth() * tl->getHeight();
	int s2 = tr->getWidth() * tr->getHeight();

	if (s1 == s2)
		return left < right;
	else
		return lv_current->sortDescend() ? s1 > s2 : s2 > s1;
}

// ----------------------------------------------------------------------------
// TextureXListView::sortItems
//
// Sorts the list items depending on the current sorting column
// ----------------------------------------------------------------------------
void TextureXListView::sortItems()
{
	lv_current = this;
	if (sort_column == 1)
		std::sort(items.begin(), items.end(), &TextureXListView::sizeSort);
	else
		std::sort(items.begin(), items.end(), &VirtualListView::defaultSort);
}

// ----------------------------------------------------------------------------
// TextureXListView::applyFilter
//
// Filters items by the current filter text string
// ----------------------------------------------------------------------------
void TextureXListView::applyFilter()
{
	// Show all if no filter
	if (filter_text.IsEmpty())
		return;

	// Split filter by ,
	wxArrayString terms = wxSplit(filter_text, ',');

	// Process filter strings
	for (auto& term : terms)
	{
		// Remove spaces
		term.Replace(" ", "");

		// Set to lowercase and add * to the end
		if (!term.IsEmpty()) term = term.Lower() + "*";
	}

	// Go through filtered list
	for (unsigned a = 0; a < items.size(); a++)
	{
		auto tex = texturex_->getTexture(items[a]);

		// Check for name match with filter
		bool match = false;
		for (auto& term : terms)
		{
			if (tex->getName().Lower().Matches(term))
			{
				match = true;
				break;
			}
		}
		if (match)
			continue;

		// No match, remove from filtered list
		items.erase(items.begin() + a);
		a--;
	}
}


// ----------------------------------------------------------------------------
//
// Undo Steps
//
// ----------------------------------------------------------------------------

class TextureSwapUS : public UndoStep
{
public:
	TextureSwapUS(TextureXList& texturex, int index1, int index2)
		: texturex_(texturex), index1_(index1), index2_(index2) {}
	~TextureSwapUS() {}

	bool doSwap()
	{
		// Get parent dir
		texturex_.swapTextures(index1_, index2_);
		return true;
	}

	bool doUndo() override
	{
		return doSwap();
	}

	bool doRedo() override
	{
		return doSwap();
	}

private:
	TextureXList&	texturex_;
	int				index1_;
	int				index2_;
};

class TextureCreateDeleteUS : public UndoStep
{
public:
	TextureCreateDeleteUS(TextureXPanel* tx_panel, CTexture* texture, bool created)
		: tx_panel_(tx_panel), created_(created)
	{
		tex_removed_ = created ? NULL : texture;
		index_ = tx_panel->txList().textureIndex(texture->getName());
	}

	~TextureCreateDeleteUS()
	{
		if (tex_removed_)
			delete tex_removed_;
	}

	bool deleteTexture()
	{
		tex_removed_ = tx_panel_->txList().removeTexture(index_, false);
		if (tx_panel_->currentTexture() == tex_removed_)
			tx_panel_->textureEditor()->clearTexture();
		return true;
	}

	bool createTexture()
	{
		tx_panel_->txList().addTexture(tex_removed_, index_);
		tex_removed_ = nullptr;
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
	TextureXPanel*	tx_panel_;
	CTexture*		tex_removed_;
	int				index_;
	bool			created_;
};

class TextureModificationUS : public UndoStep
{
public:
	TextureModificationUS(TextureXPanel* tx_panel, CTexture* texture)
		: tx_panel_(tx_panel)
	{
		tex_copy_ = new CTexture();
		tex_copy_->copyTexture(texture);
		tex_copy_->setState(texture->getState());
		index_ = tx_panel->txList().textureIndex(tex_copy_->getName());
	}

	~TextureModificationUS()
	{
		delete tex_copy_;
	}

	bool swapData()
	{
		CTexture* replaced = tx_panel_->txList().replaceTexture(index_, tex_copy_);
		if (replaced)
		{
			if (tx_panel_->currentTexture() == replaced || tx_panel_->currentTexture() == tex_copy_)
				tx_panel_->textureEditor()->openTexture(tex_copy_, &(tx_panel_->txList()));
			tex_copy_ = replaced;
		}
		else
			return false;

		return true;
	}

	bool doUndo() override
	{
		return swapData();
	}

	bool doRedo() override
	{
		return swapData();
	}

private:
	TextureXPanel*	tx_panel_;
	CTexture*		tex_copy_;
	int				index_;
};


// ----------------------------------------------------------------------------
//
// TextureXPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// TextureXPanel::TextureXPanel
//
// TextureXPanel class constructor
// ----------------------------------------------------------------------------
TextureXPanel::TextureXPanel(wxWindow* parent, TextureXEditor& tx_editor) :
	wxPanel{ parent, -1 },
	tx_editor_{ &tx_editor },
	undo_manager_{ tx_editor.getUndoManager() }
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Add textures list
	wxStaticBox* frame = new wxStaticBox(this, -1, "Textures");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	label_tx_format_ = new wxStaticText(this, -1, "Format:");
	hbox->Add(label_tx_format_, 0, wxALIGN_BOTTOM | wxRIGHT, UI::pad());
	btn_save_ = new SIconButton(this, "save", "Save");
	hbox->AddStretchSpacer();
	hbox->Add(btn_save_, 0, wxEXPAND);
	framesizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT, UI::pad());
	list_textures_ = new TextureXListView(this, &texturex_);
	framesizer->Add(list_textures_, 1, wxEXPAND | wxALL, UI::pad());
	sizer->Add(framesizer, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, UI::pad());

	// Texture list filter
	text_filter_ = new wxTextCtrl(this, -1);
	btn_clear_filter_ = new SIconButton(this, "close", "Clear Filter");
	WxUtils::layoutHorizontally(
		framesizer,
		vector<wxObject*>{ WxUtils::createLabelHBox(this, "Filter:", text_filter_), btn_clear_filter_ },
		wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, UI::pad()),
		0
	);

	// Add texture operations buttons
	wxGridBagSizer* gbsizer = new wxGridBagSizer(UI::pad(), UI::pad());
	framesizer->Add(gbsizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());
	btn_move_up_ = new SIconButton(this, "up", "Move Up");
	btn_move_down_ = new SIconButton(this, "down", "Move Down");
	btn_new_texture_ = new SIconButton(this, "tex_new", "New");
	btn_remove_texture_ = new SIconButton(this, "tex_delete", "Remove");
	btn_new_from_patch_ = new SIconButton(this, "tex_newpatch", "New from Patch");
	btn_new_from_file_ = new SIconButton(this, "tex_newfile", "New from File");
	gbsizer->Add(btn_new_texture_, { 0, 0 }, { 1, 1 });
	gbsizer->Add(btn_new_from_patch_, { 0, 1 }, { 1, 1 });
	gbsizer->Add(btn_new_from_file_, { 0, 2 }, { 1, 1 });
	gbsizer->Add(btn_remove_texture_, { 0, 3 }, { 1, 1 });
	gbsizer->Add(btn_move_up_, { 0, 4 }, { 1, 1 });
	gbsizer->Add(btn_move_down_, { 0, 5 }, { 1, 1 });

	// Bind events
	list_textures_->Bind(wxEVT_LIST_ITEM_SELECTED, &TextureXPanel::onTextureListSelect, this);
	list_textures_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &TextureXPanel::onTextureListRightClick, this);
	list_textures_->Bind(wxEVT_KEY_DOWN, &TextureXPanel::onTextureListKeyDown, this);
	btn_new_texture_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { newTexture(); });
	btn_new_from_patch_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { newTextureFromPatch(); });
	btn_new_from_file_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { newTextureFromFile(); });
	btn_remove_texture_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { removeTexture(); });
	btn_move_up_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { moveUp(); });
	btn_move_down_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { moveDown(); });
	btn_save_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { tx_editor_->saveChanges(); });
	Bind(wxEVT_SHOW, [&](wxShowEvent&) { tx_editor_->updateMenuStatus(); });
	text_filter_->Bind(wxEVT_TEXT, &TextureXPanel::onTextFilterChanged, this);
	btn_clear_filter_->Bind(wxEVT_BUTTON, &TextureXPanel::onBtnClearFitler, this);
}

// ----------------------------------------------------------------------------
// TextureXPanel::~TextureXPanel
//
// TextureXPanel class destructor
// ----------------------------------------------------------------------------
TextureXPanel::~TextureXPanel()
{
	if (tx_entry_)
		tx_entry_->unlock();
}

// ----------------------------------------------------------------------------
// TextureXPanel::openTEXTUREX
//
// Loads a TEXTUREx or TEXTURES format texture list into the editor
// ----------------------------------------------------------------------------
bool TextureXPanel::openTEXTUREX(ArchiveEntry* entry)
{
	// Open texture list (check format)
	if (entry->getType()->formatId() == "texturex")
	{
		// TEXTURE1/2 format
		if (!texturex_.readTEXTUREXData(entry, tx_editor_->patchTable()))
			return false;

		// Create default texture editor
		texture_editor_ = new TextureEditorPanel(this, tx_editor_);

		// Update patch table usage info
		for (size_t a = 0; a < texturex_.nTextures(); a++)
		{
			CTexture* tex = texturex_.getTexture(a);

			// Go through texture's patches
			for (size_t p = 0; p < tex->nPatches(); p++)
				tx_editor_->patchTable().patch(tex->getPatch(p)->getName()).used_in.push_back(tex->getName());
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
	GetSizer()->Add(texture_editor_, 1, wxEXPAND|wxALL, UI::pad());
	texture_editor_->setupLayout();

	// Update format label
	label_tx_format_->SetLabel("Format: " + texturex_.getTextureXFormatString());

	// Update texture list
	list_textures_->updateList();

	return true;
}

// ----------------------------------------------------------------------------
// TextureXPanel::saveTEXTUREX
//
// Saves a TEXTUREX format texture list
// ----------------------------------------------------------------------------
bool TextureXPanel::saveTEXTUREX()
{
	// Save any changes to current texture
	applyChanges();

	// Write list to entry, in the correct format
	tx_entry_->unlock();	// Have to unlock the entry first
	bool ok = false;
	if (texturex_.getFormat() == TXF_TEXTURES)
		ok = texturex_.writeTEXTURESData(tx_entry_);
	else
		ok = texturex_.writeTEXTUREXData(tx_entry_, tx_editor_->patchTable());

	// Redetect type and lock it up
	EntryType::detectEntryType(tx_entry_);
	tx_entry_->lock();

	// Set all textures to unmodified
	for (unsigned a = 0; a < texturex_.nTextures(); a++)
		texturex_.getTexture(a)->setState(0);
	list_textures_->updateList();

	// Update variables
	modified_ = false;

	return ok;
}

// ----------------------------------------------------------------------------
// TextureXPanel::setPalette
//
// Sets the texture editor's palette
// ----------------------------------------------------------------------------
void TextureXPanel::setPalette(Palette* pal)
{
	texture_editor_->setPalette(pal);
}

// ----------------------------------------------------------------------------
// TextureXPanel::applyChanges
//
// Applies changes to the current texture, if any
// ----------------------------------------------------------------------------
void TextureXPanel::applyChanges()
{
	if (texture_editor_->texModified() && tex_current_)
	{
		// Clear undo levels for individual edits to the current texture
		undo_manager_->clearToResetPoint();

		// Create single 'modify texture' undo level
		undo_manager_->beginRecord("Modify Texture");
		undo_manager_->recordUndoStep(new TextureModificationUS(this, tex_current_));
		undo_manager_->endRecord(true);

		undo_manager_->setResetPoint();

		tex_current_->copyTexture(texture_editor_->texture());
		tex_current_->setState(1);
		tx_editor_->patchTable().updatePatchUsage(tex_current_);
		list_textures_->updateList();
		modified_ = true;
		texture_editor_->openTexture(tex_current_, &texturex_);
	}
}

// ----------------------------------------------------------------------------
// TextureXPanel::newTextureFromPatch
//
// Creates a new texture called [name] from [patch]. The new texture will be
// set to the dimensions of the patch, with the patch added at 0,0
// ----------------------------------------------------------------------------
CTexture* TextureXPanel::newTextureFromPatch(string name, string patch)
{
	// Create new texture
	CTexture* tex = new CTexture();
	tex->setName(name);
	tex->setState(2);

	// Setup texture scale
	if (texturex_.getFormat() == TXF_TEXTURES)
	{
		tex->setScale(1, 1);
		tex->setExtended(true);
	}
	else
		tex->setScale(0, 0);

	// Add patch
	tex->addPatch(patch, 0, 0);

	// Load patch image (to determine dimensions)
	SImage image;
	tex->loadPatchImage(0, image);

	// Set dimensions
	tex->setWidth(image.getWidth());
	tex->setHeight(image.getHeight());

	// Update variables
	modified_ = true;

	// Return the new texture
	return tex;
}

// ----------------------------------------------------------------------------
// TextureXPanel::newTexture
//
// Creates a new, empty texture
// ----------------------------------------------------------------------------
void TextureXPanel::newTexture()
{
	// Prompt for new texture name
	string name = wxGetTextFromUser("Enter a texture name:", "New Texture");

	// Do nothing if no name entered
	if (name.IsEmpty())
		return;

	// Process name
	if (texturex_.getFormat() != TXF_TEXTURES)
	{
		name = name.Upper().Truncate(8);
	}

	// Create new texture
	CTexture* tex = new CTexture();
	tex->setName(name);
	tex->setState(2);

	// Default size = 64x128
	tex->setWidth(64);
	tex->setHeight(128);

	// Setup texture scale
	if (texturex_.getFormat() == TXF_TEXTURES)
	{
		tex->setScale(1, 1);
		tex->setExtended(true);
	}
	else
		tex->setScale(0, 0);

	// Add it after the last selected item
	int selected = list_textures_->getItemIndex(list_textures_->getLastSelected());
	if (selected == -1) selected = texturex_.nTextures() - 1; // Add to end of the list if nothing selected
	texturex_.addTexture(tex, selected + 1);

	// Record undo level
	undo_manager_->beginRecord("New Texture");
	undo_manager_->recordUndoStep(new TextureCreateDeleteUS(this, tex, true));
	undo_manager_->endRecord(true);

	// Update texture list
	list_textures_->updateList();

	// Select the new texture
	list_textures_->clearSelection();
	list_textures_->selectItem(selected + 1);
	list_textures_->EnsureVisible(selected + 1);

	// Update variables
	modified_ = true;
}

// ----------------------------------------------------------------------------
// TextureXPanel::newTextureFromPatch
//
// Creates a new texture from an existing patch
// ----------------------------------------------------------------------------
void TextureXPanel::newTextureFromPatch()
{
	// Browse for patch
	string patch;
	if (texturex_.getFormat() == TXF_TEXTURES)
		patch = tx_editor_->browsePatchEntry();
	else
		patch = tx_editor_->patchTable().patchName(tx_editor_->browsePatchTable());

	if (!patch.IsEmpty())
	{
		// Prompt for new texture name
		string name = wxGetTextFromUser("Enter a texture name:", "New Texture", patch);

		// Do nothing if no name entered
		if (name.IsEmpty())
			return;

		// Process name
		name = name.Upper().Truncate(8);

		// Create new texture from patch
		CTexture* tex = newTextureFromPatch(name, patch);

		// Add texture after the last selected item
		int selected = list_textures_->getItemIndex(list_textures_->getLastSelected());
		if (selected == -1) selected = texturex_.nTextures() - 1; // Add to end of the list if nothing selected
		texturex_.addTexture(tex, selected + 1);

		// Record undo level
		undo_manager_->beginRecord("New Texture from Patch");
		undo_manager_->recordUndoStep(new TextureCreateDeleteUS(this, tex, true));
		undo_manager_->endRecord(true);

		// Update texture list
		list_textures_->updateList();

		// Select the new texture
		list_textures_->clearSelection();
		list_textures_->selectItem(selected + 1);
		list_textures_->EnsureVisible(selected + 1);

		// Update patch table counts
		tx_editor_->patchTable().updatePatchUsage(tex);
	}
}

// ----------------------------------------------------------------------------
// TextureXPanel::newTextureFromFile
//
// Creates a new texture from an image file. The file will be imported and
// added to the patch table if needed
// ----------------------------------------------------------------------------
void TextureXPanel::newTextureFromFile()
{
	// Get all entry types
	vector<EntryType*> etypes = EntryType::allTypes();

	// Go through types
	string ext_filter = "All files (*.*)|*.*|";
	for (unsigned a = 0; a < etypes.size(); a++)
	{
		// If the type is a valid image type, add its extension filter
		if (etypes[a]->extraProps().propertyExists("image"))
		{
			ext_filter += etypes[a]->fileFilterString();
			ext_filter += "|";
		}
	}

	// Create open file dialog
	wxFileDialog dialog_open(this, "Choose file(s) to open", dir_last, wxEmptyString,
	                         ext_filter, wxFD_OPEN|wxFD_MULTIPLE|wxFD_FILE_MUST_EXIST, wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Get file selection
		wxArrayString files;
		dialog_open.GetPaths(files);

		// Save 'dir_last'
		dir_last = dialog_open.GetDirectory();

		// Go through file selection
		for (unsigned a = 0; a < files.size(); a++)
		{
			// Load the file into a temporary ArchiveEntry
			ArchiveEntry* entry = new ArchiveEntry();
			entry->importFile(files[a]);

			// Determine type
			EntryType::detectEntryType(entry);

			// If it's not a valid image type, ignore this file
			if (!entry->getType()->extraProps().propertyExists("image"))
			{
				LOG_MESSAGE(1, "%s is not a valid image file", files[a]);
				continue;
			}

			// Ask for name for texture
			wxFileName fn(files[a]);
			string name = fn.GetName().Upper().Truncate(8);
			name = wxGetTextFromUser(S_FMT("Enter a texture name for %s:", fn.GetFullName()), "New Texture", name);
			name = name.Truncate(8);

			// Add patch to archive
			entry->setName(name);
			entry->setExtensionByType();
			tx_entry_->getParent()->addEntry(entry, "patches");

			// Add patch to patch table if needed
			if (texturex_.getFormat() != TXF_TEXTURES)
				tx_editor_->patchTable().addPatch(name);


			// Create new texture from patch
			CTexture* tex = newTextureFromPatch(name, name);

			// Add texture after the last selected item
			int selected = list_textures_->getItemIndex(list_textures_->getLastSelected());
			if (selected == -1) selected = texturex_.nTextures() - 1; // Add to end of the list if nothing selected
			texturex_.addTexture(tex, selected + 1);

			// Record undo level
			undo_manager_->beginRecord("New Texture from File");
			undo_manager_->recordUndoStep(new TextureCreateDeleteUS(this, tex, true));
			undo_manager_->endRecord(true);

			// Update texture list
			list_textures_->updateList();

			// Select the new texture
			list_textures_->clearSelection();
			list_textures_->selectItem(selected + 1);
			list_textures_->EnsureVisible(selected + 1);

			// Update patch table counts
			tx_editor_->patchTable().updatePatchUsage(tex);
		}
	}
}

// ----------------------------------------------------------------------------
// TextureXPanel::removeTexture
//
// Removes any selected textures
// ----------------------------------------------------------------------------
void TextureXPanel::removeTexture()
{
	// Get selected textures
	vector<long> selection = list_textures_->getSelection(true);

	// Begin recording undo level
	undo_manager_->beginRecord("Remove Texture(s)");

	// Go through selection backwards
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Remove texture from patch table entries
		CTexture* tex = texturex_.getTexture(selection[a]);
		for (unsigned p = 0; p < tex->nPatches(); p++)
			tx_editor_->patchTable().patch(tex->getPatch(p)->getName()).removeTextureUsage(tex->getName());

		// Record undo step
		undo_manager_->recordUndoStep(new TextureCreateDeleteUS(this, tex, false));

		// Remove texture from list
		texturex_.removeTexture(selection[a], false);
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

// ----------------------------------------------------------------------------
// TextureXPanel::moveUp
//
// Moves all selected textures up
// ----------------------------------------------------------------------------
void TextureXPanel::moveUp()
{
	// Get selected textures
	vector<long> selection = list_textures_->getSelection(true);

	// Do nothing if the first selected item is at the top of the list
	if (selection.size() > 0 && selection[0] == 0)
		return;

	// Begin recording undo level
	undo_manager_->beginRecord("Move Texture(s) Up");

	// Go through selection
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Swap selected texture with the one above it
		texturex_.swapTextures(selection[a], selection[a] - 1);

		// Record undo step
		undo_manager_->recordUndoStep(new TextureSwapUS(texturex_, selection[a], selection[a] - 1));
	}

	// End recording undo level
	undo_manager_->endRecord(true);

	// Update selection
	list_textures_->clearSelection();
	for (unsigned a = 0; a < selection.size(); a++)
		list_textures_->selectItem(selection[a] - 1);

	// Refresh
	list_textures_->updateList();

	// Update variables
	modified_ = true;
}

// ----------------------------------------------------------------------------
// TextureXPanel::moveDown
//
// Moves all selected textures down
// ----------------------------------------------------------------------------
void TextureXPanel::moveDown()
{
	// Get selected textures
	vector<long> selection = list_textures_->getSelection(true);

	// Do nothing if the last selected item is at the end of the list
	if (selection.size() > 0 && selection.back() == list_textures_->GetItemCount()-1)
		return;

	// Begin recording undo level
	undo_manager_->beginRecord("Move Texture(s) Down");

	// Go through selection backwards
	for (int a = selection.size()-1; a >= 0; a--)
	{
		// Swap selected texture with the one below it
		texturex_.swapTextures(selection[a], selection[a] + 1);

		// Record undo step
		undo_manager_->recordUndoStep(new TextureSwapUS(texturex_, selection[a], selection[a] + 1));
	}

	// End recording undo level
	undo_manager_->endRecord(true);

	// Update selection
	list_textures_->clearSelection();
	for (unsigned a = 0; a < selection.size(); a++)
		list_textures_->selectItem(selection[a] + 1);

	// Refresh
	list_textures_->updateList();

	// Update variables
	modified_ = true;
}

// ----------------------------------------------------------------------------
// TextureXPanel::sort
//
// Sorts all selected textures
// ----------------------------------------------------------------------------
void TextureXPanel::sort()
{
	// Get selected textures
	vector<long> selection = list_textures_->getSelection(true);
	// Without selection of multiple texture, sort everything instead
	if (selection.size() < 2)
	{
		selection.clear();
		selection.resize(texturex_.nTextures());
		for (size_t i = 0; i < texturex_.nTextures(); ++i)
			selection[i] = i;
	}

	// No sorting needed even after adding everything
	if (selection.size() < 2)
		return;

	// Fill a map with <texture name, texture index> pairs
	size_t * origindex = new size_t[texturex_.nTextures()];
	std::map<string, size_t> tmap; tmap.clear();
	for (size_t i = 0; i < selection.size(); ++i)
	{
		// We want to be sure that each key is unique, so we add the position to the name string
		string name = S_FMT("%-8s%8d", texturex_.getTexture(selection[i])->getName(), selection[i]);
		// x keeps the current position, while y keeps the original position
		tmap[name] = selection[i];
		origindex[selection[i]] = selection[i];
	}

	// Begin recording undo level
	undo_manager_->beginRecord("Sort Textures");

	// And now, sort the textures based on the map
	std::map<string, size_t>::iterator itr = tmap.begin();
	for (size_t i = 0; i < selection.size(); ++i)
	{
		// If the texture isn't in its sorted place already
		if (selection[i] != itr->second)
		{
			// Swap the texture in the spot with the sorted one
			size_t tmp = origindex[selection[i]];
			origindex[selection[i]] = origindex[itr->second];
			origindex[itr->second] = tmp;
			texturex_.swapTextures(selection[i], itr->second);
			undo_manager_->recordUndoStep(new TextureSwapUS(texturex_, selection[i], itr->second));
			// Update the position of the displaced texture in the tmap
			string name = S_FMT("%-8s%8d", texturex_.getTexture(itr->second)->getName(), tmp);
			tmap[name] = itr->second;
		}
		itr++;
	}

	// End recording undo level
	undo_manager_->endRecord(true);

	// Cleanup
	delete[] origindex;

	// Refresh
	list_textures_->updateList();

	// Update variables
	modified_ = true;
}

// ----------------------------------------------------------------------------
// TextureXPanel::copy
//
// Copies any selected textures to the clipboard
// ----------------------------------------------------------------------------
void TextureXPanel::copy()
{
	// Get selected textures
	vector<long> selection = list_textures_->getSelection(true);

	// Do nothing if nothing selected
	if (selection.size() == 0)
		return;

	// Create list of textures to copy
	vector<ClipboardItem*> copy_items;
	for (unsigned a = 0; a < selection.size(); a++)
		copy_items.push_back(new TextureClipboardItem(texturex_.getTexture(selection[a]), tx_editor_->getArchive()));

	// Add list to clipboard
	theClipboard->addItems(copy_items);
}

// ----------------------------------------------------------------------------
// TextureXPanel::paste
//
// Pastes any textures on the clipboard after the last selected texture
// ----------------------------------------------------------------------------
void TextureXPanel::paste()
{
	// Check there is anything on the clipboard
	if (theClipboard->nItems() == 0)
		return;

	// Get last selected index
	int selected = list_textures_->getItemIndex(list_textures_->getLastSelected());
	if (selected == -1) selected = texturex_.nTextures() - 1; // Add to end of the list if nothing selected

	// Begin recording undo level
	undo_manager_->beginRecord("Paste Texture(s)");

	// Go through clipboard items
	for (unsigned a = 0; a < theClipboard->nItems(); a++)
	{
		// Skip if not a texture clipboard item
		if (theClipboard->getItem(a)->getType() != CLIPBOARD_COMPOSITE_TEXTURE)
			continue;

		// Get texture item
		TextureClipboardItem* item = (TextureClipboardItem*)(theClipboard->getItem(a));

		// Add new texture after last selected item
		CTexture* ntex = new CTexture((texturex_.getFormat() == TXF_TEXTURES));
		ntex->copyTexture(item->getTexture(), true);
		ntex->setState(2);
		texturex_.addTexture(ntex, ++selected);

		// Record undo step
		undo_manager_->recordUndoStep(new TextureCreateDeleteUS(this, ntex, true));

		// Deal with patches
		for (unsigned p = 0; p < ntex->nPatches(); p++)
		{
			CTPatch* patch = ntex->getPatch(p);

			// Update patch table if necessary
			if (texturex_.getFormat() != TXF_TEXTURES)
				tx_editor_->patchTable().addPatch(patch->getName());

			// Get the entry for this patch
			ArchiveEntry* entry = patch->getPatchEntry(tx_editor_->getArchive());

			// If the entry wasn't found in any open archive, try copying it from the clipboard
			// (the user may have closed the archive the original patch was in)
			if (!entry)
			{
				entry = item->getPatchEntry(patch->getName());

				// Copy the copied patch entry over to this archive
				if (entry)
					tx_editor_->getArchive()->addEntry(entry, "patches", true);
			}

			// If the entry exists in the base resource archive or this archive, do nothing
			else if (entry->getParent() == App::archiveManager().baseResourceArchive() ||
			         entry->getParent() == tx_editor_->getArchive())
				continue;

			// Otherwise, copy the entry over to this archive
			else
				tx_editor_->getArchive()->addEntry(entry, "patches", true);
		}
	}

	// End recording undo level
	undo_manager_->endRecord(true);

	// Refresh
	list_textures_->updateList();

	// Update variables
	modified_ = true;
}

// ----------------------------------------------------------------------------
// TextureXPanel::renameTexture
//
// Create standalone image entries of any selected textures
// ----------------------------------------------------------------------------
void TextureXPanel::renameTexture(bool each)
{
	// Get selected textures
	vector<long> selec_num = list_textures_->getSelection(true);
	vector<CTexture*> selection;

	if (!tx_entry_) return;

	// Go through selection
	for (unsigned a = 0; a < selec_num.size(); ++a)
		selection.push_back(texturex_.getTexture(selec_num[a]));

	// Check any are selected
	if (each || selection.size() == 1)
	{
		// If only one entry is selected, or "rename each" mode is desired, just do basic rename
		for (unsigned a = 0; a < selection.size(); a++)
		{
			// Prompt for a new name
			string new_name = wxGetTextFromUser("Enter new texture name: (* = unchanged)", "Rename", selection[a]->getName());
			if (wad_force_uppercase) new_name.MakeUpper();

			// Rename entry (if needed)
			if (!new_name.IsEmpty() && selection[a]->getName() != new_name)
			{
				selection[a]->setName(new_name);
				selection[a]->setState(1);
				modified_ = true;
			}
		}
	}
	else if (selection.size() > 1)
	{
		// Get a list of entry names
		wxArrayString names;
		for (unsigned a = 0; a < selection.size(); a++)
			names.push_back(selection[a]->getName());

		// Get filter string
		string filter = Misc::massRenameFilter(names);

		// Prompt for a new name
		string new_name = wxGetTextFromUser("Enter new texture name: (* = unchanged)", "Rename", filter);
		if (wad_force_uppercase) new_name.MakeUpper();

		// Apply mass rename to list of names
		if (!new_name.IsEmpty())
		{
			Misc::doMassRename(names, new_name);

			// Go through the list
			for (size_t a = 0; a < selection.size(); a++)
			{
				// Rename the entry (if needed)
				if (selection[a]->getName() != names[a])
				{
					selection[a]->setName(names[a]);
					selection[a]->setState(1);
					modified_ = true;
				}
			}
		}
	}
	Refresh();
}

// ----------------------------------------------------------------------------
// TextureXPanel::exportTexture
//
// Create standalone image entries of any selected textures
// ----------------------------------------------------------------------------
void TextureXPanel::exportTexture()
{
	// Get selected textures
	vector<long> selec_num = list_textures_->getSelection(true);
	vector<CTexture*> selection;

	if (!tx_entry_) return;

	//saveTEXTUREX();

	Archive* archive = tx_entry_->getParent();
	bool force_rgba = texture_editor_->blendRGBA();

	// Go through selection
	for (unsigned a = 0; a < selec_num.size(); ++a)
	{
		selection.push_back(texturex_.getTexture(selec_num[a]));
	}

	// Create gfx conversion dialog
	GfxConvDialog gcd(this);

	// Send selection to the gcd
	gcd.openTextures(selection, texture_editor_->palette(), archive, force_rgba);

	// Run the gcd
	gcd.ShowModal();

	// Show splash window
	UI::showSplash("Writing converted image data...", true);

	// Write any changes
	for (unsigned a = 0; a < selection.size(); a++)
	{
		// Update splash window
		UI::setSplashProgressMessage(selection[a]->getName());
		UI::setSplashProgress((float)a / (float)selection.size());

		// Skip if the image wasn't converted
		if (!gcd.itemModified(a))
			continue;

		// Get image and conversion info
		SImage* image = gcd.getItemImage(a);
		SIFormat* format = gcd.getItemFormat(a);

		// Write converted image back to entry
		MemChunk mc;
		format->saveImage(*image, mc, force_rgba ? NULL : gcd.getItemPalette(a));
		ArchiveEntry* lump = new ArchiveEntry;
		lump->importMemChunk(mc);
		lump->rename(selection[a]->getName());
		archive->addEntry(lump, "textures");
		EntryType::detectEntryType(lump);
		lump->setExtensionByType();
	}

	// Hide splash window
	UI::hideSplash();
}

// ----------------------------------------------------------------------------
// TextureXPanel::exportAsPNG
//
// Converts [texture] to a PNG image (if possible) and saves the PNG data to a
// file [filename]. Does not alter the texture data itself
// ----------------------------------------------------------------------------
bool TextureXPanel::exportAsPNG(CTexture* texture, string filename, bool force_rgba)
{
	// Check entry was given
	if (!texture)
		return false;

	// Create image from entry
	SImage image;
	if (!texture->toImage(image, nullptr, texture_editor_->palette(), force_rgba))
	{
		LOG_MESSAGE(1, "Error converting %s: %s", texture->getName(), Global::error);
		return false;
	}

	// Write png data
	MemChunk png;
	SIFormat* fmt_png = SIFormat::getFormat("png");
	if (!fmt_png->saveImage(image, png, texture_editor_->palette()))
	{
		LOG_MESSAGE(1, "Error converting %s", texture->getName());
		return false;
	}

	// Export file
	return png.exportFile(filename);
}

// ----------------------------------------------------------------------------
// TextureXPanel::extractTexture
//
// Create standalone image entries of any selected textures
// ----------------------------------------------------------------------------
void TextureXPanel::extractTexture()
{
	// Get selected textures
	vector<long> selec_num = list_textures_->getSelection(true);
	vector<CTexture*> selection;

	if (!tx_entry_) return;

	//saveTEXTUREX();

	Archive* archive = tx_entry_->getParent();
	bool force_rgba = texture_editor_->blendRGBA();

	// Go through selection
	for (unsigned a = 0; a < selec_num.size(); ++a)
	{
		selection.push_back(texturex_.getTexture(selec_num[a]));
	}


	// If we're just exporting one texture
	if (selection.size() == 1)
	{
		string name = Misc::lumpNameToFileName(selection[0]->getName());
		wxFileName fn(name);

		// Set extension
		fn.SetExt("png");

		// Run save file dialog
		SFileDialog::fd_info_t info;
		if (SFileDialog::saveFile(info, "Export Texture \"" + selection[0]->getName() + "\" as PNG", "PNG Files (*.png)|*.png", this, fn.GetFullName()))
		{
			// If a filename was selected, export it
			if (!exportAsPNG(selection[0], info.filenames[0], force_rgba))
			{
				wxMessageBox(S_FMT("Error: %s", Global::error), "Error", wxOK|wxICON_ERROR);
				return;
			}
		}

		return;
	}
	else
	{
		// Run save files dialog
		SFileDialog::fd_info_t info;
		if (SFileDialog::saveFiles(info, "Export Textures as PNG (Filename will be ignored)", "PNG Files (*.png)|*.png", this))
		{
			// Show splash window
			UI::showSplash("Saving converted image data...", true);

			// Go through the selection
			for (size_t a = 0; a < selection.size(); a++)
			{
				// Update splash window
				UI::setSplashProgressMessage(selection[a]->getName());
				UI::setSplashProgress((float)a / (float)selection.size());

				// Setup entry filename
				wxFileName fn(selection[a]->getName());
				fn.SetPath(info.path);
				fn.SetExt("png");

				// Do export
				exportAsPNG(selection[a], fn.GetFullPath(), force_rgba);
			}

			// Hide splash window
			UI::hideSplash();
		}
	}
}

// ----------------------------------------------------------------------------
// TextureXPanel::modifyOffsets
//
// Changes the offsets for each selected texture. Only for ZDoom!
// ----------------------------------------------------------------------------
bool TextureXPanel::modifyOffsets()
{
	if (!tx_entry_) return false;
	//saveTEXTUREX();

	// Create modify offsets dialog
	ModifyOffsetsDialog mod;
	mod.SetParent(this);
	mod.CenterOnParent();

	// Run the dialog
	if (mod.ShowModal() == wxID_CANCEL)
		return false;

	// Go through selection
	vector<long> selec_num = list_textures_->getSelection(true);
	for (unsigned a = 0; a < selec_num.size(); ++a)
	{
		// Get texture
		bool current = false;
		CTexture* ctex = texturex_.getTexture(selec_num[a]);
		if (ctex == tex_current_)
		{
			// Texture is currently open in the editor
			ctex = texture_editor_->texture();
			current = true;
		}

		// Calculate and apply new offsets
		point2_t offsets = mod.calculateOffsets(ctex->getOffsetX(), ctex->getOffsetY(), ctex->getWidth(), ctex->getHeight());
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

// ----------------------------------------------------------------------------
// TextureXPanel::onUndo
//
// Called when an action is undone
// ----------------------------------------------------------------------------
void TextureXPanel::onUndo(string action)
{
	list_textures_->updateList();
}

// ----------------------------------------------------------------------------
// TextureXPanel::onRedo
//
// Called when an action is redone
// ----------------------------------------------------------------------------
void TextureXPanel::onRedo(string action)
{
	list_textures_->updateList();
}

// ----------------------------------------------------------------------------
// TextureXPanel::handleAction
//
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// ----------------------------------------------------------------------------
bool TextureXPanel::handleAction(string id)
{
	// Don't handle if hidden
	if (!tx_editor_->IsShown() || !IsShown())
		return false;

	// Only interested in "txed_" events
	if (!id.StartsWith("txed_"))
		return false;

	// Handle action
	if (id == "txed_new")
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
	else
		return false;	// Not handled here

	return true;
}


// ----------------------------------------------------------------------------
//
// TextureXPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// TextureXPanel::onTextureListSelect
//
// Called when an item on the texture list is selected
// ----------------------------------------------------------------------------
void TextureXPanel::onTextureListSelect(wxListEvent& e)
{
	// Do nothing if multiple textures are selected
	if (list_textures_->GetSelectedItemCount() > 1)
	{
		tex_current_ = nullptr;
		undo_manager_->setResetPoint();
		texture_editor_->openTexture(tex_current_, &texturex_);
		return;
	}

	// Get selected texture
	CTexture* tex = texturex_.getTexture(list_textures_->getItemIndex(e.GetIndex()));

	// Save any changes to previous texture
	applyChanges();

	// Open texture in editor
	undo_manager_->setResetPoint();
	texture_editor_->openTexture(tex, &texturex_);

	// Set current texture
	tex_current_ = tex;
}

// ----------------------------------------------------------------------------
// TextureXPanel::onTextureListRightClick
//
// Called when an item on the texture list is right clicked
// ----------------------------------------------------------------------------
void TextureXPanel::onTextureListRightClick(wxListEvent& e)
{
	// Create context menu
	wxMenu context;
	wxMenu* texport = new wxMenu();
	SAction::fromId("txed_delete")->addToMenu(&context, true);
	context.AppendSeparator();
	SAction::fromId("txed_rename")->addToMenu(&context, true);
	if (list_textures_->GetSelectedItemCount() > 1)
		SAction::fromId("txed_rename_each")->addToMenu(&context, true);
	if (texturex_.getFormat() == TXF_TEXTURES)
		SAction::fromId("txed_offsets")->addToMenu(&context, true);
	SAction::fromId("txed_export")->addToMenu(texport, "Archive (as image)");
	SAction::fromId("txed_extract")->addToMenu(texport, "File");
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

// ----------------------------------------------------------------------------
// TextureXPanel::onTextureListKeyDown
//
// Called when a key is pressed in the texture list
// ----------------------------------------------------------------------------
void TextureXPanel::onTextureListKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	wxArrayString binds = KeyBind::getBinds(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Go through matching binds
	for (unsigned a = 0; a < binds.size(); a++)
	{
		string name = binds[a];

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

// ----------------------------------------------------------------------------
// TextureXPanel::onTextFilterChanged
//
// Called when the filter text is changed
// ----------------------------------------------------------------------------
void TextureXPanel::onTextFilterChanged(wxCommandEvent& e)
{
	// Filter the entry list
	list_textures_->setFilter(text_filter_->GetValue());

	e.Skip();
}

// ----------------------------------------------------------------------------
// TextureXPanel::onBtnClearFitler
//
// Called when the 'Clear Filter' button is clicked
// ----------------------------------------------------------------------------
void TextureXPanel::onBtnClearFitler(wxCommandEvent& e)
{
	text_filter_->SetValue(wxEmptyString);
	list_textures_->setFilter(wxEmptyString);
}

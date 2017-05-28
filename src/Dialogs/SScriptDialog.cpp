
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SScriptDialog.cpp
// Description: SLADE Script manager dialog
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
#include "SScriptDialog.h"
#include "Archive/ArchiveManager.h"
#include "General/Lua.h"
#include "UI/TextEditor/TextEditor.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
string SScriptDialog::prev_script_ = "";


// ----------------------------------------------------------------------------
// ScriptTreeItemData Class
//
// Just used to store ArchiveEntry pointers with wxTreeCtrl items
// ----------------------------------------------------------------------------
class ScriptTreeItemData : public wxTreeItemData
{
public:
	ScriptTreeItemData(ArchiveEntry* entry) : entry{ entry } {}
	ArchiveEntry* entry;
};


// ----------------------------------------------------------------------------
//
// SScriptDialog Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SScriptDialog::SScriptDialog
//
// SScriptDialog class constructor
// ----------------------------------------------------------------------------
SScriptDialog::SScriptDialog(wxWindow* parent) :
	SDialog(parent, "Script Manager", "script_manager", 800, 600)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Scripts Tree
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 1, wxEXPAND | wxALL, 10);
	tree_scripts_ = new wxTreeCtrl(
		this,
		-1,
		wxDefaultPosition,
		{ 200, -1 },
		wxTR_DEFAULT_STYLE | wxTR_NO_LINES | wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT
	);
	tree_scripts_->EnableSystemTheme();
	populateScriptsTree();
	hbox->Add(tree_scripts_, 0, wxEXPAND | wxRIGHT, 10);

	// Text Editor
	text_editor_ = new TextEditor(this, -1);
	text_editor_->SetText(prev_script_);
	text_editor_->setLanguage(TextLanguage::getLanguage("sladescript"));
	hbox->Add(text_editor_, 1, wxEXPAND);

	// Buttons
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
	btn_run_ = new wxButton(this, -1, "Run Script");
	hbox->AddStretchSpacer();
	hbox->Add(btn_run_, 0, wxEXPAND);

	// Bind events
	bindEvents();

	SetMinSize({ 500, 400 });
	Layout();
	CenterOnParent();
}

// ----------------------------------------------------------------------------
// SScriptDialog::~SScriptDialog
//
// SScriptDialog class destructor
// ----------------------------------------------------------------------------
SScriptDialog::~SScriptDialog()
{
}

// ----------------------------------------------------------------------------
// SScriptDialog::bindEvents
//
// Bind events for wx controls
// ----------------------------------------------------------------------------
void SScriptDialog::bindEvents()
{
	// 'Run' button click
	btn_run_->Bind(wxEVT_BUTTON, [=](wxCommandEvent)
	{
		prev_script_ = text_editor_->GetText();
		// TODO: Show error in message box
		if (!Lua::run(prev_script_))
			wxMessageBox("See Console Log", "Script Error", wxOK | wxICON_ERROR, this);
	});

	// Tree item activate
	tree_scripts_->Bind(wxEVT_TREE_ITEM_ACTIVATED, [=](wxTreeEvent e)
	{
		auto data = (ScriptTreeItemData*)tree_scripts_->GetItemData(e.GetItem());
		if (data && data->entry)
			text_editor_->loadEntry(data->entry);
	});
}

// ----------------------------------------------------------------------------
// SScriptDialog::populateScriptsTree
//
// Loads scripts from slade.pk3 into the scripts tree control
// ----------------------------------------------------------------------------
void SScriptDialog::populateScriptsTree()
{
	// Clear tree
	tree_scripts_->DeleteAllItems();

	// Get 'scripts' dir of slade.pk3
	auto scripts_dir = theArchiveManager->programResourceArchive()->getDir("scripts");
	if (!scripts_dir)
		return;

	// Recursive function to populate the tree
	std::function<void(wxTreeCtrl*, wxTreeItemId, ArchiveTreeNode*)> addToTree =
		[&](wxTreeCtrl* tree, wxTreeItemId node, ArchiveTreeNode* dir)
	{
		// Add subdirs
		for (unsigned a = 0; a < dir->nChildren(); a++)
		{
			auto subdir = (ArchiveTreeNode*)dir->getChild(a);
			auto subnode = tree->AppendItem(node, subdir->getName());
			addToTree(tree, subnode, subdir);
		}

		// Add script files
		for (unsigned a = 0; a < dir->numEntries(); a++)
		{
			auto entry = dir->getEntry(a);
			if (entry->getName(true) != "init")
				tree->AppendItem(node, entry->getName(true), -1, -1, new ScriptTreeItemData(entry));
		}
	};

	// Populate from root
	auto root = tree_scripts_->AddRoot("Scripts");
	addToTree(tree_scripts_, root, scripts_dir);
}

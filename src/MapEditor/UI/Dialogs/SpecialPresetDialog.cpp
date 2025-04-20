
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SpecialPresetDialog.cpp
// Description: A Dialog showing a tree of all currently loaded special presets
//              from the game configuration for user selection
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
#include "SpecialPresetDialog.h"
#include "Game/Configuration.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Helper Classes
//
// -----------------------------------------------------------------------------
namespace slade
{
class SpecialPresetData : public wxClientData
{
public:
	SpecialPresetData(const game::SpecialPreset& preset) : preset_{ preset } {}

	const game::SpecialPreset& preset() const { return preset_; }

private:
	game::SpecialPreset const& preset_;
};

// -----------------------------------------------------------------------------
// SpecialPresetTreeView Class
//
// A wxDataViewTreeCtrl specialisation showing the special presets and groups
// in a tree structure
// -----------------------------------------------------------------------------
class SpecialPresetTreeView : public wxDataViewTreeCtrl
{
public:
	SpecialPresetTreeView(wxWindow* parent) :
		wxDataViewTreeCtrl{ parent, -1 },
		root_{ wxDataViewItem(nullptr) },
		parent_dialog_{ nullptr }
	{
		// Computing the minimum width of the tree is slightly complicated, since
		// wx doesn't expose it to us directly
		wxClientDC dc(this);
		dc.SetFont(GetFont());
		wxSize textsize;

		// Populate tree
		addPresets(game::customSpecialPresets(), textsize, dc);           // User custom presets
		addPresets(game::configuration().specialPresets(), textsize, dc); // From game configuration
		wxDataViewCtrl::Expand(root_);

		// Bind events
		Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, [&](wxDataViewEvent& e) { e.Veto(); });
		Bind(
			wxEVT_DATAVIEW_ITEM_ACTIVATED,
			[&](wxDataViewEvent& e)
			{
				if (GetChildCount(e.GetItem()) > 0)
				{
					// Expand if group node
					Expand(e.GetItem());
					e.Skip();
				}
				else if (parent_dialog_)
					parent_dialog_->EndModal(wxID_OK);
			});

		// 64 is an arbitrary fudge factor -- should be at least the width of a
		// scrollbar plus the expand icons plus any extra padding
		int min_width = textsize.GetWidth() + GetIndent() + ui::scalePx(64);
		wxWindowBase::SetMinSize(wxSize(min_width, ui::scalePx(200)));
	}

	game::SpecialPreset selectedPreset() const
	{
		// Get data from selected item
		auto sel_data = dynamic_cast<SpecialPresetData*>(GetItemData(GetSelection()));
		if (sel_data)
			return sel_data->preset();

		return {};
	}

	void setParentDialog(wxDialog* dlg) { parent_dialog_ = dlg; }

private:
	wxDataViewItem root_;
	wxDialog*      parent_dialog_ = nullptr;

	struct Group
	{
		string         name;
		wxDataViewItem item;
		Group(wxDataViewItem item, const string& name) : name{ name }, item{ item } {}
	};
	vector<Group> groups_;

	wxDataViewItem getGroup(const string& group)
	{
		// Check if group was already made
		for (auto& g : groups_)
		{
			if (group == g.name)
				return g.item;
		}

		// Split group into subgroups
		auto path = strutil::splitV(group, '/');

		// Create group needed
		auto   current = root_;
		string fullpath;
		for (unsigned p = 0; p < path.size(); p++)
		{
			if (p > 0)
				fullpath += "/";
			fullpath += path[p];

			bool found = false;
			for (auto& g : groups_)
			{
				if (g.name == fullpath)
				{
					current = g.item;
					found   = true;
					break;
				}
			}

			if (!found)
			{
				current = AppendContainer(current, wxutil::strFromView(path[p]), -1, 1);
				groups_.emplace_back(current, fullpath);
			}
		}

		return current;
	}

	void addPresets(const vector<game::SpecialPreset>& presets, wxSize& textsize, wxClientDC& dc)
	{
		for (auto& preset : presets)
		{
			auto item = AppendItem(getGroup(preset.group), wxString::FromUTF8(preset.name));
			SetItemData(item, new SpecialPresetData(preset));
			textsize.IncTo(dc.GetTextExtent(wxString::FromUTF8(preset.name)));
		}
	}
};
} // namespace slade


// -----------------------------------------------------------------------------
//
// SpecialPresetDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SpecialPresetDialog class constructor
// -----------------------------------------------------------------------------
SpecialPresetDialog::SpecialPresetDialog(wxWindow* parent) : SDialog{ parent, "Special Presets", "special_presets" }
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Presets tree
	tree_presets_ = new SpecialPresetTreeView(this);
	tree_presets_->setParentDialog(this);
	sizer->Add(tree_presets_, 1, wxALL | wxEXPAND, ui::padLarge());

	// OK button
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::padLarge());
	hbox->AddStretchSpacer(1);
	auto btn_ok = new wxButton(this, -1, wxS("OK"));
	hbox->Add(btn_ok, 0, wxEXPAND | wxRIGHT, ui::pad());
	btn_ok->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) { EndModal(wxID_OK); });

	// Cancel button
	auto btn_cancel = new wxButton(this, -1, wxS("Cancel"));
	hbox->Add(btn_cancel, 0, wxEXPAND);
	btn_cancel->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) { EndModal(wxID_CANCEL); });
}

// -----------------------------------------------------------------------------
// Returns the currently selected special preset
// -----------------------------------------------------------------------------
game::SpecialPreset SpecialPresetDialog::selectedPreset() const
{
	return tree_presets_->selectedPreset();
}

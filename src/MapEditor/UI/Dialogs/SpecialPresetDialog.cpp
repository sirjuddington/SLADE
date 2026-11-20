
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Game/SpecialPreset.h"
#include "Graphics/Icons.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"
#include "wx/statbmp.h"

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
	SpecialPresetData(const game::SpecialPreset& preset, bool custom = false) : preset_{ preset }, custom_{ custom } {}

	const game::SpecialPreset& preset() const { return preset_; }
	bool                       custom() const { return custom_; }

private:
	game::SpecialPreset preset_;
	bool                custom_ = false;
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
	// -------------------------------------------------------------------------
	// Constructor
	// -------------------------------------------------------------------------
	SpecialPresetTreeView(wxWindow* parent) :
		wxDataViewTreeCtrl{ parent, -1, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER },
		root_{ wxDataViewItem(nullptr) }
	{
		// Computing the minimum width of the tree is slightly complicated,
		// since wx doesn't expose it to us directly
		wxClientDC dc(this);
		dc.SetFont(GetFont());
		wxSize textsize;

		// Populate tree
		addPresets(game::customSpecialPresets(), textsize, dc, true);            // User custom presets
		addPresets(game::configuration().specialPresets(), textsize, dc, false); // From game configuration
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

		// 32 is an arbitrary fudge factor -- should be at least the width of a
		// scrollbar plus the expand icons plus any extra padding
		int min_width = textsize.GetWidth() + GetIndent() + 32;
		wxWindowBase::SetMinSize(FromDIP(wxSize(min_width, 200)));
	}

	// -------------------------------------------------------------------------
	// Returns the SpecialPreset of the currently selected item, or an empty
	// preset if none is selected
	// -------------------------------------------------------------------------
	game::SpecialPreset selectedPreset() const
	{
		// Get data from selected item
		if (auto selection = GetSelection(); selection.IsOk())
			if (auto sel_data = dynamic_cast<SpecialPresetData*>(GetItemData(selection)))
				return sel_data->preset();

		return {};
	}

	// -------------------------------------------------------------------------
	// Returns true if the currently selected item is a custom preset
	// -------------------------------------------------------------------------
	bool customSelected() const
	{
		if (auto selection = GetSelection(); selection.IsOk())
			if (auto sel_data = dynamic_cast<SpecialPresetData*>(GetItemData(selection)))
				return sel_data->custom();

		return false;
	}

	// -------------------------------------------------------------------------
	// Filters the visible presets, based on a filter string
	// -------------------------------------------------------------------------
	void filterPresets(string_view filter)
	{
		DeleteAllItems();
		groups_.clear();

		wxClientDC dc(this);
		dc.SetFont(GetFont());
		wxSize textsize;
		addPresets(game::customSpecialPresets(), textsize, dc, true, filter);
		addPresets(game::configuration().specialPresets(), textsize, dc, false, filter);

		// Expand all groups if a filter was specified
		if (!filter.empty())
			wxutil::expandAll(this);

		// Select and show the first available preset
		if (auto first = firstPreset(root_); first.IsOk())
		{
			vector<wxDataViewItem> parents;
			for (auto parent = GetItemParent(first); parent.IsOk(); parent = GetItemParent(parent))
				parents.push_back(parent);
			for (auto& parent : std::views::reverse(parents))
				Expand(parent);
			Select(first);
			EnsureVisible(first);
		}
	}

	// -------------------------------------------------------------------------
	// Deletes the currently selected item (and any resulting empty groups)
	// -------------------------------------------------------------------------
	void deleteSelected()
	{
		auto selected = GetSelection();
		if (!selected.IsOk())
			return;

		DeleteItem(selected);

		// Delete any resulting empty groups
		for (auto it = groups_.begin(); it != groups_.end();)
		{
			if (it->item.IsOk() && GetChildCount(it->item) == 0)
			{
				DeleteItem(it->item);
				groups_.erase(it);
				it = groups_.begin();
			}
			else
				++it;
		}
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

	// -------------------------------------------------------------------------
	// Returns the first preset item under [parent], or an invalid
	// wxDataViewItem if none is found
	// -------------------------------------------------------------------------
	wxDataViewItem firstPreset(const wxDataViewItem& parent) const
	{
		wxDataViewItemArray children;
		GetStore()->GetChildren(parent, children);
		for (const auto& child : children)
		{
			if (GetItemData(child))
				return child;
			if (auto preset = firstPreset(child); preset.IsOk())
				return preset;
		}
		return {};
	}

	// -------------------------------------------------------------------------
	// Returns the parent wxDataViewItem representing preset group [group]
	// -------------------------------------------------------------------------
	wxDataViewItem getGroup(const string& group)
	{
		// Check if group was already made
		for (auto& g : groups_)
			if (group == g.name)
				return g.item;

		// Split group into subgroups
		auto path = strutil::splitV(group, '/');

		// Create group needed
		auto   current = root_;
		string fullpath;
		for (unsigned p = 0; p < path.size(); p++)
		{
			if (p > 0)
				fullpath += '/';
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
				current = AppendContainer(current, wxutil::strFromView(path[p]));
				groups_.emplace_back(current, fullpath);
			}
		}

		return current;
	}

	// -------------------------------------------------------------------------
	// Adds [presets] to the tree, under their respective groups, and updates
	// [textsize] with the maximum text size of any preset name
	// -------------------------------------------------------------------------
	void addPresets(
		const vector<game::SpecialPreset>& presets,
		wxSize&                            textsize,
		const wxClientDC&                  dc,
		bool                               custom,
		string_view                        filter = {})
	{
		for (auto& preset : presets)
		{
			if (!strutil::containsCI(preset.name, filter))
				continue;

			auto group = custom ? "Custom/" + preset.group : preset.group;
			if (strutil::endsWith(group, '/'))
				group.pop_back();

			auto item = AppendItem(getGroup(group), wxString::FromUTF8(preset.name));
			SetItemData(item, new SpecialPresetData(preset, custom));
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
	auto lh    = ui::LayoutHelper(this);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Filter
	auto hbox        = new wxBoxSizer(wxHORIZONTAL);
	auto filter_icon = new wxStaticBitmap(this, -1, icons::getIcon(icons::Type::Any, "filter"));
	hbox->Add(filter_icon, lh.sfWithBorder(0, wxRIGHT).Center());
	text_filter_ = new wxTextCtrl(this, -1);
	hbox->Add(text_filter_, wxSizerFlags(1).Expand());
	sizer->Add(hbox, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxTOP).Expand());

	// Presets tree
	tree_presets_ = new SpecialPresetTreeView(this);
	tree_presets_->setParentDialog(this);
	sizer->AddSpacer(lh.pad());
	sizer->Add(tree_presets_, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT).Expand());

	// Delete Preset button
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithLargeBorder(0).Expand());
	auto btn_delete = new wxButton(this, -1, wxS("Delete"));
	hbox->Add(btn_delete, lh.sfWithBorder(0, wxRIGHT).Expand());
	btn_delete->Bind(
		wxEVT_BUTTON,
		[this, btn_delete](wxCommandEvent& e)
		{
			deleteSelectedCustomPreset();
			btn_delete->Enable(false);
		});

	// Apply button
	auto btn_ok = new wxButton(this, -1, wxS("Apply"));
	hbox->AddStretchSpacer(1);
	hbox->Add(btn_ok, lh.sfWithBorder(0, wxRIGHT).Expand());
	btn_ok->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) { EndModal(wxID_OK); });

	// Cancel button
	auto btn_cancel = new wxButton(this, -1, wxS("Cancel"));
	hbox->Add(btn_cancel, wxSizerFlags().Expand());
	btn_cancel->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) { EndModal(wxID_CANCEL); });

	// Setup button states
	btn_ok->Enable(false);
	btn_delete->Enable(false);
	Bind(
		wxEVT_DATAVIEW_SELECTION_CHANGED,
		[&, btn_ok, btn_delete](wxDataViewEvent& e)
		{
			// Only enable the apply button if a preset is selected
			btn_ok->Enable(tree_presets_->selectedPreset().special >= 0);

			// Only enable the delete button if a custom preset is selected
			btn_delete->Enable(tree_presets_->customSelected());
		});

	// Bind events
	text_filter_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { timer_filter_.StartOnce(300); });
	timer_filter_.Bind(
		wxEVT_TIMER,
		[this, btn_ok, btn_delete](wxTimerEvent&)
		{
			tree_presets_->filterPresets(text_filter_->GetValue().utf8_string());
			btn_ok->Enable(tree_presets_->selectedPreset().special >= 0);
			btn_delete->Enable(tree_presets_->customSelected());
		});

	wxWindowBase::SetMinClientSize(sizer->GetMinSize());
	CenterOnParent();

	text_filter_->SetFocus();
}

// -----------------------------------------------------------------------------
// Returns the currently selected special preset
// -----------------------------------------------------------------------------
game::SpecialPreset SpecialPresetDialog::selectedPreset() const
{
	return tree_presets_->selectedPreset();
}

// -----------------------------------------------------------------------------
// Deletes the currently selected custom special preset
// -----------------------------------------------------------------------------
void SpecialPresetDialog::deleteSelectedCustomPreset() const
{
	if (!tree_presets_->customSelected())
		return;

	auto preset = tree_presets_->selectedPreset();
	if (game::deleteCustomSpecialPreset(preset.group, preset.name))
		tree_presets_->deleteSelected();
}

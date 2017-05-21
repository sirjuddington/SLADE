
#include "Main.h"
#include "SpecialPresetDialog.h"
#include "Game/Configuration.h"

namespace
{
	class SpecialPresetData : public wxClientData
	{
	public:
		SpecialPresetData(const Game::SpecialPreset& preset) : preset_{ preset } {}

		const Game::SpecialPreset& preset() const { return preset_; }

	private:
		Game::SpecialPreset const&	preset_;
	};
}

// A wxDataViewTreeCtrl specialisation showing the special presets and groups in a tree structure
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
		for (auto& preset : Game::configuration().specialPresets())
		{
			auto item = AppendItem(getGroup(preset.group), preset.name);
			SetItemData(item, new SpecialPresetData(preset));
			textsize.IncTo(dc.GetTextExtent(preset.name));
		}
		wxDataViewCtrl::Expand(root_);

		// Bind events
		Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, [&](wxDataViewEvent& e) { e.Veto(); });
		Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [&](wxDataViewEvent& e)
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
		int min_width = textsize.GetWidth() + GetIndent() + 64;
		wxWindowBase::SetMinSize(wxSize(min_width, 200));
	}

	Game::SpecialPreset selectedPreset() const
	{
		// Get data from selected item
		auto sel_data = (SpecialPresetData*)GetItemData(GetSelection());
		if (sel_data)
			return sel_data->preset();

		return {};
	}

	void setParentDialog(wxDialog* dlg)
	{
		parent_dialog_ = dlg;
	}

private:
	wxDataViewItem	root_;
	wxDialog*		parent_dialog_;

	struct Group
	{
		string			name;
		wxDataViewItem	item;
		Group(wxDataViewItem item, string name) : name{ name }, item{ item } {}
	};
	vector<Group> groups;

	wxDataViewItem getGroup(string group)
	{
		// Check if group was already made
		for (unsigned a = 0; a < groups.size(); a++)
		{
			if (group == groups[a].name)
				return groups[a].item;
		}

		// Split group into subgroups
		auto path = wxSplit(group, '/');

		// Create group needed
		auto current = root_;
		string fullpath = "";
		for (unsigned p = 0; p < path.size(); p++)
		{
			if (p > 0) fullpath += "/";
			fullpath += path[p];

			bool found = false;
			for (unsigned a = 0; a < groups.size(); a++)
			{
				if (groups[a].name == fullpath)
				{
					current = groups[a].item;
					found = true;
					break;
				}
			}

			if (!found)
			{
				current = AppendContainer(current, path[p], -1, 1);
				groups.push_back({ current, fullpath });
			}
		}

		return current;
	}
};

SpecialPresetDialog::SpecialPresetDialog(wxWindow* parent) :
	SDialog{ parent, "Special Presets", "special_presets" },
	tree_presets_{ nullptr }
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Presets tree
	tree_presets_ = new SpecialPresetTreeView(this);
	tree_presets_->setParentDialog(this);
	sizer->Add(tree_presets_, 1, wxALL | wxEXPAND, 10);

	// OK button
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
	hbox->AddStretchSpacer(1);
	auto btn_ok = new wxButton(this, -1, "OK");
	hbox->Add(btn_ok, 0, wxEXPAND | wxRIGHT, 4);
	btn_ok->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) { EndModal(wxID_OK); });

	// Cancel button
	auto btn_cancel = new wxButton(this, -1, "Cancel");
	hbox->Add(btn_cancel, 0, wxEXPAND);
	btn_cancel->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) { EndModal(wxID_CANCEL); });
}

Game::SpecialPreset SpecialPresetDialog::selectedPreset() const
{
	return tree_presets_->selectedPreset();
}

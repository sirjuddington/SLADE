
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SAuiToolBar.cpp
// Description: A custom wxAuiToolBar with various extra functionality including
//              the ability to hide items, load layout from a JSON definition,
//              handle SActions, etc.
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
#include "SAuiToolBar.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "General/SAction.h"
#include "General/SActionHandler.h"
#include "Graphics/Icons.h"
#include "SAuiToolBarArt.h"
#include "Utility/StringUtils.h"
#include "WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, toolbar_size, 16, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// SAuiToolBar Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SAuiToolBar class constructor
// -----------------------------------------------------------------------------
SAuiToolBar::SAuiToolBar(wxWindow* parent, bool vertical, bool main_toolbar, wxAuiManager* aui_mgr) :
	wxAuiToolBar(
		parent,
		wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize,
		wxAUI_TB_PLAIN_BACKGROUND | (vertical ? wxAUI_TB_VERTICAL : wxAUI_TB_HORIZONTAL)),
	aui_mgr_{ aui_mgr }
{
	SetArtProvider(new SAuiToolBarArt(this, main_toolbar));
	SetToolSeparation(FromDIP(12));
	wxAuiToolBar::SetDoubleBuffered(true);

	if (main_toolbar)
		SetMargins(FromDIP(wxSize(5, 4)));
	else
		SetMargins(0, 1, 0, 0);

	// Set background colour
	wxWindow::SetBackgroundColour(parent->GetBackgroundColour());
#ifdef __WXMSW__
	if (main_toolbar)
	{
		if (app::isDarkTheme())
			wxWindow::SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_MENU));
		else if (global::win_version_major >= 10)
			wxWindow::SetBackgroundColour(wxColor(250, 250, 250));
	}
#endif

	// Popup associated menu when a dropdown button is clicked
	Bind(
		wxEVT_AUITOOLBAR_TOOL_DROPDOWN,
		[this](wxAuiToolBarEvent& e)
		{
			auto id = e.GetId();
			if (auto item = itemByWxId(id); item && item->menu)
			{
				PopupMenu(item->menu, e.GetItemRect().GetBottomLeft());
				SetPressedItem(nullptr); // Need to clear this or the button will stay pressed after the menu closes
			}
			else
				e.Skip();
		});

	// If an item is clicked and has an associated SAction, handle it
	Bind(
		wxEVT_MENU,
		[this](wxCommandEvent& e)
		{
			if (auto sa = SAction::fromWxId(e.GetId()); sa->wxId() == e.GetId())
				SActionHandler::doAction(sa->id());
			else
				e.Skip();
		});

	// Update relevant checked items when an SAction is checked/unchecked
	sc_saction_checked_ = SAction::signals().checked_changed.connect(
		[this](SAction& action)
		{
			if (auto item = itemById(action.id()); item && item->aui_item)
			{
				if (action.isChecked())
					item->aui_item->SetState(item->aui_item->GetState() | wxAUI_BUTTON_STATE_CHECKED);
				else
					item->aui_item->SetState(item->aui_item->GetState() & ~wxAUI_BUTTON_STATE_CHECKED);
				Refresh();
			}
		});
}

// -----------------------------------------------------------------------------
// SAuiToolBar class destructor
// -----------------------------------------------------------------------------
SAuiToolBar::~SAuiToolBar() {}

// -----------------------------------------------------------------------------
// Adds an SAction [action_id] to the toolbar.
// If [show_name] is true, the action's name will be shown beside the icon.
// If [icon] is non-empty, it will be used as the icon instead of the action's
// default icon.
// -----------------------------------------------------------------------------
wxAuiToolBarItem* SAuiToolBar::addAction(const string& action_id, bool show_name, string_view icon)
{
	// Get SAction to add
	auto sa = SAction::fromId(action_id);
	if (!sa)
	{
		log::warning("SAuiToolBar::addAction: Action '{}' not found", action_id);
		return nullptr;
	}

	// Help text
	auto sc        = sa->shortcutText();
	auto help_text = sa->helpText();
	if (!sc.empty())
		help_text += fmt::format(" (Shortcut: {})", sc);

	// Tooltip text
	wxString tooltip;
	auto     sa_text = strutil::replace(sa->text(), "&", "");
	if (!show_name)
	{
		auto tip = sa_text;
		if (!sc.empty())
			tip += fmt::format(" (Shortcut: {})", sc);
		tooltip = wxString::FromUTF8(tip);
	}
	else if (!sc.empty())
		tooltip = WX_FMT("Shortcut: {}", sc);

	// Get icon
	auto icon_bmp = icons::getIcon(icons::Type::Any, icon.empty() ? sa->iconName() : icon, toolbar_size);

	auto tool = AddTool(
		sa->wxId(),
		wxString::FromUTF8(sa_text),
		icon_bmp,
		icon_bmp,
		wxITEM_NORMAL,
		tooltip,
		wxString::FromUTF8(help_text),
		nullptr);

	setItemChecked(tool, sa->isChecked());

	items_.push_back({ .id = action_id, .action = sa, .aui_item = tool, .wx_id = sa->wxId(), .show_text = show_name });

	return tool;
}

// -----------------------------------------------------------------------------
// Adds a (non-SAction) button to the toolbar:
// [button_id] is an identifier for the button, used in other functions.
// [text] is the button text/tooltip.
// [icon] is the button icon name (see icons::getIcon).
// [help_text] is shown in the status bar when the button is hovered.
// [menu] is an optional dropdown menu for the button.
// If [show_name] is true, the button [text] will be shown beside the icon.
// -----------------------------------------------------------------------------
wxAuiToolBarItem* SAuiToolBar::addButton(
	const string& button_id,
	string_view   text,
	string_view   icon,
	string_view   help_text,
	wxMenu*       menu,
	bool          show_name)
{
	auto id       = SAction::nextWxId();
	auto icon_bmp = icons::getIcon(icons::Type::Any, icon, toolbar_size);
	auto tool     = AddTool(
        id,
        wxutil::strFromView(text),
        icon_bmp,
        icon_bmp,
        wxITEM_NORMAL,
        wxutil::strFromView(text),
        wxutil::strFromView(help_text),
        nullptr);

	if (menu)
		tool->SetHasDropDown(true);

	items_.push_back({ .id = button_id, .aui_item = tool, .menu = menu, .wx_id = id, .show_text = show_name });

	return tool;
}

// -----------------------------------------------------------------------------
// Creates a 'group' named [group_name] containing the items with IDs in
// [item_ids].
// A group can be shown/hidden as a whole using showGroup() and enabled/disabled
// using enableGroup().
// -----------------------------------------------------------------------------
void SAuiToolBar::groupItems(string_view group_name, const vector<string>& item_ids)
{
	Group* group = nullptr;
	for (auto& g : groups_)
		if (g.name == group_name)
		{
			group = &g;
			break;
		}

	if (!group)
	{
		groups_.emplace_back();
		group       = &groups_.back();
		group->name = group_name;
	}

	group->items = item_ids;
}

// -----------------------------------------------------------------------------
// Sets the associated dropdown [menu] for the button with ID [button_id].
// -----------------------------------------------------------------------------
void SAuiToolBar::setButtonDropdownMenu(string_view button_id, wxMenu* menu)
{
	for (auto& dm : dropdown_menus_)
		if (dm.item_id == button_id)
			dm.menu = menu;

	if (auto item = itemById(button_id); item && item->aui_item)
	{
		item->menu = menu;
		item->aui_item->SetHasDropDown(menu != nullptr);
	}
}

// -----------------------------------------------------------------------------
// Sets the icon for the button with ID [button_id] to [icon].
// -----------------------------------------------------------------------------
void SAuiToolBar::setButtonIcon(string_view button_id, string_view icon)
{
	if (auto item = itemById(button_id); item && item->aui_item)
	{
		auto icon_bmp = icons::getIcon(icons::Type::Any, icon);
		item->aui_item->SetBitmap(icon_bmp);
	}
}

// -----------------------------------------------------------------------------
// Returns true if the toolbar item with ID [id] is enabled,
// false if disabled or not found.
// -----------------------------------------------------------------------------
bool SAuiToolBar::itemEnabled(string_view id)
{
	if (auto item = itemById(id); item && item->aui_item)
		return itemEnabled(item->aui_item);

	return false;
}

// -----------------------------------------------------------------------------
// Enables/disables the toolbar item with ID [id].
// If [refresh] is true, the toolbar will be refreshed after enabling/disabling
// the item.
// -----------------------------------------------------------------------------
void SAuiToolBar::enableItem(string_view id, bool enable, bool refresh)
{
	bool changed = false;

	if (auto item = itemById(id); item)
	{
		changed = itemEnabled(item->aui_item) != enable;
		EnableTool(item->wx_id, enable);
	}
	else
		return;

	if (!changed)
		return;

	if (layout_)
		for (auto& j_item : *layout_)
			if (j_item.value("id", "") == id)
				j_item["enabled"] = enable;

	if (refresh)
		Refresh();
}

// -----------------------------------------------------------------------------
// Enables/disables all items in the toolbar group named [group].
// If [refresh] is true, the toolbar will be refreshed after enabling/disabling
// the items.
// -----------------------------------------------------------------------------
void SAuiToolBar::enableGroup(string_view group, bool enable, bool refresh)
{
	auto g = groupByName(group);
	if (!g)
		return;

	for (const auto& item_id : g->items)
		enableItem(item_id, enable, false);

	if (refresh)
		Refresh();
}

// -----------------------------------------------------------------------------
// Shows/hides the toolbar item with ID [id].
// If [refresh] is true, the toolbar will be recreated after showing/hiding the
// item.
// -----------------------------------------------------------------------------
void SAuiToolBar::showItem(string_view id, bool show, bool refresh)
{
	if (!layout_)
		return;

	for (auto& j_item : *layout_)
		if (j_item.value("id", "") == id)
			j_item["hidden"] = !show;

	if (refresh)
		createFromLayout();
}

// -----------------------------------------------------------------------------
// Shows/hides all items in the toolbar group named [group].
// If [refresh] is true, the toolbar will be recreated after showing/hiding the
// items.
// -----------------------------------------------------------------------------
void SAuiToolBar::showGroup(string_view group, bool show, bool refresh)
{
	if (!layout_)
		return;

	auto g = groupByName(group);
	if (!g)
		return;

	for (const auto& item_id : g->items)
	{
		for (auto& j_item : *layout_)
			if (j_item.value("id", "") == item_id)
				j_item["hidden"] = !show;
	}

	if (refresh)
		createFromLayout();
}

// -----------------------------------------------------------------------------
// Returns true if the toolbar item with ID [id] is checked.
// -----------------------------------------------------------------------------
bool SAuiToolBar::itemChecked(string_view id) const
{
	if (auto item = itemById(id); item && item->aui_item)
		return (item->aui_item->GetState() & wxAUI_BUTTON_STATE_CHECKED) != 0;

	return false;
}

// -----------------------------------------------------------------------------
// Sets the checked state of the toolbar item with ID [id] to [checked].
// -----------------------------------------------------------------------------
void SAuiToolBar::setItemChecked(string_view id, bool checked)
{
	if (auto item = itemById(id); item && item->aui_item)
		setItemChecked(item->aui_item, checked);
}

// -----------------------------------------------------------------------------
// Toggles the checked state of the toolbar item with ID [id].
// Returns the new checked state.
// -----------------------------------------------------------------------------
bool SAuiToolBar::toggleItemChecked(string_view id)
{
	if (auto item = itemById(id); item && item->aui_item)
	{
		auto checked = !(item->aui_item->GetState() & wxAUI_BUTTON_STATE_CHECKED);
		setItemChecked(item->aui_item, checked);
		return checked;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Registers a custom control with identifier [id].
// The control will then be available to be added from a JSON layout definition.
// -----------------------------------------------------------------------------
void SAuiToolBar::registerCustomControl(const string& id, wxControl* control)
{
	for (auto& custom_control : custom_controls_)
		if (custom_control.name == id)
		{
			custom_control.control = control;
			return;
		}

	custom_controls_.emplace_back(id, control);
}

// -----------------------------------------------------------------------------
// Registers a dropdown [menu] for the button with ID [button_id].
// The menu will then be available to be added from a JSON layout definition.
// -----------------------------------------------------------------------------
void SAuiToolBar::registerDropdownMenu(const string& button_id, wxMenu* menu)
{
	for (auto& m : dropdown_menus_)
		if (m.item_id == button_id)
		{
			m.menu = menu;
			return;
		}

	dropdown_menus_.emplace_back(button_id, menu);
}

// -----------------------------------------------------------------------------
// Loads a toolbar layout from a JSON string [json].
// If [create] is true, the toolbar will be (re)created from the layout after
// loading.
// -----------------------------------------------------------------------------
void SAuiToolBar::loadLayout(string_view json, bool create)
{
	auto j = jsonutil::parse(json);

	if (!j.is_discarded())
	{
		if (j.contains("groups"))
		{
			groups_.clear();
			for (const auto& j_group : j.at("groups"))
			{
				groups_.emplace_back(j_group.at("name").get<string>());
				groups_.back().can_hide = j_group.value("can_hide", false);
				groups_.back().items    = j_group.at("item_ids").get<vector<string>>();
			}
		}

		if (j.contains("items"))
		{
			layout_ = std::make_unique<Json>(j.at("items"));
			if (create)
				createFromLayout();
		}
	}
}

// -----------------------------------------------------------------------------
// Loads a toolbar layout from the program resources.
// [entry_name] is the name of the resource entry (without path or extension).
// If [create] is true, the toolbar will be (re)created from the layout after
// loading.
// -----------------------------------------------------------------------------
void SAuiToolBar::loadLayoutFromResource(string_view entry_name, bool create)
{
	auto entry_path    = fmt::format("toolbars/{}.json", entry_name);
	auto toolbar_entry = app::programResource()->entryAtPath(entry_path);
	if (toolbar_entry)
		loadLayout(toolbar_entry->data().asString(), create);
	else
		log::error("SAuiToolBar::loadLayoutFromResource: Toolbar resource '{}' not found", entry_path);
}

// -----------------------------------------------------------------------------
// (Re)creates the toolbar from the currently loaded layout.
// -----------------------------------------------------------------------------
void SAuiToolBar::createFromLayout()
{
	if (!layout_)
		return;

	// Clear existing items
	Clear();
	items_.clear();

	// Hide all custom controls
	for (const auto& custom_control : custom_controls_)
		custom_control.control->Hide();

	// Add items from layout
	for (const auto& j_item : *layout_)
	{
		// Ignore if hidden
		if (j_item.value("hidden", false))
			continue;

		auto type = j_item.at("type").get<string>();

		// Action
		if (type == "action")
		{
			if (j_item.contains("label"))
				AddLabel(-1, wxString::FromUTF8(j_item.at("label").get<string>()));

			auto aui_item = addAction(
				j_item.at("id").get<string>(), j_item.value("show_text", false), j_item.value("icon", ""));

			EnableTool(aui_item->GetId(), j_item.value("enabled", true));
		}

		// Button
		else if (type == "button")
		{
			if (j_item.contains("label"))
				AddLabel(-1, wxString::FromUTF8(j_item.at("label").get<string>()));

			auto button_id = j_item.at("id").get<string>();

			wxMenu* menu = nullptr;
			for (const auto& dm : dropdown_menus_)
				if (dm.item_id == button_id)
					menu = dm.menu;

			auto aui_item = addButton(
				button_id,
				j_item.value("text", button_id),
				j_item.at("icon").get<string>(),
				j_item.value("help_text", ""),
				menu,
				j_item.value("show_text", false));

			EnableTool(aui_item->GetId(), j_item.value("enabled", true));
		}

		// Label
		else if (type == "label")
			AddLabel(-1, wxString::FromUTF8(j_item.at("text").get<string>()));

		// Custom control
		else if (type == "custom_control")
		{
			if (j_item.contains("label"))
				AddLabel(-1, wxString::FromUTF8(j_item.at("label").get<string>()));

			auto   id      = j_item.at("id").get<string>();
			string label   = j_item.value("label", "");
			bool   enabled = j_item.value("enabled", true);

			for (const auto& control : custom_controls_)
				if (id == control.name)
				{
					AddControl(control.control, wxString::FromUTF8(label));
					control.control->Show();
					control.control->Enable(enabled);
				}
		}

		// Separator
		else if (type == "separator")
			AddSeparator();

		// Spacer
		else if (type == "spacer")
			AddSpacer(FromDIP(j_item.value("width", 8)));

		// Stretch spacer
		else if (type == "stretch_spacer")
			AddStretchSpacer(j_item.value("proportion", 1));
	}

	// Remove extraneous separators
	constexpr auto wxITEM_SPACER = 6; // Hopefully this doesn't change
	for (int i = m_items.size() - 1; i >= 0; i--)
	{
		if (m_items.empty() || m_items[i].GetKind() != wxITEM_SEPARATOR)
			continue;

		// Separator at start or end
		if (i == m_items.size() - 1 || i == 0)
		{
			m_items.RemoveAt(i);
			++i;
			continue;
		}

		// Double separator
		if (m_items[i - 1].GetKind() == wxITEM_SEPARATOR)
		{
			m_items.RemoveAt(i);
			++i;
			continue;
		}

		// Separator-spacer
		if (i < m_items.size() - 1 && m_items[i + 1].GetKind() == wxITEM_SPACER)
		{
			m_items.RemoveAt(i);
			++i;
		}
	}

	Realize();
	if (aui_mgr_)
		aui_mgr_->Update();
	else
		GetParent()->Layout();
}

// -----------------------------------------------------------------------------
// Returns the button/SAction ID of the toolbar item with wx id [wx_id].
// -----------------------------------------------------------------------------
string SAuiToolBar::actionFromWxId(int wx_id)
{
	if (auto item = itemByWxId(wx_id); item)
		return item->id;

	return "";
}

// -----------------------------------------------------------------------------
// Returns a pointer to the toolbar item with wx id [wx_id], or nullptr if not
// found.
// -----------------------------------------------------------------------------
const SAuiToolBar::Item* SAuiToolBar::itemByWxId(int wx_id) const
{
	for (auto& item : items_)
		if (item.wx_id == wx_id)
			return &item;

	return nullptr;
}
SAuiToolBar::Item* SAuiToolBar::itemByWxId(int wx_id)
{
	for (auto& item : items_)
		if (item.wx_id == wx_id)
			return &item;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a pointer to the toolbar item with button/SAction ID [id], or nullptr
// if not found.
// -----------------------------------------------------------------------------
const SAuiToolBar::Item* SAuiToolBar::itemById(string_view id) const
{
	for (auto& item : items_)
		if (item.id == id)
			return &item;

	return nullptr;
}
SAuiToolBar::Item* SAuiToolBar::itemById(string_view id)
{
	for (auto& item : items_)
		if (item.id == id)
			return &item;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a pointer to the toolbar group [name], or nullptr if not found.
// -----------------------------------------------------------------------------
SAuiToolBar::Group* SAuiToolBar::groupByName(string_view name)
{
	for (auto& group : groups_)
		if (group.name == name)
			return &group;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Sets the checked state of [item] to [checked].
// -----------------------------------------------------------------------------
void SAuiToolBar::setItemChecked(wxAuiToolBarItem* item, bool checked)
{
	if (!item)
		return;

	if (checked)
		item->SetState(item->GetState() | wxAUI_BUTTON_STATE_CHECKED);
	else
		item->SetState(item->GetState() & ~wxAUI_BUTTON_STATE_CHECKED);

	Refresh();
}

// -----------------------------------------------------------------------------
// Returns true if [item] is enabled, false if disabled or null.
// -----------------------------------------------------------------------------
bool SAuiToolBar::itemEnabled(wxAuiToolBarItem* item) const
{
	if (!item)
		return false;

	return (item->GetState() & wxAUI_BUTTON_STATE_DISABLED) == 0;
}

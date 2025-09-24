
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SAction.cpp
// Description: SAction class, represents an 'action', which can be put on any
//              menu or toolbar and handled by any SActionHandler that claims
//              its id
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
#include "SAction.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "General/KeyBind.h"
#include "SActionHandler.h"
#include "UI/WxUtils.h"
#include "Utility/JsonUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
int                         cur_id = 0;
vector<unique_ptr<SAction>> actions;

vector<SActionHandler*> action_handlers;
int                     wx_id_offset = 0;
} // namespace


// -----------------------------------------------------------------------------
//
// SAction Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SAction class constructor
// -----------------------------------------------------------------------------
SAction::SAction(
	string_view id,
	string_view text,
	string_view icon,
	string_view helptext,
	string_view shortcut,
	Type        type,
	string_view radio_group,
	int         reserve_ids) :
	id_{ id },
	wx_id_{ 0 },
	reserved_ids_{ reserve_ids },
	text_{ text },
	icon_{ icon },
	helptext_{ helptext },
	shortcut_{ shortcut },
	type_{ type },
	group_{ radio_group },
	checked_{ false },
	linked_cvar_{ nullptr }
{
}

// -----------------------------------------------------------------------------
// Returns the shortcut key for this action as a string, taking into account if
// the shortcut is a keybind
// -----------------------------------------------------------------------------
string SAction::shortcutText() const
{
	if (strutil::startsWith(shortcut_, "kb:"))
	{
		auto kp = KeyBind::bind({ shortcut_.data() + 3, shortcut_.size() - 3 }).key(0);
		if (!kp.key.empty())
			return kp.asString();

		return "INVALID KEYBIND";
	}

	return shortcut_;
}

// -----------------------------------------------------------------------------
// Sets the toggled state of the action to [checked], and updates the value of
// the linked cvar (if any) to match
// -----------------------------------------------------------------------------
void SAction::setChecked(bool checked)
{
	if (type_ == Type::Normal)
	{
		checked_ = false;
		return;
	}

	// If toggling a radio action, un-toggle others in the group
	if (checked && type_ == Type::Radio && !group_.empty())
	{
		// Go through and toggle off all other actions in the same group
		for (auto& action : actions)
		{
			if (action->group_ == group_)
				action->setChecked(false);
		}

		checked_ = true;
	}
	else
		checked_ = checked; // Otherwise just set toggled state

	// Signal
	signals().checked_changed(*this);

	// Update linked CVar
	if (linked_cvar_)
		*linked_cvar_ = checked_;
}

// -----------------------------------------------------------------------------
// Sets the action's wxWidgets id to the next available id
// -----------------------------------------------------------------------------
void SAction::initWxId()
{
	wx_id_ = cur_id;
	cur_id += reserved_ids_;
}

// -----------------------------------------------------------------------------
// Adds this action to [menu]. If [text_override] is not "NO", it will be used
// instead of the action's text as the menu item label.
// [show_shortcut]: 0 = Don't Show, 1 = Always Show, 2 = Auto (show if ctrl/alt)
// -----------------------------------------------------------------------------
bool SAction::addToMenu(
	wxMenu*     menu,
	int         show_shortcut,
	string_view text_override,
	string_view icon_override,
	int         wx_id_offset)
{
	// Can't add to nonexistant menu
	if (!menu)
		return false;

	// Determine shortcut key
	auto sc         = shortcut_;
	bool sc_control = strutil::contains(shortcut_, "Ctrl") || strutil::contains(shortcut_, "Alt");
	if (strutil::startsWith(shortcut_, "kb:"))
	{
		auto kp = KeyBind::bind({ shortcut_.data() + 3, shortcut_.size() - 3 }).key(0);
		if (!kp.key.empty())
			sc = kp.asString();
		else
			sc = "None";

		// Keybinds are handled separately so we don't want menu item shortcut keys to override them
		show_shortcut = 0;
	}

	// Determine if shortcut key should be added
	if (show_shortcut > 1)
		show_shortcut = sc_control ? 1 : 0;

	// Setup menu item string
	auto item_text = text_;
	if (text_override != "NO")
		item_text = text_override;
	if (show_shortcut == 1 && !sc.empty())
		item_text = fmt::format("{}\t{}", item_text, sc);

	// Append this action to the menu
	auto help      = helptext_;
	int  wid       = wx_id_ + wx_id_offset;
	auto real_icon = (icon_override == "NO") ? iconName() : string{ icon_override };
	if (!sc.empty())
		help += fmt::format(" (Shortcut: {})", sc);
	if (type_ == Type::Normal)
		menu->Append(wxutil::createMenuItem(menu, wid, item_text, help, real_icon));
	else if (type_ == Type::Check)
	{
		auto item = menu->AppendCheckItem(wid, wxString::FromUTF8(item_text), wxString::FromUTF8(help));
		item->Check(checked_);
	}
	else if (type_ == Type::Radio)
		menu->AppendRadioItem(wid, wxString::FromUTF8(item_text), wxString::FromUTF8(help));

	return true;
}

// -----------------------------------------------------------------------------
// Sets the base wxWidgets id to [id]. This is used to determine the next
// available wxWidgets id for new SActions
// -----------------------------------------------------------------------------
void SAction::setBaseWxId(int id)
{
	cur_id = id;
}

// -----------------------------------------------------------------------------
// Loads SAction properties from a JSON object [j]
// -----------------------------------------------------------------------------
void SAction::fromJson(const Json& j)
{
	text_         = j.value("text", id_);
	icon_         = j.value("icon", icon_);
	helptext_     = j.value("help_text", helptext_);
	wx_id_        = j.value("wx_id", wx_id_);
	reserved_ids_ = j.value("reserved_ids", reserved_ids_);
	group_        = j.value("group", group_);

	// Shortcut can be defined as either "shortcut" or "keybind"
	if (j.contains("shortcut"))
		shortcut_ = j["shortcut"];
	else if (j.contains("keybind"))
		shortcut_ = fmt::format("kb:{}", j["keybind"].get<string>());

	if (j.contains("type"))
	{
		if (auto lc_type = strutil::lower(j["type"].get<string>()); lc_type == "check")
			type_ = Type::Check;
		else if (lc_type == "radio")
			type_ = Type::Radio;
	}

	if (j.contains("linked_cvar"))
	{
		auto cvar = CVar::get(j["linked_cvar"]);
		if (cvar && cvar->type == CVar::Type::Boolean)
		{
			linked_cvar_ = dynamic_cast<CBoolCVar*>(cvar);
			checked_     = cvar->getValue().Bool;
		}
	}

	// Get wx id if not custom defined
	if (wx_id_ == 0)
		initWxId();
}


// -----------------------------------------------------------------------------
//
// SAction Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Loads and parses all SActions configured in actions.cfg in the program
// resource archive
// -----------------------------------------------------------------------------
bool SAction::initActions()
{
	// Get actions directory from program resource
	auto actions_dir = app::programResource()->dirAtPath("actions");
	if (!actions_dir)
		return false;

	// Parse each json file in the directory
	for (auto& entry : actions_dir->entries())
	{
		if (auto j = jsonutil::parse(entry->data()); !j.is_discarded())
		{
			for (auto& [id, j_action] : j.items())
			{
				auto action = std::make_unique<SAction>(id, id);
				action->fromJson(j_action);
				actions.push_back(std::move(action));
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns the SAction with id matching [id]
// -----------------------------------------------------------------------------
SAction* SAction::fromId(string_view id)
{
	// Find action with id
	for (auto& action : actions)
		if (strutil::equalCI(action->id_, id))
			return action.get();

	// Not found
	return invalidAction();
}

// -----------------------------------------------------------------------------
// Returns the SAction covering wxWidgets id [wx_id]
// -----------------------------------------------------------------------------
SAction* SAction::fromWxId(int wx_id)
{
	// Find action with id
	for (auto& action : actions)
		if (action->isWxId(wx_id))
			return action.get();

	// Not found
	return invalidAction();
}

// -----------------------------------------------------------------------------
// Adds [action] to the list of all SActions (if it isn't in there already)
// -----------------------------------------------------------------------------
SAction* SAction::add(unique_ptr<SAction> action)
{
	if (!action)
		return nullptr;

	actions.push_back(move(action));

	return actions.back().get();
}

// -----------------------------------------------------------------------------
// Gets the next free wxWidgets action id
// -----------------------------------------------------------------------------
int SAction::nextWxId()
{
	int id = cur_id;
	++cur_id;
	return id;
}

// -----------------------------------------------------------------------------
// Returns the SAction signals struct
// -----------------------------------------------------------------------------
SAction::Signals& SAction::signals()
{
	static Signals signals;
	return signals;
}

// -----------------------------------------------------------------------------
// Returns the global 'invalid' SAction, creating it if necessary
// -----------------------------------------------------------------------------
SAction* SAction::invalidAction()
{
	static SAction action_invalid{ "invalid", "Invalid Action", "", "Something's gone wrong here" };
	return &action_invalid;
}


// -----------------------------------------------------------------------------
//
// SActionHandler Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SActionHandler class constructor
// -----------------------------------------------------------------------------
SActionHandler::SActionHandler()
{
	action_handlers.push_back(this);
}

// -----------------------------------------------------------------------------
// SActionHandler class destructor
// -----------------------------------------------------------------------------
SActionHandler::~SActionHandler()
{
	VECTOR_REMOVE(action_handlers, this);
}

// -----------------------------------------------------------------------------
// Gets the wxWidgets id offset for SActions currently being handled
// -----------------------------------------------------------------------------
int SActionHandler::wxIdOffset()
{
	return wx_id_offset;
}

// -----------------------------------------------------------------------------
// Sets the wxWidgets id [offset] for SActions currently being handled
// -----------------------------------------------------------------------------
void SActionHandler::setWxIdOffset(int offset)
{
	wx_id_offset = offset;
}

// -----------------------------------------------------------------------------
// Handles the SAction [id], returns true if an SActionHandler handled the
// action, false otherwise
// -----------------------------------------------------------------------------
bool SActionHandler::doAction(string_view id)
{
	bool handled = false;

	// Toggle action if necessary
	if (auto* action = SAction::fromId(id))
		if (action->type() != SAction::Type::Normal)
		{
			action->toggle();

			// Action is technically 'handled' already if there was a linked cvar (don't log warning)
			if (action->linkedCVar())
				handled = true;
		}

	// Send action to all handlers
	for (auto& action_handler : action_handlers)
	{
		if (action_handler->handleAction(id))
		{
			handled = true;
			break;
		}
	}

	// Warn if nothing handled it
	if (!handled)
		log::warning(fmt::format("Warning: Action \"{}\" not handled", id));

	// Log action (to log file only)
	// TODO: this
	// exiting = true;
	// log::info(1, "**** Action \"%s\"", id);
	// exiting = false;

	// Return true if handled
	return handled;
}

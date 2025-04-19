
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "General/SAction.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/KeyBind.h"
#include "UI/WxUtils.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
int                     SAction::n_groups_ = 0;
int                     SAction::cur_id_   = 0;
vector<SAction*>        SAction::actions_;
SAction*                SAction::action_invalid_ = nullptr;
vector<SActionHandler*> SActionHandler::action_handlers_;
int                     SActionHandler::wx_id_offset_ = 0;


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
	int         radio_group,
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
// Sets the toggled state of the action to [toggle], and updates the value of
// the linked cvar (if any) to match
// -----------------------------------------------------------------------------
void SAction::setChecked(bool toggle)
{
	if (type_ == Type::Normal)
	{
		checked_ = false;
		return;
	}

	// If toggling a radio action, un-toggle others in the group
	if (toggle && type_ == Type::Radio && group_ >= 0)
	{
		// Go through and toggle off all other actions in the same group
		for (auto& action : actions_)
		{
			if (action->group_ == group_)
				action->setChecked(false);
		}

		checked_ = true;
	}
	else
		checked_ = toggle; // Otherwise just set toggled state

	// Update linked CVar
	if (linked_cvar_)
		*linked_cvar_ = checked_;
}

// -----------------------------------------------------------------------------
// Sets the action's wxWidgets id to the next available id
// -----------------------------------------------------------------------------
void SAction::initWxId()
{
	wx_id_ = cur_id_;
	cur_id_ += reserved_ids_;
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
// Loads a parsed SAction definition
// -----------------------------------------------------------------------------
bool SAction::parse(const ParseTreeNode* node)
{
	string linked_cvar;
	int    custom_wxid = 0;

	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto prop      = node->childPTN(a);
		auto prop_name = strutil::lower(prop->name());

		// Text
		if (prop_name == "text")
			text_ = prop->stringValue();

		// Icon
		else if (prop_name == "icon")
			icon_ = prop->stringValue();

		// Help Text
		else if (prop_name == "help_text")
			helptext_ = prop->stringValue();

		// Shortcut
		else if (prop_name == "shortcut")
			shortcut_ = prop->stringValue();

		// Keybind (shortcut)
		else if (prop_name == "keybind")
			shortcut_ = fmt::format("kb:{}", prop->stringValue());

		// Type
		else if (prop_name == "type")
		{
			auto lc_type = strutil::lower(prop->stringValue());
			if (lc_type == "check")
				type_ = Type::Check;
			else if (lc_type == "radio")
				type_ = Type::Radio;
		}

		// Linked CVar
		else if (prop_name == "linked_cvar")
			linked_cvar = prop->stringValue();

		// Custom wx id
		else if (prop_name == "custom_wx_id")
			custom_wxid = prop->intValue();

		// Reserve ids
		else if (prop_name == "reserve_ids")
			reserved_ids_ = prop->intValue();
	}

	// Setup wxWidgets id stuff
	if (custom_wxid == 0)
		initWxId();
	else
		wx_id_ = custom_wxid;

	// Setup linked cvar
	if (type_ == Type::Check && !linked_cvar.empty())
	{
		auto cvar = CVar::get(linked_cvar);
		if (cvar && cvar->type == CVar::Type::Boolean)
		{
			linked_cvar_ = dynamic_cast<CBoolCVar*>(cvar);
			checked_     = cvar->getValue().Bool;
		}
	}

	return true;
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
	// Get actions.cfg from slade.pk3
	auto cfg_entry = app::archiveManager().programResourceArchive()->entryAtPath("actions.cfg");
	if (!cfg_entry)
		return false;

	Parser parser(cfg_entry->parentDir());
	if (parser.parseText(cfg_entry->data(), "actions.cfg"))
	{
		auto root = parser.parseTreeRoot();
		for (unsigned a = 0; a < root->nChildren(); a++)
		{
			auto node = root->childPTN(a);

			// Single action
			if (strutil::equalCI(node->type(), "action"))
			{
				auto action = new SAction(node->name(), node->name());
				if (action->parse(node))
					actions_.push_back(action);
				else
					delete action;
			}

			// Group of actions
			else if (strutil::equalCI(node->name(), "group"))
			{
				int group = newGroup();

				for (unsigned b = 0; b < node->nChildren(); b++)
				{
					auto group_node = node->childPTN(b);
					if (strutil::equalCI(group_node->type(), "action"))
					{
						auto action = new SAction(group_node->name(), group_node->name());
						if (action->parse(group_node))
						{
							action->group_ = group;
							actions_.push_back(action);
						}
						else
							delete action;
					}
				}
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns a new, unused SAction group id
// -----------------------------------------------------------------------------
int SAction::newGroup()
{
	return n_groups_++;
}

// -----------------------------------------------------------------------------
// Returns the SAction with id matching [id]
// -----------------------------------------------------------------------------
SAction* SAction::fromId(string_view id)
{
	// Find action with id
	for (auto& action : actions_)
		if (strutil::equalCI(action->id_, id))
			return action;

	// Not found
	return invalidAction();
}

// -----------------------------------------------------------------------------
// Returns the SAction covering wxWidgets id [wx_id]
// -----------------------------------------------------------------------------
SAction* SAction::fromWxId(int wx_id)
{
	// Find action with id
	for (auto& action : actions_)
		if (action->isWxId(wx_id))
			return action;

	// Not found
	return invalidAction();
}

// -----------------------------------------------------------------------------
// Adds [action] to the list of all SActions (if it isn't in there already)
// -----------------------------------------------------------------------------
void SAction::add(SAction* action)
{
	if (!action)
		return;

	if (VECTOR_EXISTS(actions_, action))
		return;

	actions_.push_back(action);
}

// -----------------------------------------------------------------------------
// Gets the next free wxWidgets action id
// -----------------------------------------------------------------------------
int SAction::nextWxId()
{
	int id = cur_id_;
	++cur_id_;
	return id;
}

// -----------------------------------------------------------------------------
// Returns the global 'invalid' SAction, creating it if necessary
// -----------------------------------------------------------------------------
SAction* SAction::invalidAction()
{
	if (!action_invalid_)
		action_invalid_ = new SAction("invalid", "Invalid Action", "", "Something's gone wrong here");

	return action_invalid_;
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
	action_handlers_.push_back(this);
}

// -----------------------------------------------------------------------------
// SActionHandler class destructor
// -----------------------------------------------------------------------------
SActionHandler::~SActionHandler()
{
	VECTOR_REMOVE(action_handlers_, this);
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
	for (auto& action_handler : action_handlers_)
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

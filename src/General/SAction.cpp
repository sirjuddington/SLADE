
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
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
#include "Archive/ArchiveManager.h"
#include "General/KeyBind.h"
#include "Graphics/Icons.h"
#include "UI/WxUtils.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"


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
	if (StrUtil::startsWith(shortcut_, "kb:"))
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
// Adds this action to [menu]. If [text_override] is not "NO", it will be used
// instead of the action's text as the menu item label
// -----------------------------------------------------------------------------
bool SAction::addToMenu(wxMenu* menu, string_view text_override, string_view icon_override, int wx_id_offset)
{
	return addToMenu(menu, false, text_override, icon_override, wx_id_offset);
}
bool SAction::addToMenu(
	wxMenu*     menu,
	bool        show_shortcut,
	string_view text_override,
	string_view icon_override,
	int         wx_id_offset)
{
	// Can't add to nonexistant menu
	if (!menu)
		return false;

	// Determine shortcut key
	auto sc         = shortcut_;
	bool sc_control = StrUtil::contains(shortcut_, "Ctrl") || StrUtil::contains(shortcut_, "Alt");
	if (StrUtil::startsWith(shortcut_, "kb:"))
	{
		auto kp = KeyBind::bind({ shortcut_.data() + 3, shortcut_.size() - 3 }).key(0);
		if (!kp.key.empty())
			sc = kp.asString();
		else
			sc = "None";
		sc_control = (kp.ctrl || kp.alt);
	}

	// Setup menu item string
	auto item_text = text_;
	if (text_override != "NO")
		item_text = text_override;
	if (!sc.empty() && (sc_control || show_shortcut))
		item_text = fmt::format("{}\t{}", item_text, sc);

	// Append this action to the menu
	auto help      = helptext_;
	int  wid       = wx_id_ + wx_id_offset;
	auto real_icon = (icon_override == "NO") ? icon_ : icon_override;
	if (!sc.empty())
		help += fmt::format(" (Shortcut: {})", sc);
	if (type_ == Type::Normal)
		menu->Append(WxUtils::createMenuItem(menu, wid, item_text, help, WxUtils::strFromView(real_icon)));
	else if (type_ == Type::Check)
	{
		auto item = menu->AppendCheckItem(wid, item_text, help);
		item->Check(checked_);
	}
	else if (type_ == Type::Radio)
		menu->AppendRadioItem(wid, item_text, help);

	return true;
}

// -----------------------------------------------------------------------------
// Adds this action to [toolbar]. If [icon_override] is not "NO", it will be
// used instead of the action's icon as the tool icon
// -----------------------------------------------------------------------------
bool SAction::addToToolbar(wxAuiToolBar* toolbar, string_view icon_override, int wx_id_offset) const
{
	// Can't add to nonexistant toolbar
	if (!toolbar)
		return false;

	// Setup icon
	auto useicon = icon_;
	if (icon_override != "NO")
		useicon = icon_override;

	// Append this action to the toolbar
	int wid = wx_id_ + wx_id_offset;
	if (type_ == Type::Normal)
		toolbar->AddTool(wid, text_, Icons::getIcon(Icons::General, useicon), helptext_);
	else if (type_ == Type::Check)
		toolbar->AddTool(wid, text_, Icons::getIcon(Icons::General, useicon), helptext_, wxITEM_CHECK);
	else if (type_ == Type::Radio)
		toolbar->AddTool(wid, text_, Icons::getIcon(Icons::General, useicon), helptext_, wxITEM_RADIO);

	return true;
}

// -----------------------------------------------------------------------------
// Adds this action to [toolbar]. If [icon_override] is not "NO", it will be
// used instead of the action's icon as the tool icon
// -----------------------------------------------------------------------------
bool SAction::addToToolbar(wxToolBar* toolbar, string_view icon_override, int wx_id_offset) const
{
	// Can't add to nonexistant toolbar
	if (!toolbar)
		return false;

	// Setup icon
	auto useicon = icon_;
	if (icon_override != "NO")
		useicon = icon_override;

	// Append this action to the toolbar
	int wid = wx_id_ + wx_id_offset;
	if (type_ == Type::Normal)
		toolbar->AddTool(wid, "", Icons::getIcon(Icons::General, useicon), helptext_);
	else if (type_ == Type::Check)
		toolbar->AddTool(wid, "", Icons::getIcon(Icons::General, useicon), helptext_, wxITEM_CHECK);
	else if (type_ == Type::Radio)
		toolbar->AddTool(wid, "", Icons::getIcon(Icons::General, useicon), helptext_, wxITEM_RADIO);

	return true;
}

// -----------------------------------------------------------------------------
// Loads a parsed SAction definition
// -----------------------------------------------------------------------------
bool SAction::parse(ParseTreeNode* node)
{
	string linked_cvar;
	int    custom_wxid = 0;

	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto prop      = node->childPTN(a);
		auto prop_name = StrUtil::lower(prop->name());

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
			auto lc_type = StrUtil::lower(prop->stringValue());
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
	{
		wx_id_ = cur_id_;
		cur_id_ += reserved_ids_;
	}
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
	auto cfg_entry = App::archiveManager().programResourceArchive()->entryAtPath("actions.cfg");
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
			if (StrUtil::equalCI(node->type(), "action"))
			{
				auto action = new SAction(node->name(), node->name());
				if (action->parse(node))
					actions_.push_back(action);
				else
					delete action;
			}

			// Group of actions
			else if (StrUtil::equalCI(node->name(), "group"))
			{
				int group = newGroup();

				for (unsigned b = 0; b < node->nChildren(); b++)
				{
					auto group_node = node->childPTN(b);
					if (StrUtil::equalCI(group_node->type(), "action"))
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
		if (StrUtil::equalCI(action->id_, id))
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
	// Toggle action if necessary
	SAction::fromId(id)->toggle();

	// Send action to all handlers
	bool handled = false;
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
		Log::warning(fmt::format("Warning: Action \"{}\" not handled", id));

	// Log action (to log file only)
	// TODO: this
	// exiting = true;
	// Log::info(1, "**** Action \"%s\"", id);
	// exiting = false;

	// Return true if handled
	return handled;
}


// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
	string id,
	string text,
	string icon,
	string helptext,
	string shortcut,
	Type   type,
	int    radio_group,
	int    reserve_ids) :
	id_{ id },
	wx_id_{ -1 },
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
// SAction class destructor
// -----------------------------------------------------------------------------
SAction::~SAction() {}

// -----------------------------------------------------------------------------
// Returns the shortcut key for this action as a string, taking into account if
// the shortcut is a keybind
// -----------------------------------------------------------------------------
string SAction::getShortcutText() const
{
	if (shortcut_.StartsWith("kb:"))
	{
		auto kp = KeyBind::getBind(shortcut_.Mid(3)).getKey(0);
		if (kp.key != "")
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
		for (unsigned a = 0; a < actions_.size(); a++)
		{
			if (actions_[a]->group_ == group_)
				actions_[a]->setChecked(false);
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
bool SAction::addToMenu(wxMenu* menu, string text_override, string icon_override, int wx_id_offset)
{
	return addToMenu(menu, false, text_override, icon_override, wx_id_offset);
}
bool SAction::addToMenu(wxMenu* menu, bool show_shortcut, string text_override, string icon_override, int wx_id_offset)
{
	// Can't add to nonexistant menu
	if (!menu)
		return false;

	// Determine shortcut key
	string sc         = shortcut_;
	bool   sc_control = shortcut_.Contains("Ctrl") || shortcut_.Contains("Alt");
	if (shortcut_.StartsWith("kb:"))
	{
		auto kp = KeyBind::getBind(shortcut_.Mid(3)).getKey(0);
		if (kp.key != "")
			sc = kp.asString();
		else
			sc = "None";
		sc_control = (kp.ctrl || kp.alt);
	}

	// Setup menu item string
	string item_text = text_;
	if (!(S_CMP(text_override, "NO")))
		item_text = text_override;
	if (!sc.IsEmpty() && (sc_control || show_shortcut))
		item_text = S_FMT("%s\t%s", item_text, sc);

	// Append this action to the menu
	string help      = helptext_;
	int    wid       = wx_id_ + wx_id_offset;
	string real_icon = (icon_override == "NO") ? icon_ : icon_override;
	if (!sc.IsEmpty())
		help += S_FMT(" (Shortcut: %s)", sc);
	if (type_ == Type::Normal)
		menu->Append(WxUtils::createMenuItem(menu, wid, item_text, help, real_icon));
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
bool SAction::addToToolbar(wxAuiToolBar* toolbar, string icon_override, int wx_id_offset)
{
	// Can't add to nonexistant toolbar
	if (!toolbar)
		return false;

	// Setup icon
	string useicon = icon_;
	if (!(S_CMP(icon_override, "NO")))
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
bool SAction::addToToolbar(wxToolBar* toolbar, string icon_override, int wx_id_offset)
{
	// Can't add to nonexistant toolbar
	if (!toolbar)
		return false;

	// Setup icon
	string useicon = icon_;
	if (!(S_CMP(icon_override, "NO")))
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
	int    custom_wxid = -1;

	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto   prop      = node->getChildPTN(a);
		string prop_name = prop->getName();

		// Text
		if (S_CMPNOCASE(prop_name, "text"))
			text_ = prop->stringValue();

		// Icon
		else if (S_CMPNOCASE(prop_name, "icon"))
			icon_ = prop->stringValue();

		// Help Text
		else if (S_CMPNOCASE(prop_name, "help_text"))
			helptext_ = prop->stringValue();

		// Shortcut
		else if (S_CMPNOCASE(prop_name, "shortcut"))
			shortcut_ = prop->stringValue();

		// Keybind (shortcut)
		else if (S_CMPNOCASE(prop_name, "keybind"))
			shortcut_ = S_FMT("kb:%s", prop->stringValue());

		// Type
		else if (S_CMPNOCASE(prop_name, "type"))
		{
			string lc_type = prop->stringValue().Lower();
			if (lc_type == "check")
				type_ = Type::Check;
			else if (lc_type == "radio")
				type_ = Type::Radio;
		}

		// Linked CVar
		else if (S_CMPNOCASE(prop_name, "linked_cvar"))
			linked_cvar = prop->stringValue();

		// Custom wx id
		else if (S_CMPNOCASE(prop_name, "custom_wx_id"))
			custom_wxid = prop->intValue();

		// Reserve ids
		else if (S_CMPNOCASE(prop_name, "reserve_ids"))
			reserved_ids_ = prop->intValue();
	}

	// Setup wxWidgets id stuff
	if (custom_wxid == -1)
	{
		wx_id_ = cur_id_;
		cur_id_ += reserved_ids_;
	}
	else
		wx_id_ = custom_wxid;

	// Setup linked cvar
	if (type_ == Type::Check && !linked_cvar.IsEmpty())
	{
		auto cvar = getCVar(linked_cvar);
		if (cvar && cvar->type == CVAR_BOOLEAN)
		{
			this->linked_cvar_ = (CBoolCVar*)cvar;
			checked_           = cvar->GetValue().Bool;
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

	Parser parser(cfg_entry->getParentDir());
	if (parser.parseText(cfg_entry->getMCData(), "actions.cfg"))
	{
		auto root = parser.parseTreeRoot();
		for (unsigned a = 0; a < root->nChildren(); a++)
		{
			auto node = root->getChildPTN(a);

			// Single action
			if (S_CMPNOCASE(node->type(), "action"))
			{
				auto action = new SAction(node->getName(), node->getName());
				if (action->parse(node))
					actions_.push_back(action);
				else
					delete action;
			}

			// Group of actions
			else if (S_CMPNOCASE(node->getName(), "group"))
			{
				int group = newGroup();

				for (unsigned b = 0; b < node->nChildren(); b++)
				{
					auto group_node = node->getChildPTN(b);
					if (S_CMPNOCASE(group_node->type(), "action"))
					{
						auto action = new SAction(group_node->getName(), group_node->getName());
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
SAction* SAction::fromId(const string& id)
{
	// Find action with id
	for (auto& action : actions_)
		if (S_CMPNOCASE(action->id_, id))
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
bool SActionHandler::doAction(string id)
{
	// Toggle action if necessary
	SAction::fromId(id)->toggle();

	// Send action to all handlers
	bool handled = false;
	for (unsigned a = 0; a < action_handlers_.size(); a++)
	{
		action_handlers_[a]->wx_id_offset_ = wx_id_offset_;
		if (action_handlers_[a]->handleAction(id))
		{
			handled = true;
			break;
		}
	}

	// Warn if nothing handled it
	if (!handled)
		LOG_MESSAGE(1, "Warning: Action \"%s\" not handled", id);

	// Log action (to log file only)
	// TODO: this
	// exiting = true;
	// LOG_MESSAGE(1, "**** Action \"%s\"", id);
	// exiting = false;

	// Return true if handled
	return handled;
}

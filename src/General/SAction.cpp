
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SAction.cpp
 * Description: SAction class, represents an 'action', which can
 *              be put on any menu or toolbar and handled by any
 *              SActionHandler that claims its id
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "Archive/ArchiveManager.h"
#include "General/KeyBind.h"
#include "General/SAction.h"
#include "Graphics/Icons.h"
#include "UI/WxStuff.h"
#include "Utility/Parser.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
int						SAction::n_groups = 0;
int						SAction::cur_id = 0;
vector<SAction*>		SAction::actions;
SAction*				SAction::action_invalid = nullptr;
vector<SActionHandler*>	SActionHandler::action_handlers;
int						SActionHandler::wx_id_offset = 0;


/*******************************************************************
 * SACTION CLASS FUNCTIONS
 *******************************************************************/

/* SAction::SAction
 * SAction class constructor
 *******************************************************************/
SAction::SAction(
	string id,
	string text,
	string icon,
	string helptext,
	string shortcut,
	Type type,
	int radio_group,
	int reserve_ids
) :
	id { id },
	wx_id{ -1 },
	reserved_ids{ reserve_ids },
	text{ text },
	icon{ icon },
	helptext{ helptext },
	shortcut{ shortcut },
	type{ type },
	group{ radio_group },
	checked{ false },
	linked_cvar{ nullptr }
{
	//// Add to actions
	//actions.push_back(this);

	//// Setup wxWidgets id stuff
	//if (custom_wxid == -1)
	//{
	//	wx_id = cur_id;
	//	cur_id += reserved_ids;
	//}
	//else
	//	wx_id = custom_wxid;

	//// Setup linked cvar
	//if (type == Type::Check && !linked_cvar.IsEmpty())
	//{
	//	auto cvar = get_cvar(linked_cvar);
	//	if (cvar && cvar->type == CVAR_BOOLEAN)
	//	{
	//		this->linked_cvar = (CBoolCVar*)cvar;
	//		checked = cvar->GetValue().Bool;
	//	}
	//}
}

/* SAction::~SAction
 * SAction class destructor
 *******************************************************************/
SAction::~SAction()
{
}

/* SAction::getShortcutText
 * Returns the shortcut key for this action as a string, taking into
 * account if the shortcut is a keybind
 *******************************************************************/
string SAction::getShortcutText() const
{
	if (shortcut.StartsWith("kb:"))
	{
		auto kp = KeyBind::getBind(shortcut.Mid(3)).getKey(0);
		if (kp.key != "")
			return kp.as_string();

		return "INVALID KEYBIND";
	}

	return shortcut;
}

/* SAction::setToggled
 * Sets the toggled state of the action to [toggle], and updates the
 * value of the linked cvar (if any) to match
 *******************************************************************/
void SAction::setChecked(bool toggle)
{
	if (type == Type::Normal)
	{
		checked = false;
		return;
	}

	// If toggling a radio action, un-toggle others in the group
	if (toggle && type == Type::Radio && group >= 0)
	{
		// Go through and toggle off all other actions in the same group
		for (unsigned a = 0; a < actions.size(); a++)
		{
			if (actions[a]->group == group)
				actions[a]->setChecked(false);
		}

		checked = true;
	}
	else
		checked = toggle;	// Otherwise just set toggled state

	// Update linked CVar
	if (linked_cvar)
		*linked_cvar = checked;
}

/* SAction::addToMenu
 * Adds this action to [menu]. If [text_override] is not "NO", it
 * will be used instead of the action's text as the menu item label.
 * If [popup] is true the shortcut key will always be added to the
 * item label
 *******************************************************************/
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
	string sc = shortcut;
	bool sc_control = shortcut.Contains("Ctrl") || shortcut.Contains("Alt");
	if (shortcut.StartsWith("kb:"))
	{
		auto kp = KeyBind::getBind(shortcut.Mid(3)).getKey(0);
		if (kp.key != "")
			sc = kp.as_string();
		else
			sc = "None";
		sc_control = (kp.ctrl || kp.alt);
	}

	// Setup menu item string
	string item_text = text;
	if (!(S_CMP(text_override, "NO")))
		item_text = text_override;
	if (!sc.IsEmpty() && (sc_control || show_shortcut))
		item_text = S_FMT("%s\t%s", item_text, sc);

	// Append this action to the menu
	string help = helptext;
	int wid = wx_id + wx_id_offset;
	string real_icon = (icon_override == "NO") ? icon : icon_override;
	if (!sc.IsEmpty()) help += S_FMT(" (Shortcut: %s)", sc);
	if (type == Type::Normal)
		menu->Append(createMenuItem(menu, wid, item_text, help, real_icon));
	else if (type == Type::Check)
	{
		auto item = menu->AppendCheckItem(wid, item_text, help);
		item->Check(checked);
	}
	else if (type == Type::Radio)
		menu->AppendRadioItem(wid, item_text, help);

	return true;
}

/* SAction::addToToolbar
 * Adds this action to [toolbar]. If [icon_override] is not "NO", it
 * will be used instead of the action's icon as the tool icon
 *******************************************************************/
bool SAction::addToToolbar(wxAuiToolBar* toolbar, string icon_override, int wx_id_offset)
{
	// Can't add to nonexistant toolbar
	if (!toolbar)
		return false;

	// Setup icon
	string useicon = icon;
	if (!(S_CMP(icon_override, "NO")))
		useicon = icon_override;

	// Append this action to the toolbar
	int wid = wx_id + wx_id_offset;
	if (type == Type::Normal)
		toolbar->AddTool(wid, text, Icons::getIcon(Icons::GENERAL, useicon), helptext);
	else if (type == Type::Check)
		toolbar->AddTool(wid, text, Icons::getIcon(Icons::GENERAL, useicon), helptext, wxITEM_CHECK);
	else if (type == Type::Radio)
		toolbar->AddTool(wid, text, Icons::getIcon(Icons::GENERAL, useicon), helptext, wxITEM_RADIO);

	return true;
}

/* SAction::addToToolbar
 * Adds this action to [toolbar]. If [icon_override] is not "NO", it
 * will be used instead of the action's icon as the tool icon
 *******************************************************************/
bool SAction::addToToolbar(wxToolBar* toolbar, string icon_override, int wx_id_offset)
{
	// Can't add to nonexistant toolbar
	if (!toolbar)
		return false;

	// Setup icon
	string useicon = icon;
	if (!(S_CMP(icon_override, "NO")))
		useicon = icon_override;

	// Append this action to the toolbar
	int wid = wx_id + wx_id_offset;
	if (type == Type::Normal)
		toolbar->AddTool(wid, "", Icons::getIcon(Icons::GENERAL, useicon), helptext);
	else if (type == Type::Check)
		toolbar->AddTool(wid, "", Icons::getIcon(Icons::GENERAL, useicon), helptext, wxITEM_CHECK);
	else if (type == Type::Radio)
		toolbar->AddTool(wid, "", Icons::getIcon(Icons::GENERAL, useicon), helptext, wxITEM_RADIO);

	return true;
}

/* SAction::parse
 * Loads a parsed SAction definition
 *******************************************************************/
bool SAction::parse(ParseTreeNode* node)
{
	string linked_cvar;
	int custom_wxid = -1;

	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto prop = (ParseTreeNode*)node->getChild(a);
		string prop_name = prop->getName();
		
		// Text
		if (S_CMPNOCASE(prop_name, "text"))
			text = prop->getStringValue();

		// Icon
		else if (S_CMPNOCASE(prop_name, "icon"))
			icon = prop->getStringValue();

		// Help Text
		else if (S_CMPNOCASE(prop_name, "help_text"))
			helptext = prop->getStringValue();

		// Shortcut
		else if (S_CMPNOCASE(prop_name, "shortcut"))
			shortcut = prop->getStringValue();

		// Keybind (shortcut)
		else if (S_CMPNOCASE(prop_name, "keybind"))
			shortcut = S_FMT("kb:%s", prop->getStringValue());

		// Type
		else if (S_CMPNOCASE(prop_name, "type"))
		{
			string lc_type = prop->getStringValue().Lower();
			if (lc_type == "check")
				type = Type::Check;
			else if (lc_type == "radio")
				type = Type::Radio;
		}

		// Linked CVar
		else if (S_CMPNOCASE(prop_name, "linked_cvar"))
			linked_cvar = prop->getStringValue();

		// Custom wx id
		else if (S_CMPNOCASE(prop_name, "custom_wx_id"))
			custom_wxid = prop->getIntValue();

		// Reserve ids
		else if (S_CMPNOCASE(prop_name, "reserve_ids"))
			reserved_ids = prop->getIntValue();
	}

	// Setup wxWidgets id stuff
	if (custom_wxid == -1)
	{
		wx_id = cur_id;
		cur_id += reserved_ids;
	}
	else
		wx_id = custom_wxid;

	// Setup linked cvar
	if (type == Type::Check && !linked_cvar.IsEmpty())
	{
		auto cvar = get_cvar(linked_cvar);
		if (cvar && cvar->type == CVAR_BOOLEAN)
		{
			this->linked_cvar = (CBoolCVar*)cvar;
			checked = cvar->GetValue().Bool;
		}
	}
	
	return true;
}


/*******************************************************************
 * SACTION CLASS STATIC FUNCTIONS
 *******************************************************************/

/* SAction::initActions
 * Loads and parses all SActions configured in actions.cfg in the
 * program resource archive
 *******************************************************************/
bool SAction::initActions()
{
	// Get actions.cfg from slade.pk3
	auto cfg_entry = theArchiveManager->programResourceArchive()->entryAtPath("actions.cfg");
	if (!cfg_entry)
		return false;

	Parser parser(cfg_entry->getParentDir());
	if (parser.parseText(cfg_entry->getMCData(), "actions.cfg"))
	{
		auto root = parser.parseTreeRoot();
		for (unsigned a = 0; a < root->nChildren(); a++)
		{
			auto node = (ParseTreeNode*)root->getChild(a);

			// Single action
			if (S_CMPNOCASE(node->getType(), "action"))
			{
				auto action = new SAction(node->getName(), node->getName());
				if (action->parse(node))
					actions.push_back(action);
				else
					delete action;
			}

			// Group of actions
			else if (S_CMPNOCASE(node->getName(), "group"))
			{
				int group = newGroup();

				for (unsigned b = 0; b < node->nChildren(); b++)
				{
					auto group_node = (ParseTreeNode*)node->getChild(b);
					if (S_CMPNOCASE(group_node->getType(), "action"))
					{
						auto action = new SAction(group_node->getName(), group_node->getName());
						if (action->parse(group_node))
						{
							action->group = group;
							actions.push_back(action);
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

/* SAction::newGroup
 * Returns a new, unused SAction group id
 *******************************************************************/
int SAction::newGroup()
{
	return n_groups++;
}

/* SAction::fromId
 * Returns the SAction with id matching [id]
 *******************************************************************/
SAction* SAction::fromId(const string& id)
{
	// Find action with id
	for (auto& action : actions)
		if (S_CMPNOCASE(action->id, id))
			return action;

	// Not found
	return invalidAction();
}

/* SAction::fromWxId
 * Returns the SAction covering wxWidgets id [wx_id]
 *******************************************************************/
SAction* SAction::fromWxId(int wx_id)
{
	// Find action with id
	for (auto& action : actions)
		if (action->isWxId(wx_id))
			return action;

	// Not found
	return invalidAction();
}

/* SAction::invalidAction
 * Returns the global 'invalid' SAction, creating it if necessary
 *******************************************************************/
SAction* SAction::invalidAction()
{
	if (!action_invalid)
		action_invalid = new SAction("invalid", "Invalid Action", "", "Something's gone wrong here");

	return action_invalid;
}


/*******************************************************************
 * SACTIONHANDLER CLASS FUNCTIONS
 *******************************************************************/

/* SActionHandler::SActionHandler
 * SActionHandler class constructor
 *******************************************************************/
SActionHandler::SActionHandler()
{
	action_handlers.push_back(this);
}

/* SActionHandler::~SActionHandler
 * SActionHandler class destructor
 *******************************************************************/
SActionHandler::~SActionHandler()
{
	VECTOR_REMOVE(action_handlers, this);
}

/* SActionHandler::doAction
 * Handles the SAction [id], returns true if an SActionHandler
 * handled the action, false otherwise
 *******************************************************************/
bool SActionHandler::doAction(string id)
{
	// Toggle action if necessary
	SAction::fromId(id)->toggle();

	// Send action to all handlers
	bool handled = false;
	for (unsigned a = 0; a < action_handlers.size(); a++)
	{
		action_handlers[a]->wx_id_offset = wx_id_offset;
		if (action_handlers[a]->handleAction(id))
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
	//exiting = true;
	//LOG_MESSAGE(1, "**** Action \"%s\"", id);
	//exiting = false;

	// Return true if handled
	return handled;
}

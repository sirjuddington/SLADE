
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
#include "UI/WxStuff.h"
#include "General/SAction.h"
#include "Graphics/Icons.h"
#include "MainApp.h"
#include "General/KeyBind.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
int SAction::n_groups = 0;


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
	int type,
	int custom_wxid,
	int radio_group,
	int reserve_ids,
	string linked_cvar
	)
{
	// Init variables
	this->id = id;
	this->text = text;
	this->icon = icon;
	this->helptext = helptext;
	this->type = type;
	this->shortcut = shortcut;
	this->group = radio_group;
	this->toggled = false;
	this->reserved_ids = reserve_ids;
	this->linked_cvar = NULL;

	// Add to MainApp
	theApp->actions.push_back(this);
	if (custom_wxid == -1)
	{
		wx_id = theApp->cur_id;
		theApp->cur_id += reserved_ids;
	}
	else
		wx_id = custom_wxid;

	// Setup linked cvar
	if (type == CHECK && !linked_cvar.IsEmpty())
	{
		CVar* cvar = get_cvar(linked_cvar);
		if (cvar && cvar->type == CVAR_BOOLEAN)
		{
			this->linked_cvar = (CBoolCVar*)cvar;
			toggled = cvar->GetValue().Bool;
		}
	}
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
string SAction::getShortcutText()
{
	if (shortcut.StartsWith("kb:"))
	{
		keypress_t kp = KeyBind::getBind(shortcut.Mid(3)).getKey(0);
		if (kp.key != "")
			return kp.as_string();
		else
			return "INVALID KEYBIND";
	}

	return shortcut;
}

/* SAction::setToggled
 * Sets the toggled state of the action to [toggle], and updates the
 * value of the linked cvar (if any) to match
 *******************************************************************/
void SAction::setToggled(bool toggle)
{
	toggled = toggle;
	if (linked_cvar)
		*linked_cvar = toggle;
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
	bool is_bind = false;
	bool sc_control = shortcut.Contains("Ctrl") || shortcut.Contains("Alt");
	if (shortcut.StartsWith("kb:"))
	{
		is_bind = true;
		keypress_t kp = KeyBind::getBind(shortcut.Mid(3)).getKey(0);
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
	if (type == NORMAL)
		menu->Append(createMenuItem(menu, wid, item_text, help, real_icon));
	else if (type == CHECK)
	{
		wxMenuItem* item = menu->AppendCheckItem(wid, item_text, help);
		item->Check(toggled);
	}
	else if (type == RADIO)
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
	wxAuiToolBarItem* item = NULL;
	int wid = wx_id + wx_id_offset;
	if (type == NORMAL)
		item = toolbar->AddTool(wid, text, Icons::getIcon(Icons::GENERAL, useicon), helptext);
	else if (type == CHECK)
		item = toolbar->AddTool(wid, text, Icons::getIcon(Icons::GENERAL, useicon), helptext, wxITEM_CHECK);
	else if (type == RADIO)
		item = toolbar->AddTool(wid, text, Icons::getIcon(Icons::GENERAL, useicon), helptext, wxITEM_RADIO);

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
	if (type == NORMAL)
		toolbar->AddTool(wid, "", Icons::getIcon(Icons::GENERAL, useicon), helptext);
	else if (type == CHECK)
		toolbar->AddTool(wid, "", Icons::getIcon(Icons::GENERAL, useicon), helptext, wxITEM_CHECK);
	else if (type == RADIO)
		toolbar->AddTool(wid, "", Icons::getIcon(Icons::GENERAL, useicon), helptext, wxITEM_RADIO);

	return true;
}


/*******************************************************************
 * SACTIONHANDLER CLASS FUNCTIONS
 *******************************************************************/

/* SActionHandler::SActionHandler
 * SActionHandler class constructor
 *******************************************************************/
SActionHandler::SActionHandler()
{
	theApp->action_handlers.push_back(this);
}

/* SActionHandler::~SActionHandler
 * SActionHandler class destructor
 *******************************************************************/
SActionHandler::~SActionHandler()
{
	for (unsigned a = 0; a < theApp->action_handlers.size(); a++)
	{
		if (theApp->action_handlers[a] == this)
		{
			theApp->action_handlers.erase(theApp->action_handlers.begin() + a);
			a--;
		}
	}
}

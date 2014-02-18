
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
#include "WxStuff.h"
#include "SAction.h"
#include "Icons.h"
#include "MainApp.h"
#include "KeyBind.h"
#include <wx/aui/auibar.h>


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
SAction::SAction(string id, string text, string icon, string helptext, string shortcut, int type, int custom_wxid, int radio_group)
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
	this->keybind = keybind;

	// Add to MainApp
	theApp->actions.push_back(this);
	if (custom_wxid == -1)
		wx_id = theApp->cur_id++;
	else
		wx_id = custom_wxid;
}

/* SAction::~SAction
 * SAction class destructor
 *******************************************************************/
SAction::~SAction()
{
}

/* SAction::addToMenu
 * Adds this action to [menu]. If [text_override] is not "NO", it
 * will be used instead of the action's text as the menu item label
 *******************************************************************/
bool SAction::addToMenu(wxMenu* menu, string text_override)
{
	// Can't add to nonexistant menu
	if (!menu)
		return false;

	// Setup menu item string
	string item_text = text;
	if (!(S_CMP(text_override, "NO")))
		item_text = text_override;
	if (!shortcut.IsEmpty())
	{
		if (shortcut.StartsWith("kb:"))
		{
			keypress_t kp = KeyBind::getBind(shortcut.Mid(3)).getKey(0);
			if (kp.key != "")
				item_text = S_FMT("%s\t%s", item_text, kp.as_string());
		}
		else
			item_text = S_FMT("%s\t%s", item_text, shortcut);
	}

	// Append this action to the menu
	if (type == NORMAL)
		menu->Append(createMenuItem(menu, wx_id, item_text, helptext, icon));
	else if (type == CHECK)
		menu->AppendCheckItem(wx_id, item_text, helptext);
	else if (type == RADIO)
		menu->AppendRadioItem(wx_id, item_text, helptext);

	return true;
}

/* SAction::addToToolbar
 * Adds this action to [toolbar]. If [icon_override] is not "NO", it
 * will be used instead of the action's icon as the tool icon
 *******************************************************************/
bool SAction::addToToolbar(wxAuiToolBar* toolbar, string icon_override)
{
	// Can't add to nonexistant toolbar
	if (!toolbar)
		return false;

	// Setup icon
	string useicon = icon;
	if (!(S_CMP(icon_override, "NO")))
		useicon = icon_override;

	// Append this action to the toolbar
	if (type == NORMAL)
		toolbar->AddTool(wx_id, text, getIcon(useicon), helptext);
	else if (type == CHECK)
		toolbar->AddTool(wx_id, text, getIcon(useicon), helptext, wxITEM_CHECK);
	else if (type == RADIO)
		toolbar->AddTool(wx_id, text, getIcon(useicon), helptext, wxITEM_RADIO);

	return true;
}

/* SAction::addToToolbar
 * Adds this action to [toolbar]. If [icon_override] is not "NO", it
 * will be used instead of the action's icon as the tool icon
 *******************************************************************/
bool SAction::addToToolbar(wxToolBar* toolbar, string icon_override)
{
	// Can't add to nonexistant toolbar
	if (!toolbar)
		return false;

	// Setup icon
	string useicon = icon;
	if (!(S_CMP(icon_override, "NO")))
		useicon = icon_override;

	// Append this action to the toolbar
	if (type == NORMAL)
		toolbar->AddTool(wx_id, "", getIcon(useicon), helptext);
	else if (type == CHECK)
		toolbar->AddTool(wx_id, "", getIcon(useicon), helptext, wxITEM_CHECK);
	else if (type == RADIO)
		toolbar->AddTool(wx_id, "", getIcon(useicon), helptext, wxITEM_RADIO);

	return true;
}

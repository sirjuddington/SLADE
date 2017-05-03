
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    KeyBind.cpp
 * Description: Input key binding system
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
#include "KeyBind.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
vector<KeyBind>			keybinds;
vector<KeyBind>			keybinds_sorted;
KeyBind					kb_none("-none-");
vector<KeyBindHandler*> kb_handlers;


/*******************************************************************
 * KEYBIND CLASS FUNCTIONS
 *******************************************************************/

/* KeyBind::KeyBind
 * KeyBind class constructor
 *******************************************************************/
KeyBind::KeyBind(string name)
{
	// Init variables
	this->name = name;
	this->pressed = false;
	this->priority = 0;
}

/* KeyBind::~KeyBind
 * KeyBind class destructor
 *******************************************************************/
KeyBind::~KeyBind()
{
}

/* KeyBind::addKey
 * Adds a key combination to the keybind
 *******************************************************************/
void KeyBind::addKey(string key, bool alt, bool ctrl, bool shift)
{
	keys.push_back(keypress_t());
	keys.back().alt = alt;
	keys.back().ctrl = ctrl;
	keys.back().shift = shift;
	keys.back().key = key;
}

/* KeyBind::keysAsString
 * Returns a string representation of all the keys bound to this
 * keybind
 *******************************************************************/
string KeyBind::keysAsString()
{
	string ret = "";

	for (unsigned a = 0; a < keys.size(); a++)
	{
		if (keys[a].ctrl) ret += "Ctrl+";
		if (keys[a].alt) ret += "Alt+";
		if (keys[a].shift) ret += "Shift+";

		string keyname = keys[a].key;
		keyname.Replace("_", " ");
		keyname = keyname.Capitalize();
		ret += keyname;

		if (a < keys.size() - 1)
			ret += ", ";
	}

	if (ret.IsEmpty())
		return "None";
	else
		return ret;
}


/*******************************************************************
 * KEYBIND CLASS STATIC FUNCTIONS
 *******************************************************************/

/* KeyBind::getBind
 * Returns the keybind [name]
 *******************************************************************/
KeyBind& KeyBind::getBind(string name)
{
	for (unsigned a = 0; a < keybinds.size(); a++)
	{
		if (keybinds[a].name == name)
			return keybinds[a];
	}

	return kb_none;
}

/* KeyBind::getBinds
 * Returns a list of all keybind names bound to [key]
 *******************************************************************/
wxArrayString KeyBind::getBinds(keypress_t key)
{
	wxArrayString matches;

	// Go through all keybinds
	bool pressed = false;
	for (unsigned k = 0; k < keybinds.size(); k++)
	{
		KeyBind& kb = keybinds[k];

		// Go through all keys bound to this keybind
		for (unsigned a = 0; a < kb.keys.size(); a++)
		{
			keypress_t& kp = kb.keys[a];

			// Check for match with keypress
			if (kp.shift == key.shift &&
			        kp.alt == key.alt &&
			        kp.ctrl == key.ctrl &&
			        kp.key == key.key)
				matches.Add(kb.name);
		}
	}

	return matches;
}

/* KeyBind::isPressed
 * Returns true if keybind [name] is currently pressed
 *******************************************************************/
bool KeyBind::isPressed(string name)
{
	return getBind(name).pressed;
}

/* KeyBind::addBind
 * Adds a new keybind
 *******************************************************************/
bool KeyBind::addBind(string name, keypress_t key, string desc, string group, bool ignore_shift, int priority)
{
	// Find keybind
	KeyBind* bind = NULL;
	for (unsigned a = 0; a < keybinds.size(); a++)
	{
		if (keybinds[a].name == name)
		{
			bind = &keybinds[a];
			break;
		}
	}

	// Add keybind if it doesn't exist
	if (!bind)
	{
		keybinds.push_back(KeyBind(name));
		bind = &keybinds.back();
		bind->ignore_shift = ignore_shift;
	}

	// Set keybind description/group
	if (!desc.IsEmpty())
	{
		bind->description = desc;
		bind->group = group;
	}

	// Check if the key is already bound to it
	for (unsigned a = 0; a < bind->keys.size(); a++)
	{
		if (bind->keys[a].alt == key.alt &&
		        bind->keys[a].ctrl == key.ctrl &&
		        bind->keys[a].shift == key.shift &&
		        bind->keys[a].key == key.key)
		{
			// It is, remove the bind
			bind->keys.erase(bind->keys.begin() + a);
			return false;
		}
	}

	// Set priority
	if (priority >= 0)
		bind->priority = priority;

	// Add the keybind
	bind->addKey(key.key, key.alt, key.ctrl, key.shift);

	return true;
}

/* KeyBind::keyName
 * Returns a string representation of [key]
 *******************************************************************/
string KeyBind::keyName(int key)
{
	// Return string representation of key id
	switch (key)
	{
	case WXK_BACK:				return "backspace";
	case WXK_TAB:				return "tab";
	case WXK_RETURN:			return "return";
	case WXK_ESCAPE:			return "escape";
	case WXK_SPACE:				return "space";
	case WXK_DELETE:			return "delete";
	case WXK_CLEAR:				return "clear";
	case WXK_SHIFT:				return "shift";
	case WXK_ALT:				return "alt";
	case WXK_PAUSE:				return "pause";
	case WXK_END:				return "end";
	case WXK_HOME:				return "home";
	case WXK_LEFT:				return "left";
	case WXK_UP:				return "up";
	case WXK_RIGHT:				return "right";
	case WXK_DOWN:				return "down";
	case WXK_INSERT:			return "insert";
	case WXK_NUMPAD0:			return "num_0";
	case WXK_NUMPAD1:			return "num_1";
	case WXK_NUMPAD2:			return "num_2";
	case WXK_NUMPAD3:			return "num_3";
	case WXK_NUMPAD4:			return "num_4";
	case WXK_NUMPAD5:			return "num_5";
	case WXK_NUMPAD6:			return "num_6";
	case WXK_NUMPAD7:			return "num_7";
	case WXK_NUMPAD8:			return "num_8";
	case WXK_NUMPAD9:			return "num_9";
	case WXK_ADD:				return "plus";
	case WXK_SUBTRACT:			return "minus";
	case WXK_F1:				return "f1";
	case WXK_F2:				return "f2";
	case WXK_F3:				return "f3";
	case WXK_F4:				return "f4";
	case WXK_F5:				return "f5";
	case WXK_F6:				return "f6";
	case WXK_F7:				return "f7";
	case WXK_F8:				return "f8";
	case WXK_F9:				return "f9";
	case WXK_F10:				return "f10";
	case WXK_F11:				return "f11";
	case WXK_F12:				return "f12";
	case WXK_F13:				return "f13";
	case WXK_F14:				return "f14";
	case WXK_F15:				return "f15";
	case WXK_F16:				return "f16";
	case WXK_F17:				return "f17";
	case WXK_F18:				return "f18";
	case WXK_F19:				return "f19";
	case WXK_F20:				return "f20";
	case WXK_F21:				return "f21";
	case WXK_F22:				return "f22";
	case WXK_F23:				return "f23";
	case WXK_F24:				return "f24";
	case WXK_NUMLOCK:			return "numlock";
	case WXK_PAGEUP:			return "pageup";
	case WXK_PAGEDOWN:			return "pagedown";
	case WXK_NUMPAD_SPACE:		return "num_space";
	case WXK_NUMPAD_TAB:		return "num_tab";
	case WXK_NUMPAD_ENTER:		return "num_enter";
	case WXK_NUMPAD_F1:			return "num_f1";
	case WXK_NUMPAD_F2:			return "num_f2";
	case WXK_NUMPAD_F3:			return "num_f3";
	case WXK_NUMPAD_F4:			return "num_f4";
	case WXK_NUMPAD_HOME:		return "num_home";
	case WXK_NUMPAD_LEFT:		return "num_left";
	case WXK_NUMPAD_UP:			return "num_up";
	case WXK_NUMPAD_RIGHT:		return "num_right";
	case WXK_NUMPAD_DOWN:		return "num_down";
	case WXK_NUMPAD_PAGEUP:		return "num_pageup";
	case WXK_NUMPAD_PAGEDOWN:	return "num_pagedown";
	case WXK_NUMPAD_END:		return "num_end";
	case WXK_NUMPAD_BEGIN:		return "num_begin";
	case WXK_NUMPAD_INSERT:		return "num_insert";
	case WXK_NUMPAD_DELETE:		return "num_delete";
	case WXK_NUMPAD_EQUAL:		return "num_equal";
	case WXK_NUMPAD_MULTIPLY:	return "num_multiply";
	case WXK_NUMPAD_ADD:		return "num_plus";
	case WXK_NUMPAD_SEPARATOR:	return "num_separator";
	case WXK_NUMPAD_SUBTRACT:	return "num_minus";
	case WXK_NUMPAD_DECIMAL:	return "num_decimal";
	case WXK_NUMPAD_DIVIDE:		return "num_divide";
	case WXK_WINDOWS_LEFT:		return "win_left";
	case WXK_WINDOWS_RIGHT:		return "win_right";
	case WXK_WINDOWS_MENU:		return "win_menu";
	case WXK_PRINT:				return "printscrn";
#ifdef __APPLE__
	case WXK_COMMAND:			return "command";
#else
	case WXK_CONTROL:			return "control";
#endif
	default: break;
	};

	// Check for ascii character
	if (key > 32 && key < 128)
		return (char)key;

	// Unknown character, just return "key##"
	return S_FMT("key%d", key);
}

/* KeyBind::mbName
 * Returns a string representation of mouse [button]
 *******************************************************************/
string KeyBind::mbName(int button)
{
	switch (button)
	{
	case wxMOUSE_BTN_LEFT:		return "mouse1";
	case wxMOUSE_BTN_RIGHT:		return "mouse2";
	case wxMOUSE_BTN_MIDDLE:	return "mouse3";
	case wxMOUSE_BTN_AUX1:		return "mouse4";
	case wxMOUSE_BTN_AUX2:		return "mouse5";
	default:
		return S_FMT("mouse%d", button);
	};
}

/* KeyBind::keyPressed
 * 'Presses' all keybinds bound to [key]
 *******************************************************************/
bool KeyBind::keyPressed(keypress_t key)
{
	// Ignore raw modifier keys
	if (key.key == "control" || key.key == "shift" || key.key == "alt" || key.key == "command")
		return false;

	// Go through all keybinds
	// (use sorted list for priority system)
	bool pressed = false;
	for (unsigned k = 0; k < keybinds_sorted.size(); k++)
	{
		KeyBind& kb = keybinds_sorted[k];

		// Go through all keys bound to this keybind
		for (unsigned a = 0; a < kb.keys.size(); a++)
		{
			keypress_t& kp = kb.keys[a];

			// Check for match with keypress
			if ((kp.shift == key.shift || kb.ignore_shift) &&
			        kp.alt == key.alt &&
			        kp.ctrl == key.ctrl &&
			        kp.key == key.key)
			{
				// Set bind state
				getBind(kb.name).pressed = true;

				// Send key pressed event to keybind handlers
				for (unsigned b = 0; b < kb_handlers.size(); b++)
					kb_handlers[b]->onKeyBindPress(kb.name);

				pressed = true;
				break;
			}
		}
	}

	return pressed;
}

/* KeyBind::keyRelease
 * 'Releases' all keybinds bound to [key]
 *******************************************************************/
bool KeyBind::keyReleased(string key)
{
	// Ignore raw modifier keys
	if (key == "control" || key == "shift" || key == "alt" || key == "command")
		return false;

	// Go through all keybinds
	bool released = false;
	for (unsigned k = 0; k < keybinds.size(); k++)
	{
		KeyBind& kb = keybinds[k];

		// Go through all keys bound to this keybind
		for (unsigned a = 0; a < kb.keys.size(); a++)
		{
			// Check for match with keypress, and that it is currently pressed
			if (kb.keys[a].key == key && kb.pressed)
			{
				// Set bind state
				kb.pressed = false;

				// Send key released event to keybind handlers
				for (unsigned b = 0; b < kb_handlers.size(); b++)
					kb_handlers[b]->onKeyBindRelease(kb.name);

				released = true;
				break;
			}
		}
	}

	return released;
}

/* KeyBind::pressBind
 * 'Presses' the keybind [name]
 *******************************************************************/
void KeyBind::pressBind(string name)
{
	for (unsigned a = 0; a < keybinds.size(); a++)
	{
		if (keybinds[a].name == name)
		{
			// Send key pressed event to keybind handlers
			for (unsigned b = 0; b < kb_handlers.size(); b++)
				kb_handlers[b]->onKeyBindPress(name);

			return;
		}
	}
}

/* KeyBind::asKeyPress
 * Returns [keycode] and [modifiers] as a keypress_t struct
 *******************************************************************/
keypress_t KeyBind::asKeyPress(int keycode, int modifiers)
{
	return keypress_t(keyName(keycode),
	                  ((modifiers & wxMOD_ALT) != 0),
	                  ((modifiers & wxMOD_CMD) != 0),
	                  ((modifiers & wxMOD_SHIFT) != 0));
}

/* KeyBind::allKeyBinds
 * Adds all keybinds to [list]
 *******************************************************************/
void KeyBind::allKeyBinds(vector<KeyBind*>& list)
{
	for (unsigned a = 0; a < keybinds.size(); a++)
		list.push_back(&keybinds[a]);
}

/* KeyBind::releaseAll
 * 'Releases' all keybinds
 *******************************************************************/
void KeyBind::releaseAll()
{
	for (unsigned a = 0; a < keybinds.size(); a++)
		keybinds[a].pressed = false;
}

/* KeyBind::initBinds
 * Initialises all default keybinds
 *******************************************************************/
void KeyBind::initBinds()
{
	// General
	string group = "General";
	addBind("copy", keypress_t("C", KPM_CTRL), "Copy", group);
	addBind("cut", keypress_t("X", KPM_CTRL), "Cut", group);
	addBind("paste", keypress_t("V", KPM_CTRL), "Paste", group);
	addBind("select_all", keypress_t("A", KPM_CTRL), "Select All", group);

	// Entry List (el*)
	group = "Entry List";
	addBind("el_new", keypress_t("N", KPM_CTRL), "New Entry", group);
	addBind("el_delete", keypress_t("delete"), "Delete Entry", group);
	addBind("el_move_up", keypress_t("U", KPM_CTRL), "Move Entry up", group);
	addBind("el_move_down", keypress_t("D", KPM_CTRL), "Move Entry down", group);
	addBind("el_rename", keypress_t("R", KPM_CTRL), "Rename Entry", group);
	addBind("el_rename", keypress_t("f2"));
	addBind("el_import", keypress_t("I", KPM_CTRL), "Import to Entry", group);
	addBind("el_import_files", keypress_t("I", KPM_CTRL|KPM_SHIFT), "Import Files", group);
	addBind("el_export", keypress_t("E", KPM_CTRL), "Export Entry", group);
	addBind("el_up_dir", keypress_t("backspace"), "Up one directory", group);

	// Text editor (ted*)
	group = "Text Editor";
	addBind("ted_autocomplete", keypress_t("space", KPM_CTRL), "Open Autocompletion list", group);
	addBind("ted_calltip", keypress_t("space", KPM_CTRL|KPM_SHIFT), "Open CallTip", group);
	addBind("ted_findreplace", keypress_t("F", KPM_CTRL), "Find/Replace", group);
	addBind("ted_findnext", keypress_t("f3"), "Find next", group);
	addBind("ted_findprev", keypress_t("f3", KPM_SHIFT), "Find previous", group);
	addBind("ted_replacenext", keypress_t("R", KPM_ALT), "Replace next", group);
	addBind("ted_replaceall", keypress_t("R", KPM_ALT|KPM_SHIFT), "Replace all", group);
	addBind("ted_jumptoline", keypress_t("G", KPM_CTRL), "Jump to Line", group);
	addBind("ted_fold_foldall", keypress_t("[", KPM_CTRL|KPM_SHIFT), "Fold All", group);
	addBind("ted_fold_unfoldall", keypress_t("]", KPM_CTRL|KPM_SHIFT), "Fold All", group);

	// Texture editor (txed*)
	group = "Texture Editor";
	addBind("txed_patch_left", keypress_t("left", KPM_CTRL), "Move Patch left", group);
	addBind("txed_patch_left8", keypress_t("left"), "Move Patch left 8", group);
	addBind("txed_patch_up", keypress_t("up", KPM_CTRL), "Move Patch up", group);
	addBind("txed_patch_up8", keypress_t("up"), "Move Patch up 8", group);
	addBind("txed_patch_right", keypress_t("right", KPM_CTRL), "Move Patch right", group);
	addBind("txed_patch_right8", keypress_t("right"), "Move Patch right 8", group);
	addBind("txed_patch_down", keypress_t("down", KPM_CTRL), "Move Patch down", group);
	addBind("txed_patch_down8", keypress_t("down"), "Move Patch down 8", group);
	addBind("txed_patch_add", keypress_t("insert"), "Add Patch", group);
	addBind("txed_patch_delete", keypress_t("delete"), "Delete Patch", group);
	addBind("txed_patch_replace", keypress_t("f2"), "Replace Patch", group);
	addBind("txed_patch_replace", keypress_t("R", KPM_CTRL));
	addBind("txed_patch_duplicate", keypress_t("D", KPM_CTRL), "Duplicate Patch", group);
	addBind("txed_patch_forward", keypress_t("]"), "Bring Patch forward", group);
	addBind("txed_patch_back", keypress_t("["), "Send Patch back", group);
	addBind("txed_tex_up", keypress_t("up", KPM_CTRL), "Move Texture up", group);
	addBind("txed_tex_up", keypress_t("U", KPM_CTRL));
	addBind("txed_tex_down", keypress_t("down", KPM_CTRL), "Move Texture down", group);
	addBind("txed_tex_down", keypress_t("D", KPM_CTRL));
	addBind("txed_tex_new", keypress_t("N", KPM_CTRL), "New Texture", group);
	addBind("txed_tex_new_patch", keypress_t("N", KPM_CTRL|KPM_SHIFT), "New Texture from Patch", group);
	addBind("txed_tex_new_file", keypress_t("N", KPM_CTRL|KPM_ALT), "New Texture from File", group);
	addBind("txed_tex_delete", keypress_t("delete"), "Delete Texture", group);

	// Map Editor (map*)
	group = "Map Editor General";
	addBind("map_edit_accept", keypress_t("return"), "Accept edit", group);
	addBind("map_edit_accept", keypress_t("num_enter"));
	addBind("map_edit_cancel", keypress_t("escape"), "Cancel edit", group);
	addBind("map_toggle_3d", keypress_t("Q"), "Toggle 3d mode", group);
	addBind("map_screenshot", keypress_t("P", KPM_CTRL|KPM_SHIFT), "Take Screenshot", group);

	// Map Editor 2D (me2d*)
	group = "Map Editor 2D Mode";
	addBind("me2d_clear_selection", keypress_t("C"), "Clear selection", group);
	addBind("me2d_pan_view", keypress_t("mouse3"), "Pan view", group);
	addBind("me2d_pan_view", keypress_t("space", KPM_CTRL));
	addBind("me2d_move", keypress_t("Z"), "Toggle item move mode", group);
	addBind("me2d_zoom_in_m", keypress_t("mwheelup"), "Zoom in (towards mouse)", group);
	addBind("me2d_zoom_out_m", keypress_t("mwheeldown"), "Zoom out (towards mouse)", group);
	addBind("me2d_zoom_in", keypress_t("="), "Zoom in (towards screen center)", group);
	addBind("me2d_zoom_out", keypress_t("-"), "Zoom out (towards screen center)", group);
	addBind("me2d_show_object", keypress_t("=", KPM_SHIFT), "Zoom in, show current object", group);
	addBind("me2d_show_object", keypress_t("mwheelup", KPM_SHIFT));
	addBind("me2d_show_all", keypress_t("-", KPM_SHIFT), "Zoom out, show full map", group);
	addBind("me2d_show_all", keypress_t("mwheeldown", KPM_SHIFT));
	addBind("me2d_left", keypress_t("left"), "Scroll left", group);
	addBind("me2d_right", keypress_t("right"), "Scroll right", group);
	addBind("me2d_up", keypress_t("up"), "Scroll up", group);
	addBind("me2d_down", keypress_t("down"), "Scroll down", group);
	addBind("me2d_grid_inc", keypress_t("["), "Increment grid level", group);
	addBind("me2d_grid_dec", keypress_t("]"), "Decrement grid level", group);
	addBind("me2d_grid_toggle_snap", keypress_t("G", KPM_SHIFT), "Toggle Grid Snap", group);
	addBind("me2d_mode_vertices", keypress_t("V"), "Vertices mode", group);
	addBind("me2d_mode_lines", keypress_t("L"), "Lines mode", group);
	addBind("me2d_mode_sectors", keypress_t("S"), "Sectors mode", group);
	addBind("me2d_mode_things", keypress_t("T"), "Things mode", group);
	addBind("me2d_flat_type", keypress_t("F", KPM_CTRL), "Cycle flat type", group);
	addBind("me2d_split_line", keypress_t("S", KPM_CTRL|KPM_SHIFT), "Split nearest line", group);
	addBind("me2d_lock_hilight", keypress_t("H", KPM_CTRL), "Lock/unlock hilight", group);
	addBind("me2d_begin_linedraw", keypress_t("space"), "Begin line drawing", group);
	addBind("me2d_begin_shapedraw", keypress_t("space", KPM_SHIFT), "Begin shape drawing", group);
	addBind("me2d_create_object", keypress_t("insert"), "Create object", group);
	addBind("me2d_delete_object", keypress_t("delete"), "Delete object", group);
	addBind("me2d_copy_properties", keypress_t("C", KPM_CTRL|KPM_SHIFT), "Copy object properties", group);
	addBind("me2d_paste_properties", keypress_t("V", KPM_CTRL|KPM_SHIFT), "Paste object properties", group);
	addBind("me2d_begin_object_edit", keypress_t("E"), "Begin object edit", group);
	addBind("me2d_toggle_selection_numbers", keypress_t("N"), "Toggle selection numbers", group);
	addBind("me2d_mirror_x", keypress_t("M", KPM_CTRL), "Mirror selection horizontally", group);
	addBind("me2d_mirror_y", keypress_t("M", KPM_CTRL|KPM_SHIFT), "Mirror selection vertically", group);
	addBind("me2d_object_properties", keypress_t("return"), "Object Properties", group, false, 100);

	// Map Editor 2D Lines mode (me2d_line*)
	group = "Map Editor 2D Lines Mode";
	addBind("me2d_line_change_texture", keypress_t("T", KPM_CTRL), "Change texture(s)", group);
	addBind("me2d_line_flip", keypress_t("F"), "Flip line(s)", group);
	addBind("me2d_line_flip_nosides", keypress_t("F", KPM_SHIFT), "Flip line(s) but not sides", group);
	addBind("me2d_line_tag_edit", keypress_t("T", KPM_SHIFT), "Begin tag edit", group);

	// Map Editor 2D Sectors mode (me2d_sector*)
	group = "Map Editor 2D Sectors Mode";
	addBind("me2d_sector_light_up16", keypress_t("'"), "Light level up 16", group);
	addBind("me2d_sector_light_up", keypress_t("'", KPM_SHIFT), "Light level up 1", group);
	addBind("me2d_sector_light_down16", keypress_t(";"), "Light level down 16", group);
	addBind("me2d_sector_light_down", keypress_t(";", KPM_SHIFT), "Light level down 1", group);
	addBind("me2d_sector_floor_up8", keypress_t(".", KPM_CTRL), "Floor height up 8", group);
	addBind("me2d_sector_floor_up", keypress_t(".", KPM_CTRL|KPM_SHIFT), "Floor height up 1", group);
	addBind("me2d_sector_floor_down8", keypress_t(",", KPM_CTRL), "Floor height down 8", group);
	addBind("me2d_sector_floor_down", keypress_t(",", KPM_CTRL|KPM_SHIFT), "Floor height down 1", group);
	addBind("me2d_sector_ceil_up8", keypress_t(".", KPM_ALT), "Ceiling height up 8", group);
	addBind("me2d_sector_ceil_up", keypress_t(".", KPM_ALT|KPM_SHIFT), "Ceiling height up 1", group);
	addBind("me2d_sector_ceil_down8", keypress_t(",", KPM_ALT), "Ceiling height down 8", group);
	addBind("me2d_sector_ceil_down", keypress_t(",", KPM_ALT|KPM_SHIFT), "Ceiling height down 1", group);
	addBind("me2d_sector_height_up8", keypress_t("."), "Height up 8", group);
	addBind("me2d_sector_height_up", keypress_t(".", KPM_SHIFT), "Height up 1", group);
	addBind("me2d_sector_height_down8", keypress_t(","), "Height down 8", group);
	addBind("me2d_sector_height_down", keypress_t(",", KPM_SHIFT), "Height down 1", group);
	addBind("me2d_sector_change_texture", keypress_t("T", KPM_CTRL), "Change texture(s)", group);
	addBind("me2d_sector_join", keypress_t("J"), "Join sectors", group);
	addBind("me2d_sector_join_keep", keypress_t("J", KPM_SHIFT), "Join sectors (keep lines)", group);

	// Map Editor 2D Things mode (me2d_thing*)
	group = "Map Editor 2D Things Mode";
	addBind("me2d_thing_change_type", keypress_t("T", KPM_CTRL), "Change type", group);
	addBind("me2d_thing_quick_angle", keypress_t("D"), "Quick angle edit", group);

	// Map Editor 3D (me3d*)
	group = "Map Editor 3D Mode";
	addBind("me3d_toggle_fog", keypress_t("F"), "Toggle fog", group);
	addBind("me3d_toggle_fullbright", keypress_t("B"), "Toggle full brightness", group);
	addBind("me3d_adjust_brightness", keypress_t("B", KPM_SHIFT), "Adjust brightness", group);
	addBind("me3d_toggle_gravity", keypress_t("G"), "Toggle camera gravity", group);
	addBind("me3d_release_mouse", keypress_t("tab"), "Release mouse cursor", group);
	addBind("me3d_clear_selection", keypress_t("C"), "Clear selection", group);
	addBind("me3d_toggle_things", keypress_t("T"), "Toggle thing display", group);
	addBind("me3d_thing_style", keypress_t("T", KPM_SHIFT), "Cycle thing render style", group);
	addBind("me3d_toggle_hilight", keypress_t("H"), "Toggle hilight", group);
	addBind("me3d_copy_tex_type", keypress_t("C", KPM_CTRL), "Copy texture or thing type", group);
	addBind("me3d_copy_tex_type", keypress_t("mouse3"));
	addBind("me3d_paste_tex_type", keypress_t("V", KPM_CTRL), "Paste texture or thing type", group);
	addBind("me3d_paste_tex_type", keypress_t("mouse3", KPM_CTRL));
	addBind("me3d_paste_tex_adj", keypress_t("mouse3", KPM_SHIFT), "Flood-fill texture", group);
	addBind("me3d_toggle_info", keypress_t("I"), "Toggle information overlay", group);
	addBind("me3d_quick_texture", keypress_t("T", KPM_CTRL), "Quick Texture", group);
	addBind("me3d_generic_up8", keypress_t("mwheelup", KPM_CTRL), "Raise target 8", group);
	addBind("me3d_generic_up", keypress_t("mwheelup", KPM_CTRL|KPM_SHIFT), "Raise target 1", group);
	addBind("me3d_generic_down8", keypress_t("mwheeldown", KPM_CTRL), "Lower target 8", group);
	addBind("me3d_generic_down", keypress_t("mwheeldown", KPM_CTRL|KPM_SHIFT), "Lower target 1", group);

	// Map Editor 3D Camera (me3d_camera*)
	group = "Map Editor 3D Mode Camera";
	addBind("me3d_camera_forward", keypress_t("W"), "Camera forward", group, true);
	addBind("me3d_camera_back", keypress_t("S"), "Camera backward", group, true);
	addBind("me3d_camera_left", keypress_t("A"), "Camera strafe left", group, true);
	addBind("me3d_camera_right", keypress_t("D"), "Camera strafe right", group, true);
	addBind("me3d_camera_up", keypress_t("up"), "Camera move up", group, true);
	addBind("me3d_camera_down", keypress_t("down"), "Camera move down", group, true);
	addBind("me3d_camera_turn_left", keypress_t("left"), "Camera turn left", group, true);
	addBind("me3d_camera_turn_right", keypress_t("right"), "Camera turn right", group, true);

	// Map Editor 3D Light (me3d_light*)
	group = "Map Editor 3D Mode Light";
	addBind("me3d_light_up16", keypress_t("'"), "Sector light level up 16", group);
	addBind("me3d_light_up", keypress_t("'", KPM_SHIFT), "Sector light level up 1", group);
	addBind("me3d_light_down16", keypress_t(";"), "Sector light level down 16", group);
	addBind("me3d_light_down", keypress_t(";", KPM_SHIFT), "Sector light level down 1", group);
	addBind("me3d_light_toggle_link", keypress_t("L", KPM_CTRL), "Toggle linked flat light levels", group);

	// Map Editor 3D Offsets (me3d_xoff*, me3d_yoff*)
	group = "Map Editor 3D Mode Offsets";
	addBind("me3d_xoff_up8", keypress_t("num_4"), "X offset up 8", group);
	addBind("me3d_xoff_up", keypress_t("num_left"), "X offset up 1", group);
	addBind("me3d_xoff_down8", keypress_t("num_6"), "X offset down 8", group);
	addBind("me3d_xoff_down", keypress_t("num_right"), "X offset down 1", group);
	addBind("me3d_yoff_up8", keypress_t("num_8"), "Y offset up 8", group);
	addBind("me3d_yoff_up", keypress_t("num_up"), "Y offset up 1", group);
	addBind("me3d_yoff_down8", keypress_t("num_2"), "Y offset down 8", group);
	addBind("me3d_yoff_down", keypress_t("num_down"), "Y offset down 1", group);
	addBind("me3d_wall_reset", keypress_t("R"), "Reset offsets and scaling", group);
#ifdef __WXGTK__
	addBind("me3d_xoff_up", keypress_t("num_left", KPM_SHIFT));
	addBind("me3d_xoff_down", keypress_t("num_right", KPM_SHIFT));
	addBind("me3d_yoff_up", keypress_t("num_up", KPM_SHIFT));
	addBind("me3d_yoff_down", keypress_t("num_down", KPM_SHIFT));
#endif

	// Map Editor 3D Scaling (me3d_scale*)
	group = "Map Editor 3D Mode Scaling";
	addBind("me3d_scalex_up_l", keypress_t("num_4", KPM_CTRL), "X scale up (large)", group);
	addBind("me3d_scalex_up_s", keypress_t("num_left", KPM_CTRL), "X scale up (small)", group);
	addBind("me3d_scalex_down_l", keypress_t("num_6", KPM_CTRL), "X scale down (large)", group);
	addBind("me3d_scalex_down_s", keypress_t("num_right", KPM_CTRL), "X scale down (small)", group);
	addBind("me3d_scaley_up_l", keypress_t("num_8", KPM_CTRL), "Y scale up (large)", group);
	addBind("me3d_scaley_up_s", keypress_t("num_up", KPM_CTRL), "Y scale up (small)", group);
	addBind("me3d_scaley_down_l", keypress_t("num_2", KPM_CTRL), "Y scale down (large)", group);
	addBind("me3d_scaley_down_s", keypress_t("num_down", KPM_CTRL), "Y scale down (small)", group);

	// Map Editor 3D Walls (me3d_wall*)
	group = "Map Editor 3D Mode Walls";
	addBind("me3d_wall_toggle_link_ofs", keypress_t("O", KPM_CTRL), "Toggle linked wall offsets", group);
	addBind("me3d_wall_autoalign_x", keypress_t("A", KPM_CTRL), "Auto-align textures on X", group);
	addBind("me3d_wall_unpeg_lower", keypress_t("L"), "Toggle lower unpegged", group);
	addBind("me3d_wall_unpeg_upper", keypress_t("U"), "Toggle upper unpegged", group);

	// Map Editor 3D Flats (me3d_flat*)
	group = "Map Editor 3D Mode Flats";
	addBind("me3d_flat_height_up8", keypress_t("num_plus"), "Height up 8", group);
	addBind("me3d_flat_height_up8", keypress_t("mwheelup"));
	addBind("me3d_flat_height_up", keypress_t("num_plus", KPM_SHIFT), "Height up 1", group);
	addBind("me3d_flat_height_up", keypress_t("mwheelup", KPM_SHIFT));
	addBind("me3d_flat_height_down8", keypress_t("num_minus"), "Height down 8", group);
	addBind("me3d_flat_height_down8", keypress_t("mwheeldown"));
	addBind("me3d_flat_height_down", keypress_t("num_minus", KPM_SHIFT), "Height down 1", group);
	addBind("me3d_flat_height_down", keypress_t("mwheeldown", KPM_SHIFT));

	// Map Editor 3D Things (me3d_thing*)
	group = "Map Editor 3D Mode Things";
	addBind("me3d_thing_remove", keypress_t("delete"), "Remove", group);
	addBind("me3d_thing_up8", keypress_t("num_8"), "Z up 8", group);
	addBind("me3d_thing_up", keypress_t("num_up"), "Z up 1", group);
	addBind("me3d_thing_down8", keypress_t("num_2"), "Z down 8", group);
	addBind("me3d_thing_down", keypress_t("num_down"), "Z down 1", group);

	// Set above keys as defaults
	for (unsigned a = 0; a < keybinds.size(); a++)
	{
		for (unsigned k = 0; k < keybinds[a].keys.size(); k++)
			keybinds[a].defaults.push_back(keybinds[a].keys[k]);
	}

	// Create sorted list
	keybinds_sorted = keybinds;
	std::sort(keybinds_sorted.begin(), keybinds_sorted.end());
}

/* KeyBind::writeBinds
 * Writes all keybind definitions as a string
 *******************************************************************/
string KeyBind::writeBinds()
{
	// Init string
	string ret = "";

	// Go through all keybinds
	for (unsigned k = 0; k < keybinds.size(); k++)
	{
		KeyBind& kb = keybinds[k];

		// Add keybind line
		ret += "\t";
		ret += kb.name;

		// 'unbound' indicates no binds
		if (kb.keys.size() == 0)
			ret += " unbound";

		// Go through all bound keys
		for (unsigned a = 0; a < kb.keys.size(); a++)
		{
			keypress_t& kp = kb.keys[a];
			ret += " \"";

			// Add modifiers (if any)
			if (kp.alt)
				ret += "a";
			if (kp.ctrl)
				ret += "c";
			if (kp.shift)
				ret += "s";
			if (kp.alt || kp.ctrl || kp.shift)
				ret += "|";

			// Add key
			ret += kp.key;
			ret += "\"";

			// Add comma if there are any more keys
			if (a < kb.keys.size() - 1)
				ret += ",";
		}

		ret += "\n";
	}

	return ret;
}

/* KeyBind::readBinds
 * Reads keybind defeinitions from tokenizer [tz]
 *******************************************************************/
bool KeyBind::readBinds(Tokenizer& tz)
{
	// Parse until ending }
	string name = tz.getToken();
	while (name != "}" && !tz.atEnd())
	{
		// Clear any current binds for the key
		getBind(name).keys.clear();

		// Read keys
		while (1)
		{
			string keystr = tz.getToken();

			// Finish if no keys are bound
			if (keystr == "unbound")
				break;

			// Parse key string
			string key, mods;
			if (keystr.Find("|") >= 0)
			{
				mods = keystr.BeforeFirst('|');
				key = keystr.AfterFirst('|');
			}
			else
				key = keystr;

			// Add the key
			addBind(name, keypress_t(key, mods.Find('a') >= 0, mods.Find('c') >= 0, mods.Find('s') >= 0));

			// Check for more keys
			if (tz.peekToken() == ",")
				tz.getToken();			// Skip ,
			else
				break;
		}

		// Next keybind
		name = tz.getToken();
	}

	// Create sorted list
	keybinds_sorted = keybinds;
	std::sort(keybinds_sorted.begin(), keybinds_sorted.end());

	return true;
}

/* KeyBind::updateSortedBindsList
 * Updates the sorted keybinds list
 *******************************************************************/
void KeyBind::updateSortedBindsList()
{
	// Create sorted list
	keybinds_sorted.clear();
	keybinds_sorted = keybinds;
	std::sort(keybinds_sorted.begin(), keybinds_sorted.end());
}


/*******************************************************************
 * KEYBINDHANDLER CLASS FUNCTIONS
 *******************************************************************/

/* KeyBindHandler::KeyBindHandler
 * KeyBindHandler class constructor
 *******************************************************************/
KeyBindHandler::KeyBindHandler()
{
	kb_handlers.push_back(this);
}

/* KeyBindHandler::~KeyBindHandler
 * KeyBindHandler class destructor
 *******************************************************************/
KeyBindHandler::~KeyBindHandler()
{
	for (unsigned a = 0; a < kb_handlers.size(); a++)
	{
		if (kb_handlers[a] == this)
		{
			kb_handlers.erase(kb_handlers.begin() + a);
			a--;
		}
	}
}

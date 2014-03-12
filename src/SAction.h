
#ifndef __SACTION_H__
#define __SACTION_H__

class wxMenu;
class wxAuiToolBar;
class wxToolBar;
class MainApp;

class SAction
{
friend class MainApp;
private:
	string		id;		// The id associated with this action - to keep things consistent, it should be of the format xxxx_*,
	// where xxxx is some 4 letter identifier for the SActionHandler that handles this action
	int			wx_id;
	string		text;
	string		icon;
	string		helptext;
	string		shortcut;
	int			type;
	int			group;
	bool		toggled;
	string		keybind;

	static int	n_groups;

public:
	// Enum for action types
	enum
	{
	    NORMAL,
	    CHECK,
	    RADIO,
	};

	SAction(string id, string text, string icon = "", string helptext = "", string shortcut = "", int type = NORMAL, int custom_wxid = -1, int radio_group = -1);
	~SAction();

	string	getId() { return id; }
	int		getWxId() { return wx_id; }
	string	getText() { return text; }
	string	getIconName() { return icon; }
	string	getHelpText() { return helptext; }
	string	getShortcut() { return shortcut; }
	string	getShortcutText();
	bool	isToggled() { return toggled; }
	bool	isRadio() { return type == RADIO; }

	bool	addToMenu(wxMenu* menu, string text_override = "NO");
	bool	addToMenu(wxMenu* menu, bool menubar, string text_override = "NO");
	bool	addToToolbar(wxAuiToolBar* toolbar, string icon_override = "NO");
	bool	addToToolbar(wxToolBar* toolbar, string icon_override = "NO");

	// Static functions
	static int newGroup()
	{
		int group = n_groups;
		n_groups++;
		return group;
	}
};

#endif//__SACTION_H__


#ifndef __SACTION_H__
#define __SACTION_H__

class wxMenu;
class wxAuiToolBar;
class wxToolBar;
class MainApp;
class CBoolCVar;

class SAction
{
friend class MainApp;
private:
	// The id associated with this action - to keep things consistent, it should be of the format xxxx_*,
	// where xxxx is some 4 letter identifier for the SActionHandler that handles this action
	string		id;

	int			wx_id;
	int			reserved_ids;	// Can reserve a range of wx ids
	string		text;
	string		icon;
	string		helptext;
	string		shortcut;
	int			type;
	int			group;
	bool		toggled;
	string		keybind;
	CBoolCVar*	linked_cvar;

	static int	n_groups;

public:
	// Enum for action types
	enum
	{
	    NORMAL,
	    CHECK,
	    RADIO,
	};

	SAction(
		string id,
		string text,
		string icon = "",
		string helptext = "",
		string shortcut = "",
		int type = NORMAL,
		int custom_wxid = -1,
		int radio_group = -1,
		int reserve_ids = 1,
		string linked_cvar = ""
		);
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
	bool	isWxId(int id) { return id >= wx_id && id < wx_id + reserved_ids; }
	void	setToggled(bool toggle = true);

	bool	addToMenu(wxMenu* menu, string text_override = "NO", string icon_override = "NO", int wx_id_offset = 0);
	bool	addToMenu(
				wxMenu* menu,
				bool show_shortcut,
				string text_override = "NO",
				string icon_override = "NO",
				int wx_id_offset = 0
				);
	bool	addToToolbar(wxAuiToolBar* toolbar, string icon_override = "NO", int wx_id_offset = 0);
	bool	addToToolbar(wxToolBar* toolbar, string icon_override = "NO", int wx_id_offset = 0);

	// Static functions
	static int newGroup()
	{
		int group = n_groups;
		n_groups++;
		return group;
	}
};

// Basic 'interface' class for classes that handle SActions (yay multiple inheritance)
class SActionHandler
{
	friend class MainApp;
protected:
	int	wx_id_offset;

	virtual bool	handleAction(string id) { return false; }

public:
	SActionHandler();
	virtual ~SActionHandler();
};

#endif//__SACTION_H__

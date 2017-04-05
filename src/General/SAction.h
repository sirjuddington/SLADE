
#ifndef __SACTION_H__
#define __SACTION_H__

class ParseTreeNode;
class wxMenu;
class wxAuiToolBar;
class wxToolBar;
class CBoolCVar;

class SAction
{
public:
	// Enum for action types
	enum class Type
	{
	    Normal,
	    Check,
	    Radio,
	};

	SAction(
		string id,
		string text,
		string icon = "",
		string helptext = "",
		string shortcut = "",
		Type type = Type::Normal,
		int radio_group = -1,
		int reserve_ids = 1
		);
	~SAction();

	string	getId() const { return id; }
	int		getWxId() const { return wx_id; }
	string	getText() const { return text; }
	string	getIconName() const { return icon; }
	string	getHelpText() const { return helptext; }
	string	getShortcut() const { return shortcut; }
	string	getShortcutText() const;
	Type	getType() const { return type; }
	bool	isChecked() const { return checked; }
	bool	isRadio() const { return type == Type::Radio; }
	bool	isWxId(int id) const { return id >= wx_id && id < wx_id + reserved_ids; }
	void	setChecked(bool checked = true);
	void	toggle() { setChecked(!checked); }

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
	static void		initWxId(int id) { cur_id = id; }
	static bool		initActions();
	static int		newGroup();
	static SAction*	fromId(const string& id);
	static SAction*	fromWxId(int wx_id);

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
	Type		type;
	int			group;
	bool		checked;
	string		keybind;
	CBoolCVar*	linked_cvar;

	// Internal functions
	bool	parse(ParseTreeNode* node);

	// Static functions
	static SAction*	invalidAction();

	// Static variables
	static int				n_groups;
	static int				cur_id;
	static vector<SAction*>	actions;
	static SAction*			action_invalid;
};

// Basic 'interface' class for classes that handle SActions (yay multiple inheritance)
class SActionHandler
{
public:
	SActionHandler();
	virtual ~SActionHandler();

	static void	setWxIdOffset(int offset) { wx_id_offset = offset; }
	static bool	doAction(string id);

protected:
	static int	wx_id_offset;

	virtual bool	handleAction(string id) { return false; }

private:
	static vector<SActionHandler*>	action_handlers;
};

#endif//__SACTION_H__

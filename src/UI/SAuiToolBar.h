#pragma once

#include "General/JsonFwd.h"
#include <wx/aui/auibar.h>

namespace slade
{
class SAction;
class SAuiToolBar : public wxAuiToolBar
{
	friend class SAuiToolBarArt;

public:
	SAuiToolBar(wxWindow* parent, bool vertical = false, bool main_toolbar = false, wxAuiManager* aui_mgr = nullptr);
	~SAuiToolBar() override = default;

	wxAuiToolBarItem* addAction(string_view action_id, bool show_name = false, string_view icon = {});
	wxAuiToolBarItem* addButton(
		string_view button_id,
		string_view text,
		string_view icon,
		string_view help_text,
		wxMenu*     menu      = nullptr,
		bool        show_name = false);

	void groupItems(string_view group_name, const vector<string>& item_ids);

	void setButtonDropdownMenu(string_view button_id, wxMenu* menu);
	void setButtonIcon(string_view button_id, string_view icon);

	void enableItem(string_view id, bool enable = true);
	void enableGroup(string_view group, bool enable = true);

	void showItem(string_view id, bool show = true, bool refresh = true);
	void showGroup(string_view group, bool show = true, bool refresh = true);

	bool itemChecked(string_view id) const;
	void setItemChecked(string_view id, bool checked);
	bool toggleItemChecked(string_view id);

	void registerCustomControl(const string& id, wxControl* control);
	void registerDropdownMenu(const string& button_id, wxMenu* menu);
	void loadLayout(string_view json, bool create = true);
	void loadLayoutFromResource(string_view entry_name, bool create = true);
	void createFromLayout();

	string actionFromWxId(int wx_id);

private:
	struct Item
	{
		string            id;
		SAction*          action    = nullptr;
		wxAuiToolBarItem* aui_item  = nullptr;
		wxMenu*           menu      = nullptr;
		int               wx_id     = -1;
		bool              show_text = false;
	};

	struct Group
	{
		string         name;
		bool           can_hide = false;
		vector<string> items;
	};

	struct CustomControl
	{
		string     name;
		wxControl* control = nullptr;
	};

	struct DropdownMenu
	{
		string  item_id;
		wxMenu* menu = nullptr;
	};

	vector<Item>          items_;
	vector<Group>         groups_;
	vector<CustomControl> custom_controls_;
	vector<DropdownMenu>  dropdown_menus_;
	unique_ptr<Json>      layout_;
	wxAuiManager*         aui_mgr_ = nullptr;

	const Item* itemByWxId(int wx_id) const;
	Item*       itemByWxId(int wx_id);
	const Item* itemById(string_view id) const;
	Item*       itemById(string_view id);
	Group*      groupByName(string_view name);

	void setItemChecked(wxAuiToolBarItem* item, bool checked);
};
} // namespace slade

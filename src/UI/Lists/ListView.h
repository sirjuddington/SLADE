#pragma once

class ListView : public wxListCtrl
{
public:
	enum class ItemStatus
	{
		Normal,
		Modified,
		New,
		Locked,
		Error,
		Disabled
	};

	ListView(wxWindow* parent, int id, long style = wxLC_REPORT);

	~ListView();
	bool showIcons() { return icons_; }
	void showIcons(bool show) { icons_ = show; }
	bool enableSizeUpdate() { return update_width_; }
	void enableSizeUpdate(bool update) { update_width_ = update; }

	bool addItem(int index, string text);
	bool addItem(int index, wxArrayString text);

	bool deleteItems(wxArrayInt items);

	ColRGBA disabledColour();
	bool    setItemStatus(int item, ItemStatus status);
	bool    setItemText(int item, int column, string text);

	void clearSelection();
	bool selectItem(int item, bool focus = true);
	bool deSelectItem(int item);

	wxArrayInt selectedItems();

	bool showItem(int item);
	bool swapItems(int item1, int item2);

	bool updateSize();

private:
	bool icons_;
	bool update_width_;
};

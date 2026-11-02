#pragma once

namespace slade
{
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
	~ListView() override = default;

	bool showIcons() const { return icons_; }
	void showIcons(bool show) { icons_ = show; }
	bool enableSizeUpdate() const { return update_width_; }
	void enableSizeUpdate(bool update) { update_width_ = update; }
	void setColumnSpacing(int spacing) { column_spacing_ = spacing; }

	bool addItem(int index, const string& text);
	bool addItem(int index, const vector<string>& text);

	bool deleteItems(wxArrayInt items);

	ColRGBA disabledColour() const;
	bool    setItemStatus(int item, ItemStatus status);
	bool    setItemText(int item, int column, const string& text);

	void clearSelection();
	bool selectItem(int item, bool focus = true);
	bool deSelectItem(int item);

	wxArrayInt selectedItems() const;

	bool showItem(int item);
	bool swapItems(int item1, int item2);

	bool updateSize();

private:
	bool icons_          = true;
	bool update_width_   = true;
	int  column_spacing_ = 0;
};
} // namespace slade

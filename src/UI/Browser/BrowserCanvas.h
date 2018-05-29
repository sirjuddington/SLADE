#pragma once

#include "UI/Canvas/OGLCanvas.h"
#include "BrowserItem.h"

class wxScrollBar;

class BrowserCanvas : public OGLCanvas
{
public:
	BrowserCanvas(wxWindow* parent);
	~BrowserCanvas() {}

	enum
	{
	    ITEMS_NORMAL = 0,
	    ITEMS_TILES = 1,

	    NAMES_NORMAL = 0,
	    NAMES_INDEX,
	    NAMES_NONE,
	};

	vector<BrowserItem*>&	itemList() { return items_; }
	int						getViewedIndex();
	void					addItem(BrowserItem* item);
	void					clearItems();
	int						fullItemSizeX();
	int						fullItemSizeY();
	void					draw() override;
	void					setScrollBar(wxScrollBar* scrollbar);
	void					updateLayout(int viewed_item = -1);
	BrowserItem*			getSelectedItem();
	BrowserItem*			itemAt(int index);
	int						itemIndex(BrowserItem* item);
	void					selectItem(int index);
	void					selectItem(BrowserItem* item);
	void					filterItems(string filter);
	void					showItem(int item, int where);
	void					showSelectedItem();
	bool					searchItemFrom(int from);
	void					setFont(int font) { this->font_ = font; }
	void					setItemNameType(int type) { this->show_names_ = type; }
	void					setItemSize(int size) { this->item_size_ = size; }
	void					setItemViewType(int type) { this->item_type_ = type; }
	int						longestItemTextWidth();

	// Events
	void	onSize(wxSizeEvent& e);
	void	onScrollThumbTrack(wxScrollEvent& e);
	void	onScrollLineUp(wxScrollEvent& e);
	void	onScrollLineDown(wxScrollEvent& e);
	void	onScrollPageUp(wxScrollEvent& e);
	void	onScrollPageDown(wxScrollEvent& e);
	void	onMouseEvent(wxMouseEvent& e);
	void	onKeyDown(wxKeyEvent& e);
	void	onKeyChar(wxKeyEvent& e);

private:
	vector<BrowserItem*>	items_;
	vector<int>				items_filter_;
	wxScrollBar*			scrollbar_		= nullptr;
	string					search_;
	BrowserItem*			item_selected_	= nullptr;

	// Display
	int	yoff_			= 0;
	int	item_border_	= 0;
	int	font_			= 0;
	int	show_names_		= 0;
	int	item_size_		= 0;
	int top_index_		= 0;
	int top_y_			= 0;
	int	item_type_		= 0;
	int	num_cols_		= 0;
};

DECLARE_EVENT_TYPE(wxEVT_BROWSERCANVAS_SELECTION_CHANGED, -1)

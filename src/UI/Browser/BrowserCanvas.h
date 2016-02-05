
#ifndef __BROWSER_CANVAS_H__
#define __BROWSER_CANVAS_H__

#include "UI/Canvas/OGLCanvas.h"
#include "BrowserItem.h"

class BrowserCanvas : public OGLCanvas
{
private:
	vector<BrowserItem*>	items;
	vector<int>				items_filter;
	wxScrollBar*			scrollbar;
	string					search;
	BrowserItem*			item_selected;

	// Display
	int	yoff;
	int	item_border;
	int	font;
	int	show_names;
	int	item_size;
	int top_index;
	int top_y;
	int	item_type;
	int	longest_text;
	int	num_cols;

public:
	BrowserCanvas(wxWindow* parent);
	~BrowserCanvas();

	enum
	{
	    ITEMS_NORMAL = 0,
	    ITEMS_TILES = 1,

	    NAMES_NORMAL = 0,
	    NAMES_INDEX,
	    NAMES_NONE,
	};

	vector<BrowserItem*>&	itemList() { return items; }
	int						getViewedIndex();
	void					addItem(BrowserItem* item);
	void					clearItems();
	int						fullItemSizeX();
	int						fullItemSizeY();
	void					draw();
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
	void					setFont(int font) { this->font = font; }
	void					setItemNameType(int type) { this->show_names = type; }
	void					setItemSize(int size) { this->item_size = size; }
	void					setItemViewType(int type) { this->item_type = type; }
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
};

DECLARE_EVENT_TYPE(wxEVT_BROWSERCANVAS_SELECTION_CHANGED, -1)

#endif//__BROWSER_CANVAS_H__

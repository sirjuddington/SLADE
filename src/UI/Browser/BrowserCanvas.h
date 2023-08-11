#pragma once

#include "UI/Canvas/GLCanvas.h"

class wxScrollBar;

namespace slade
{
class BrowserItem;
namespace gl::draw2d
{
	enum class Font;
}

class BrowserCanvas : public GLCanvas
{
public:
	BrowserCanvas(wxWindow* parent);
	~BrowserCanvas() override = default;

	enum class ItemView
	{
		Normal,
		Tiles
	};

	enum class NameType
	{
		Normal,
		Index,
		None
	};

	vector<BrowserItem*>& itemList() { return items_; }
	int                   getViewedIndex() const;
	void                  addItem(BrowserItem* item);
	void                  clearItems();
	int                   fullItemSizeX() const;
	int                   fullItemSizeY() const;
	void                  draw() override;
	void                  setScrollBar(wxScrollBar* scrollbar);
	void                  updateLayout(int viewed_index = -1);
	BrowserItem*          selectedItem() const;
	BrowserItem*          itemAt(int index) const;
	int                   itemIndex(const BrowserItem* item) const;
	void                  selectItem(int index);
	void                  selectItem(BrowserItem* item);
	void                  filterItems(wxString filter);
	void                  showItem(int item, int where);
	void                  showSelectedItem();
	bool                  searchItemFrom(int from);
	void                  setFont(gl::draw2d::Font font) { this->font_ = font; }
	void                  setItemNameType(NameType type) { this->show_names_ = type; }
	void                  setItemSize(int size) { this->item_size_ = size; }
	void                  setItemViewType(ItemView type) { this->item_type_ = type; }
	int                   longestItemTextWidth() const;

	// Events
	void onSize(wxSizeEvent& e);
	void onScrollThumbTrack(wxScrollEvent& e);
	void onScrollLineUp(wxScrollEvent& e);
	void onScrollLineDown(wxScrollEvent& e);
	void onScrollPageUp(wxScrollEvent& e);
	void onScrollPageDown(wxScrollEvent& e);
	void onMouseEvent(wxMouseEvent& e);
	void onKeyDown(wxKeyEvent& e);
	void onKeyChar(wxKeyEvent& e);

private:
	vector<BrowserItem*> items_;
	vector<int>          items_filter_;
	wxScrollBar*         scrollbar_ = nullptr;
	wxString             search_;
	BrowserItem*         item_selected_ = nullptr;

	// Display
	int              yoff_        = 0;
	int              item_border_ = 0;
	gl::draw2d::Font font_;
	NameType         show_names_ = NameType::Normal;
	int              item_size_  = -1;
	int              top_index_  = 0;
	int              top_y_      = 0;
	ItemView         item_type_  = ItemView::Normal;
	int              num_cols_   = -1;
};
} // namespace slade

DECLARE_EVENT_TYPE(wxEVT_BROWSERCANVAS_SELECTION_CHANGED, -1)

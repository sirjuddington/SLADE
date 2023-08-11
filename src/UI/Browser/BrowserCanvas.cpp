
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    BrowserCanvas.cpp
// Description: The OpenGL canvas for displaying browser items. Also keeps
//              track of a vertical scrollbar to scroll through the items it
//              contains
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "BrowserCanvas.h"
#include "BrowserItem.h"
#include "General/UI.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/Drawing.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, browser_bg_type, 0, CVar::Flag::Save)
CVAR(Int, browser_item_size, 96, CVar::Flag::Save)
DEFINE_EVENT_TYPE(wxEVT_BROWSERCANVAS_SELECTION_CHANGED)


// -----------------------------------------------------------------------------
//
// BrowserCanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// BrowserCanvas class constructor
// -----------------------------------------------------------------------------
BrowserCanvas::BrowserCanvas(wxWindow* parent) :
	GLCanvas{ parent }, item_border_{ ui::scalePx(8) }, font_{ gl::draw2d::Font::Bold }
{
	// Init canvas background style/colour
	ColRGBA col_bg = ColRGBA::BLACK;
	if (browser_bg_type == 1)
	{
		// 'System' style, get system panel background colour
		auto bgcolwx = drawing::systemPanelBGColour();
		col_bg.set(bgcolwx);
	}
	setBackground(browser_bg_type == 0 ? BGStyle::Checkered : BGStyle::Colour, col_bg);

	// Bind events
	Bind(wxEVT_SIZE, &BrowserCanvas::onSize, this);
	Bind(wxEVT_MOUSEWHEEL, &BrowserCanvas::onMouseEvent, this);
	Bind(wxEVT_LEFT_DOWN, &BrowserCanvas::onMouseEvent, this);
	Bind(wxEVT_KEY_DOWN, &BrowserCanvas::onKeyDown, this);
}

// -----------------------------------------------------------------------------
// Return the unfiltered index of the item currently in the middle of the
// viewport, or -1 if no items are visible
// -----------------------------------------------------------------------------
int BrowserCanvas::getViewedIndex() const
{
	if (items_filter_.empty())
		return -1;

	const wxSize size            = GetSize() * GetContentScaleFactor();
	int          viewport_height = size.y;
	int          row_height      = fullItemSizeY();
	int          viewport_mid_y  = yoff_ + viewport_height / 2.0;
	int          viewed_row      = viewport_mid_y / row_height;
	int          viewed_item_id  = (viewed_row + 0.5) * num_cols_;
	if (viewed_item_id < 0)
		viewed_item_id = 0;
	else if ((unsigned)viewed_item_id >= items_filter_.size())
		viewed_item_id = items_filter_.size() - 1;

	return items_filter_[viewed_item_id];
}

// -----------------------------------------------------------------------------
// Adds [item] to the list of items
// -----------------------------------------------------------------------------
void BrowserCanvas::addItem(BrowserItem* item)
{
	items_.push_back(item);
}

// -----------------------------------------------------------------------------
// Clears all items
// -----------------------------------------------------------------------------
void BrowserCanvas::clearItems()
{
	items_.clear();
}

// -----------------------------------------------------------------------------
// Returns the 'full' (including border) width of each item
// -----------------------------------------------------------------------------
int BrowserCanvas::fullItemSizeX() const
{
	int base_size;
	if (item_size_ > 0)
		base_size = item_size_ + (item_border_ * 2);
	else
		base_size = browser_item_size + (item_border_ * 2);

	if (item_type_ == ItemView::Tiles)
		return base_size + longestItemTextWidth() + item_border_ * 2;
	else
		return base_size;
}

// -----------------------------------------------------------------------------
// Returns the 'full' (including border and row gap) height of each item
// -----------------------------------------------------------------------------
int BrowserCanvas::fullItemSizeY() const
{
	int gap = 16;
	if (show_names_ == NameType::None || item_type_ == ItemView::Tiles)
		gap = 0;

	if (item_size_ > 0)
		return item_size_ + (item_border_ * 2) + gap;
	else
		return browser_item_size + (item_border_ * 2) + gap;
}

// -----------------------------------------------------------------------------
// Handles drawing of the canvas content
// -----------------------------------------------------------------------------
void BrowserCanvas::draw()
{
	gl::draw2d::Context dc(&view_);

	// Setup colours
	ColRGBA col_text    = ColRGBA::WHITE;
	bool    text_shadow = true;
	if (browser_bg_type == 1) // 'System' background style
	{
		// Get system text colour
		auto textcol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
		col_text.set(textcol);

		// Check text colour brightness, if it's dark don't draw text shadow
		auto col_temp = col_text;
		wxColor::MakeGrey(&col_temp.r, &col_temp.g, &col_temp.b);
		if (col_temp.r < 60)
			text_shadow = false;
	}

	// Selection rect
	Rectf rect_selection{ 2.0f, 2.0f, fullItemSizeX() - 3.0f, fullItemSizeY() - 3.0f };
	rect_selection.move(-item_border_, -item_border_);

	// Draw items
	int x         = item_border_;
	int y         = item_border_;
	int col_width = view_.size().x / num_cols_;
	int col       = 0;
	top_index_    = -1;
	for (unsigned a = 0; a < items_filter_.size(); a++)
	{
		// If we're not yet into the viewable area, skip
		if (y < yoff_ - fullItemSizeY())
		{
			col++;
			if (col >= num_cols_)
			{
				col = 0;
				y += fullItemSizeY();

				// Canvas is filled, stop drawing
				if (y > yoff_ + view_.size().y)
					break;
			}
			continue;
		}

		// If we're drawing the first non-hidden item, save it
		if (top_index_ < 0)
		{
			top_index_ = a;
			top_y_     = y - yoff_;
		}

		// Determine current x position
		int xgap = (col_width - fullItemSizeX()) * 0.5;
		x        = item_border_ + xgap + (col * col_width);

		// Move to item top-left
		dc.resetModel();
		dc.translate(x, y - yoff_);

		// Draw selection box if selected
		if (item_selected_ == items_[items_filter_[a]])
		{
			// Setup
			dc.texture = 0;
			dc.colour  = { 76, 128, 255, 76 };

			// Selection background
			dc.drawRect(rect_selection);

			// Selection border
			dc.line_thickness = 2.0f;
			dc.drawRectOutline(rect_selection);
		}

		// Draw item
		dc.colour          = col_text;
		dc.font            = font_;
		dc.text_dropshadow = text_shadow;
		items_[items_filter_[a]]->draw(item_size_ <= 0 ? browser_item_size : item_size_, dc, show_names_, item_type_);

		// Move over for next item
		col++;
		if (col >= num_cols_)
		{
			col = 0;
			y += fullItemSizeY();

			// Canvas is filled, stop drawing
			if (y > yoff_ + view_.size().y)
				break;
		}
	}
}

// -----------------------------------------------------------------------------
// Sets this canvas' associated vertical scrollbar
// -----------------------------------------------------------------------------
void BrowserCanvas::setScrollBar(wxScrollBar* scrollbar)
{
	// Set scrollbar
	scrollbar_ = scrollbar;

	// Bind events
	scrollbar->Bind(wxEVT_SCROLL_THUMBTRACK, &BrowserCanvas::onScrollThumbTrack, this);
	scrollbar->Bind(wxEVT_SCROLL_LINEUP, &BrowserCanvas::onScrollLineUp, this);
	scrollbar->Bind(wxEVT_SCROLL_LINEDOWN, &BrowserCanvas::onScrollLineDown, this);
	scrollbar->Bind(wxEVT_SCROLL_PAGEUP, &BrowserCanvas::onScrollPageUp, this);
	scrollbar->Bind(wxEVT_SCROLL_PAGEDOWN, &BrowserCanvas::onScrollPageDown, this);
}

// -----------------------------------------------------------------------------
// Updates variables concerning the object layout, then updates the associated
// scrollbar's properties depending on the number of items, the canvas size,
// etc.
// -----------------------------------------------------------------------------
void BrowserCanvas::updateLayout(int viewed_index)
{
	if (scrollbar_ && viewed_index < 0)
		viewed_index = getViewedIndex();

	// Determine number of columns
	const wxSize size = GetSize() * GetContentScaleFactor();
	num_cols_         = size.x / fullItemSizeX();
	if (num_cols_ == 0)
		num_cols_ = 1;

	// Update the scrollbar, if present
	if (scrollbar_)
	{
		// Try to keep the view scrolled to roughly the same area: find the
		// item currently in the middle, and keep it there
		// If the given item is no longer visible, find the first filtered item
		// after it that is
		int filtered_viewed_index = -1;
		for (unsigned a = 0; a < items_filter_.size(); a++)
			if (items_filter_[a] >= viewed_index)
			{
				filtered_viewed_index = a;
				break;
			}
		if (filtered_viewed_index < 0)
			filtered_viewed_index = items_filter_.size() - 1;

		// Determine total height of all items
		int rows            = (double)items_filter_.size() / (double)num_cols_ + 0.9999;
		int total_height    = rows * fullItemSizeY();
		int viewport_height = size.y;

		// Setup scrollbar
		scrollbar_->SetScrollbar(scrollbar_->GetThumbPosition(), viewport_height, total_height, viewport_height);
		showItem(filtered_viewed_index, 0);
	}

	// Update canvas background style/colour
	ColRGBA col_bg = ColRGBA::BLACK;
	if (browser_bg_type == 1)
	{
		// 'System' style, get system panel background colour
		auto bgcolwx = drawing::systemPanelBGColour();
		col_bg.set(bgcolwx);
	}
	setBackground(browser_bg_type == 0 ? BGStyle::Checkered : BGStyle::Colour, col_bg);

	Refresh();
}

// -----------------------------------------------------------------------------
// Returns the currently selected BrowserItem, or NULL if nothing is selected
// -----------------------------------------------------------------------------
BrowserItem* BrowserCanvas::selectedItem() const
{
	return item_selected_;
}

// -----------------------------------------------------------------------------
// Returns the currently BrowserItem at [index], taking the current filter into
// account
// -----------------------------------------------------------------------------
BrowserItem* BrowserCanvas::itemAt(int index) const
{
	// Check index
	if (index < 0 || index >= (int)items_filter_.size())
		return nullptr;

	return items_[items_filter_[index]];
}

// -----------------------------------------------------------------------------
// Returns the index of [item] taking the current filter into account
// -----------------------------------------------------------------------------
int BrowserCanvas::itemIndex(const BrowserItem* item) const
{
	// Search for the item in the current filtered list
	for (unsigned a = 0; a < items_filter_.size(); a++)
	{
		if ((unsigned)items_filter_[a] < items_.size() && items_[items_filter_[a]] == item)
			return a;
	}

	// Not found
	return -1;
}

// -----------------------------------------------------------------------------
// Selects the item [item]
// -----------------------------------------------------------------------------
void BrowserCanvas::selectItem(BrowserItem* item)
{
	// Check if we're clearing the selection
	if (item == nullptr)
		item_selected_ = nullptr;
	else
	{
		// Check the item exists in the current set
		for (auto& browser_item : items_)
		{
			if (browser_item == item)
				item_selected_ = item;
		}
	}

	// Generate event
	wxNotifyEvent e(wxEVT_BROWSERCANVAS_SELECTION_CHANGED, GetId());
	e.SetEventObject(this);
	GetEventHandler()->ProcessEvent(e);
}

// -----------------------------------------------------------------------------
// Selects the item at [index]
// -----------------------------------------------------------------------------
void BrowserCanvas::selectItem(int index)
{
	// Check index
	if (index < 0 || index >= (int)items_filter_.size())
		return;

	item_selected_ = items_[items_filter_[index]];

	// Generate event
	wxNotifyEvent e(wxEVT_BROWSERCANVAS_SELECTION_CHANGED, GetId());
	e.SetEventObject(this);
	GetEventHandler()->ProcessEvent(e);
}

// -----------------------------------------------------------------------------
// Filters the visible items by [filter], by name
// -----------------------------------------------------------------------------
void BrowserCanvas::filterItems(wxString filter)
{
	// Find the currently-viewed item before we change the item list
	int viewed_index = getViewedIndex();

	// Clear current filter list
	items_filter_.clear();

	// If the filter is empty, just add all items to the filter
	if (filter.IsEmpty())
	{
		for (unsigned a = 0; a < items_.size(); a++)
			items_filter_.push_back(a);
	}
	else
	{
		// Setup filter string
		filter.MakeLower();
		filter += "*";

		// Go through items
		for (unsigned a = 0; a < items_.size(); a++)
		{
			// Add to filter list if name matches
			if (items_[a]->name().Lower().Matches(filter))
				items_filter_.push_back(a);
		}
	}

	// Update scrollbar and refresh
	updateLayout(viewed_index);
}

// -----------------------------------------------------------------------------
// Scrolls the view to show [item] if it is currently off-screen.
// If [where] is positive, the item will be shown on the top row;
// if negative, the item will be shown on the bottom row;
// if zero, the item will be roughly centered.
// -----------------------------------------------------------------------------
void BrowserCanvas::showItem(int item, int where)
{
	// Check item index
	if (item < 0 || item >= (int)items_filter_.size())
		return;

	// Determine y-position of item
	const wxSize size     = GetSize() * GetContentScaleFactor();
	int          num_cols = size.x / fullItemSizeX();
	if (num_cols == 0)
		return;
	int y_top    = (item / num_cols) * fullItemSizeY();
	int y_bottom = y_top + fullItemSizeY();

	// Check if item is outside current view (but always center an item if
	// asked)
	if (y_top < yoff_ || y_bottom > yoff_ + size.y || where == 0)
	{
		if (where > 0)
			// Scroll view to show the item on the top row
			yoff_ = y_top;
		else if (where < 0)
			// Scroll view to show the item on the bottom row
			yoff_ = y_bottom - size.y;
		else
		{
			// Scroll view to put the item's middle in the middle of the canvas
			yoff_ = y_top + (fullItemSizeY() - size.y) / 2;
			if (yoff_ < 0)
				yoff_ = 0;
		}

		if (scrollbar_)
			scrollbar_->SetThumbPosition(yoff_);
	}
}

// -----------------------------------------------------------------------------
// Scrolls the view to show the currently selected item
// -----------------------------------------------------------------------------
void BrowserCanvas::showSelectedItem()
{
	showItem(itemIndex(item_selected_), 1);
}

// -----------------------------------------------------------------------------
// Used by BrowserCanvas::onKeyChar, returns true if an item matching [search]
// is found (starting from [from]), false otherwise
// -----------------------------------------------------------------------------
bool BrowserCanvas::searchItemFrom(int from)
{
	int  index  = from;
	bool looped = false;
	while ((!looped && index < (int)items_filter_.size()) || (looped && index < from))
	{
		wxString name = items_[items_filter_[index]]->name();
		if (name.Upper().StartsWith(search_))
		{
			// Matches, update selection
			selectItem(index);
			showSelectedItem();
			return true;
		}

		// No match, next item; look in the above entries
		// if no matches were found below.
		if (++index == (int)items_filter_.size() && !looped)
		{
			looped = true;
			index  = 0;
		}
	}
	// Didn't get any match
	return false;
}

int BrowserCanvas::longestItemTextWidth() const
{
	return 144;

	//// Just return it if it's already calculated
	// if (longest_text >= 0)
	//	return longest_text;

	//// Go through all items
	// for (unsigned a = 0; a < items.size(); a++)
	//{
	//	string name = items[a]->getName();
	//	int width = Drawing::textExtents(name, font).x;
	//	if (width > longest_text)
	//		longest_text = width;
	//}

	// return longest_text;
}


// -----------------------------------------------------------------------------
//
// BrowserCanvas Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the canvas is resized
// -----------------------------------------------------------------------------
void BrowserCanvas::onSize(wxSizeEvent& e)
{
	updateLayout();

	// Do default stuff
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the scrollbar 'thumb' is moved
// -----------------------------------------------------------------------------
void BrowserCanvas::onScrollThumbTrack(wxScrollEvent& e)
{
	// Update y-offset and refresh
	yoff_ = scrollbar_->GetThumbPosition();
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when the scrollbar recieves a 'line up' command (ie when the up arrow
// is clicked)
// -----------------------------------------------------------------------------
void BrowserCanvas::onScrollLineUp(wxScrollEvent& e)
{
	// Scroll up by one row
	scrollbar_->SetThumbPosition(yoff_ - fullItemSizeY());

	// Update y-offset and refresh
	yoff_ = scrollbar_->GetThumbPosition();
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when the scrollbar recieves a 'line down' command (ie when the down
// arrow is clicked)
// -----------------------------------------------------------------------------
void BrowserCanvas::onScrollLineDown(wxScrollEvent& e)
{
	// Scroll down by one row
	scrollbar_->SetThumbPosition(yoff_ + fullItemSizeY());

	// Update y-offset and refresh
	yoff_ = scrollbar_->GetThumbPosition();
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when the scrollbar recieves a 'page up' command (ie when the area
// above the 'thumb' is clicked)
// -----------------------------------------------------------------------------
void BrowserCanvas::onScrollPageUp(wxScrollEvent& e)
{
	// Scroll up by one screen
	const wxSize size = GetSize() * GetContentScaleFactor();
	scrollbar_->SetThumbPosition(yoff_ - size.y);

	// Update y-offset and refresh
	yoff_ = scrollbar_->GetThumbPosition();
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when the scrollbar recieves a 'page down' command (ie when the area
// below the 'thumb' is clicked)
// -----------------------------------------------------------------------------
void BrowserCanvas::onScrollPageDown(wxScrollEvent& e)
{
	// Scroll down by one screen
	const wxSize size = GetSize() * GetContentScaleFactor();
	scrollbar_->SetThumbPosition(yoff_ + size.y);

	// Update y-offset and refresh
	yoff_ = scrollbar_->GetThumbPosition();
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when any mouse event is generated (click, scroll, etc)
// -----------------------------------------------------------------------------
void BrowserCanvas::onMouseEvent(wxMouseEvent& e)
{
	// --- Scroll wheel ---
	if (e.GetEventType() == wxEVT_MOUSEWHEEL)
	{
		// Detemine the scroll multiplier
		float scroll_mult = (float)e.GetWheelRotation() / (float)e.GetWheelDelta();

		// Scrolling by 1.0 means by 1 row
		int scroll_amount = (fullItemSizeY()) * -scroll_mult;

		// Do scroll
		scrollbar_->SetThumbPosition(yoff_ + scroll_amount);

		// Update y-offset and refresh
		yoff_ = scrollbar_->GetThumbPosition();
		Refresh();
	}

	// --- Left click ---
	else if (e.GetEventType() == wxEVT_LEFT_DOWN)
	{
		// Clear selection
		item_selected_ = nullptr;

		// Get column clicked & number of columns
		const wxSize size      = GetSize() * GetContentScaleFactor();
		int          col_width = size.x / num_cols_;
		int          col       = e.GetPosition().x * GetContentScaleFactor() / col_width;

		// Get row clicked
		int row = (e.GetPosition().y * GetContentScaleFactor() - top_y_) / (fullItemSizeY());

		// Select item
		selectItem(top_index_ + (row * num_cols_) + col);
		Refresh();
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a key is pressed within the canvas
// -----------------------------------------------------------------------------
void BrowserCanvas::onKeyDown(wxKeyEvent& e)
{
	const wxSize size     = GetSize() * GetContentScaleFactor();
	int          num_cols = size.x / fullItemSizeX();
	int          offset;

	// Down arrow
	if (e.GetKeyCode() == WXK_DOWN)
		offset = num_cols;

	// Up arrow
	else if (e.GetKeyCode() == WXK_UP)
		offset = -1 * num_cols;

	// Left arrow
	else if (e.GetKeyCode() == WXK_LEFT)
		offset = -1;

	// Right arrow
	else if (e.GetKeyCode() == WXK_RIGHT)
		offset = 1;

	// Page up
	else if (e.GetKeyCode() == WXK_PAGEUP)
		offset = -1 * num_cols * max(size.y / fullItemSizeY(), 1);

	// Page down
	else if (e.GetKeyCode() == WXK_PAGEDOWN)
		offset = num_cols * max(size.y / fullItemSizeY(), 1);

	else
	{
		e.Skip();
		return;
	}

	// Clamp selection
	int selected = itemIndex(item_selected_) + offset;
	if (selected < 0)
		selected = 0;
	else if (selected >= (int)items_filter_.size())
		selected = (int)items_filter_.size() - 1;

	selectItem(selected);
	showItem(selected, -1 * offset);

	// Refresh canvas
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when a 'character' key is pressed within the canvas
// -----------------------------------------------------------------------------
int bc_chars[] = {
	'.', ',', '_', '-', '+', '=', '`',  '~', '!', '@', '#', '$', '(',  ')',  '[',
	']', '{', '}', ':', ';', '/', '\\', '<', '>', '?', '^', '&', '\'', '\"',
};
int  n_bc_chars = 30;
void BrowserCanvas::onKeyChar(wxKeyEvent& e)
{
	// Check the key pressed is actually a character (a-z, 0-9 etc)
	bool isRealChar = false;
	if (e.GetKeyCode() >= 'a' && e.GetKeyCode() <= 'z') // Lowercase
		isRealChar = true;
	else if (e.GetKeyCode() >= 'A' && e.GetKeyCode() <= 'Z') // Uppercase
		isRealChar = true;
	else if (e.GetKeyCode() >= '0' && e.GetKeyCode() <= '9') // Number
		isRealChar = true;
	else
	{
		for (int a = 0; a < n_bc_chars; a++)
		{
			if (e.GetKeyCode() == bc_chars[a])
			{
				isRealChar = true;
				break;
			}
		}
	}

	if (isRealChar)
	{
		// Get currently selected item (or first if nothing is focused)
		int selected = itemIndex(item_selected_);
		if (selected < 0)
			selected = 0;

		// Build search string
		search_ += e.GetKeyCode();
		search_.MakeUpper();

		// Search for match from the current focus, and if failed
		// start a new search from after the current focus.
		if (!searchItemFrom(selected))
		{
			search_ = wxString::Format("%c", e.GetKeyCode());
			search_.MakeUpper();
			searchItemFrom(selected + 1);
		}

		// Refresh canvas
		Refresh();
	}
	else
	{
		search_ = "";
		e.Skip();
	}
}

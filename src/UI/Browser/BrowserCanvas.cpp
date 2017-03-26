
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    BrowserCanvas.cpp
 * Description: The OpenGL canvas for displaying browser items. Also
 *              keeps track of a vertical scrollbar to scroll through
 *              the items it contains
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "BrowserCanvas.h"
#include "OpenGL/Drawing.h"



/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, browser_bg_type, false, CVAR_SAVE)
CVAR(Int, browser_item_size, 96, CVAR_SAVE)
DEFINE_EVENT_TYPE(wxEVT_BROWSERCANVAS_SELECTION_CHANGED)


/*******************************************************************
 * BROWSERCANVAS CLASS FUNCTIONS
 *******************************************************************/

/* BrowserCanvas::BrowserCanvas
 * BrowserCanvas class constructor
 *******************************************************************/
BrowserCanvas::BrowserCanvas(wxWindow* parent) : OGLCanvas(parent, -1)
{
	// Init variables
	yoff = 0;
	item_border = 8;
	scrollbar = NULL;
	item_selected = NULL;
	font = Drawing::FONT_BOLD;
	show_names = NAMES_NORMAL;
	item_size = -1;
	item_type = ITEMS_NORMAL;
	longest_text = -1;
	num_cols = -1;

	// Bind events
	Bind(wxEVT_SIZE, &BrowserCanvas::onSize, this);
	Bind(wxEVT_MOUSEWHEEL, &BrowserCanvas::onMouseEvent, this);
	Bind(wxEVT_LEFT_DOWN, &BrowserCanvas::onMouseEvent, this);
	Bind(wxEVT_KEY_DOWN, &BrowserCanvas::onKeyDown, this);
	//Bind(wxEVT_CHAR, &BrowserCanvas::onKeyChar, this);
}

/* BrowserCanvas::~BrowserCanvas
 * BrowserCanvas class destructor
 *******************************************************************/
BrowserCanvas::~BrowserCanvas()
{
}

/* BrowserCanvas::getViewedIndex
 * Return the unfiltered index of the item currently in the middle of the
 * viewport, or -1 if no items are visible
 *******************************************************************/
int BrowserCanvas::getViewedIndex()
{
	if (items_filter.empty())
		return -1;

	int viewport_height = GetSize().y;
	int row_height = fullItemSizeY();
	int viewport_mid_y = yoff + viewport_height / 2.0;
	int viewed_row = viewport_mid_y / row_height;
	int viewed_item_id = (viewed_row + 0.5) * num_cols;
	if (viewed_item_id < 0)
		viewed_item_id = 0;
	else if ((unsigned)viewed_item_id >= items_filter.size())
		viewed_item_id = items_filter.size() - 1;

	return items_filter[viewed_item_id];
}

/* BrowserCanvas::addItem
 * Adds [item] to the list of items
 *******************************************************************/
void BrowserCanvas::addItem(BrowserItem* item)
{
	items.push_back(item);
	longest_text = -1;
}

/* BrowserCanvas::clearItems
 * Clears all items
 *******************************************************************/
void BrowserCanvas::clearItems()
{
	items.clear();
	longest_text = -1;
}

/* BrowserCanvas::fullItemSizeX
 * Returns the 'full' (including border) width of each item
 *******************************************************************/
int BrowserCanvas::fullItemSizeX()
{
	int base_size;
	if (item_size > 0)
		base_size = item_size + (item_border*2);
	else
		base_size = browser_item_size + (item_border*2);

	if (item_type == ITEMS_TILES)
		return base_size + longestItemTextWidth() + item_border*2;
	else
		return base_size;
}

/* BrowserCanvas::fullItemSizeY
 * Returns the 'full' (including border and row gap) height of each
 * item
 *******************************************************************/
int BrowserCanvas::fullItemSizeY()
{
	int gap = 16;
	if (show_names == NAMES_NONE || item_type == ITEMS_TILES)
		gap = 0;

	if (item_size > 0)
		return item_size + (item_border*2) + gap;
	else
		return browser_item_size + (item_border*2) + gap;
}

/* BrowserCanvas::draw
 * Handles drawing of the canvas content
 *******************************************************************/
void BrowserCanvas::draw()
{
	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, GetSize().y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Setup colours
	rgba_t col_bg, col_text;
	bool text_shadow = true;
	if (browser_bg_type == 1)
	{
		// Get system panel background colour
		wxColour bgcolwx = Drawing::getPanelBGColour();
		col_bg.set(COLWX(bgcolwx));

		// Get system text colour
		wxColour textcol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
		col_text.set(COLWX(textcol));

		// Check text colour brightness, if it's dark don't draw text shadow
		rgba_t col_temp = col_text;
		wxColor::MakeGrey(&col_temp.r, &col_temp.g, &col_temp.b);
		if (col_temp.r < 60)
			text_shadow = false;
	}
	else
	{
		// Otherwise use black background
		col_bg.set(0, 0, 0);

		// And white text
		col_text.set(255, 255, 255);
	}

	// Clear
	glClearColor(col_bg.fr(), col_bg.fg(), col_bg.fb(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw background if required
	if (browser_bg_type == 0)
		drawCheckeredBackground();

	// Init for texture drawing
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glLineWidth(2.0f);

	// Draw items
	int x = item_border;
	int y = item_border;
	int col_width = GetSize().x / num_cols;
	int col = 0;
	top_index = -1;
	for (unsigned a = 0; a < items_filter.size(); a++)
	{
		// If we're not yet into the viewable area, skip
		if (y < yoff - fullItemSizeY())
		{
			col++;
			if (col >= num_cols)
			{
				col = 0;
				y += fullItemSizeY();

				// Canvas is filled, stop drawing
				if (y > yoff + GetSize().y)
					break;
			}
			continue;
		}

		// If we're drawing the first non-hidden item, save it
		if (top_index < 0)
		{
			top_index = a;
			top_y = y - yoff;
		}

		// Determine current x position
		int xgap = (col_width - fullItemSizeX()) * 0.5;
		x = item_border + xgap + (col * col_width);

		// Draw selection box if selected
		if (item_selected == items[items_filter[a]])
		{
			// Setup
			glDisable(GL_TEXTURE_2D);
			glColor4f(0.3f, 0.5f, 1.0f, 0.3f);
			glPushMatrix();
			glTranslated(x, y - yoff, 0);
			glTranslated(-item_border, -item_border, 0);

			// Selection background
			glBegin(GL_QUADS);
			glVertex2i(2, 2);
			glVertex2i(2, fullItemSizeY()-3);
			glVertex2i(fullItemSizeX()-3, fullItemSizeY()-3);
			glVertex2i(fullItemSizeX()-3, 2);
			glEnd();

			// Selection border
			glColor4f(0.6f, 0.8f, 1.0f, 1.0f);
			glBegin(GL_LINE_LOOP);
			glVertex2i(2, 2);
			glVertex2i(2, fullItemSizeY()-3);
			glVertex2i(fullItemSizeX()-3, fullItemSizeY()-3);
			glVertex2i(fullItemSizeX()-3, 2);
			glEnd();

			// Finish
			glPopMatrix();
			glEnable(GL_TEXTURE_2D);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}

		// Draw item
		if (item_size <= 0)
			items[items_filter[a]]->draw(browser_item_size, x, y - yoff, font, show_names, item_type, col_text, text_shadow);
		else
			items[items_filter[a]]->draw(item_size, x, y - yoff, font, show_names, item_type, col_text, text_shadow);

		// Move over for next item
		col++;
		if (col >= num_cols)
		{
			col = 0;
			y += fullItemSizeY();

			// Canvas is filled, stop drawing
			if (y > yoff + GetSize().y)
				break;
		}
	}

	// Swap Buffers
	SwapBuffers();
}

/* BrowserCanvas::setScrollBar
 * Sets this canvas' associated vertical scrollbar
 *******************************************************************/
void BrowserCanvas::setScrollBar(wxScrollBar* scrollbar)
{
	// Set scrollbar
	this->scrollbar = scrollbar;

	// Bind events
	scrollbar->Bind(wxEVT_SCROLL_THUMBTRACK, &BrowserCanvas::onScrollThumbTrack, this);
	scrollbar->Bind(wxEVT_SCROLL_LINEUP, &BrowserCanvas::onScrollLineUp, this);
	scrollbar->Bind(wxEVT_SCROLL_LINEDOWN, &BrowserCanvas::onScrollLineDown, this);
	scrollbar->Bind(wxEVT_SCROLL_PAGEUP, &BrowserCanvas::onScrollPageUp, this);
	scrollbar->Bind(wxEVT_SCROLL_PAGEDOWN, &BrowserCanvas::onScrollPageDown, this);
}

/* BrowserCanvas::updateLayout
 * Updates variables concerning the object layout, then updates the
 * associated scrollbar's properties depending on the number of
 * items, the canvas size, etc.
 *******************************************************************/
void BrowserCanvas::updateLayout(int viewed_index)
{
	if (scrollbar && viewed_index < 0)
		viewed_index = getViewedIndex();

	// Determine number of columns
	num_cols = GetSize().x / fullItemSizeX();
	if (num_cols == 0)
		num_cols = 1;

	// Update the scrollbar, if present
	if (scrollbar)
	{
		// Try to keep the view scrolled to roughly the same area: find the
		// item currently in the middle, and keep it there
		// If the given item is no longer visible, find the first filtered item
		// after it that is
		int filtered_viewed_index = -1;
		for (unsigned a = 0; a < items_filter.size(); a++)
			if (items_filter[a] >= viewed_index)
			{
				filtered_viewed_index = a;
				break;
			}
		if (filtered_viewed_index < 0)
			filtered_viewed_index = items_filter.size() - 1;

		// Determine total height of all items
		int rows = (double)items_filter.size() / (double)num_cols + 0.9999;
		int total_height = rows * fullItemSizeY();
		int viewport_height = GetSize().y;

		// Setup scrollbar
		scrollbar->SetScrollbar(scrollbar->GetThumbPosition(), viewport_height, total_height, viewport_height);
		showItem(filtered_viewed_index, 0);
	}

	Refresh();
}

/* BrowserCanvas::getSelectedItem
 * Returns the currently selected BrowserItem, or NULL if nothing is
 * selected
 *******************************************************************/
BrowserItem* BrowserCanvas::getSelectedItem()
{
	return item_selected;
}

/* BrowserCanvas::itemAt
 * Returns the currently BrowserItem at [index], taking the current
 * filter into account
 *******************************************************************/
BrowserItem* BrowserCanvas::itemAt(int index)
{
	// Check index
	if (index < 0 || index >= (int)items_filter.size())
		return NULL;

	return items[items_filter[index]];
}

/* BrowserCanvas::itemIndex
 * Returns the index of [item] taking the current filter into account
 *******************************************************************/
int BrowserCanvas::itemIndex(BrowserItem* item)
{
	// Search for the item in the current filtered list
	for (unsigned a = 0; a < items_filter.size(); a++)
	{
		if ((unsigned)items_filter[a] < items.size() &&
			items[items_filter[a]] == item)
			return a;
	}

	// Not found
	return -1;
}

/* BrowserCanvas::selectItem
 * Selects the item [item]
 *******************************************************************/
void BrowserCanvas::selectItem(BrowserItem* item)
{
	// Check if we're clearing the selection
	if (item == NULL)
		item_selected = NULL;
	else
	{
		// Check the item exists in the current set
		for (unsigned a = 0; a < items.size(); a++)
		{
			if (items[a] == item)
				item_selected = item;
		}
	}

	// Generate event
	wxNotifyEvent e(wxEVT_BROWSERCANVAS_SELECTION_CHANGED, GetId());
	e.SetEventObject(this);
	GetEventHandler()->ProcessEvent(e);
}

/* BrowserCanvas::selectItem
 * Selects the item at [index]
 *******************************************************************/
void BrowserCanvas::selectItem(int index)
{
	// Check index
	if (index < 0 || index >= (int)items_filter.size())
		return;

	item_selected = items[items_filter[index]];

	// Generate event
	wxNotifyEvent e(wxEVT_BROWSERCANVAS_SELECTION_CHANGED, GetId());
	e.SetEventObject(this);
	GetEventHandler()->ProcessEvent(e);
}

/* BrowserCanvas::filterItems
 * Filters the visible items by [filter], by name
 *******************************************************************/
void BrowserCanvas::filterItems(string filter)
{
	// Find the currently-viewed item before we change the item list
	int viewed_index = getViewedIndex();

	// Clear current filter list
	items_filter.clear();

	// If the filter is empty, just add all items to the filter
	if (filter.IsEmpty())
	{
		for (unsigned a = 0; a < items.size(); a++)
			items_filter.push_back(a);
	}
	else
	{
		// Setup filter string
		filter.MakeLower();
		filter += "*";

		// Go through items
		for (unsigned a = 0; a < items.size(); a++)
		{
			// Add to filter list if name matches
			if (items[a]->getName().Lower().Matches(filter))
				items_filter.push_back(a);
		}
	}

	// Update scrollbar and refresh
	updateLayout(viewed_index);
}

/* BrowserCanvas::showItem
 * Scrolls the view to show [item] if it is currently off-screen. If
 * [where] is positive, the item will be shown on the top row; if
 * negative, the item will be shown on the bottom row; if zero, the
 * item will be roughly centered.
 *******************************************************************/
void BrowserCanvas::showItem(int item, int where)
{
	// Check item index
	if (item < 0 || item >= (int)items_filter.size())
		return;

	// Determine y-position of item
	int num_cols = GetSize().x / fullItemSizeX();
	if (num_cols == 0)
		return;
	int y_top = (item / num_cols) * fullItemSizeY();
	int y_bottom = y_top + fullItemSizeY();

	int _yoff = yoff;

	// Check if item is outside current view (but always center an item if
	// asked)
	if (y_top < yoff || y_bottom > yoff + GetSize().y || where == 0)
	{
		if (where > 0)
			// Scroll view to show the item on the top row
			yoff = y_top;
		else if (where < 0)
			// Scroll view to show the item on the bottom row
			yoff = y_bottom - GetSize().y;
		else
		{
			// Scroll view to put the item's middle in the middle of the canvas
			yoff = y_top + (fullItemSizeY() - GetSize().y) / 2;
			if (yoff < 0)
				yoff = 0;
		}

		if (scrollbar) scrollbar->SetThumbPosition(yoff);
	}
}

/* BrowserCanvas::showItem
 * Scrolls the view to show the currently selected item
 *******************************************************************/
void BrowserCanvas::showSelectedItem()
{
	showItem(itemIndex(item_selected), 1);
}

/* BrowserCanvas::lookForSearchEntryFrom
 * Used by BrowserCanvas::onKeyChar, returns true if an item matching
 * [search] is found (starting from [from]), false otherwise
 *******************************************************************/
bool BrowserCanvas::searchItemFrom(int from)
{
	int index = from;
	bool looped = false;
	bool gotmatch = false;
	while ((!looped && index < (int)items_filter.size()) || (looped && index < from))
	{
		string name = items[items_filter[index]]->getName();
		if (name.Upper().StartsWith(search))
		{
			// Matches, update selection
			selectItem(index);
			showSelectedItem();
			return true;
		}

		// No match, next item; look in the above entries
		// if no matches were found below.
		if (++index == items_filter.size() && !looped)
		{
			looped = true;
			index = 0;
		}
	}
	// Didn't get any match
	return false;
}

int BrowserCanvas::longestItemTextWidth()
{
	return 144;

	//// Just return it if it's already calculated
	//if (longest_text >= 0)
	//	return longest_text;

	//// Go through all items
	//for (unsigned a = 0; a < items.size(); a++)
	//{
	//	string name = items[a]->getName();
	//	int width = Drawing::textExtents(name, font).x;
	//	if (width > longest_text)
	//		longest_text = width;
	//}

	//return longest_text;
}


/*******************************************************************
 * BROWSERCANVAS CLASS EVENTS
 *******************************************************************/

/* BrowserCanvas::onSize
 * Called when the canvas is resized
 *******************************************************************/
void BrowserCanvas::onSize(wxSizeEvent& e)
{
	updateLayout();

	// Do default stuff
	e.Skip();
}

/* BrowserCanvas::onScrollThumbTrack
 * Called when the scrollbar 'thumb' is moved
 *******************************************************************/
void BrowserCanvas::onScrollThumbTrack(wxScrollEvent& e)
{
	// Update y-offset and refresh
	yoff = scrollbar->GetThumbPosition();
	Refresh();
}

/* BrowserCanvas::onScrollLineUp
 * Called when the scrollbar recieves a 'line up' command (ie when
 * the up arrow is clicked)
 *******************************************************************/
void BrowserCanvas::onScrollLineUp(wxScrollEvent& e)
{
	// Scroll up by one row
	scrollbar->SetThumbPosition(yoff - fullItemSizeY());

	// Update y-offset and refresh
	yoff = scrollbar->GetThumbPosition();
	Refresh();
}

/* BrowserCanvas::onScrollLineDown
 * Called when the scrollbar recieves a 'line down' command (ie when
 * the down arrow is clicked)
 *******************************************************************/
void BrowserCanvas::onScrollLineDown(wxScrollEvent& e)
{
	// Scroll down by one row
	scrollbar->SetThumbPosition(yoff + fullItemSizeY());

	// Update y-offset and refresh
	yoff = scrollbar->GetThumbPosition();
	Refresh();
}

/* BrowserCanvas::onScrollPageUp
 * Called when the scrollbar recieves a 'page up' command (ie when
 * the area above the 'thumb' is clicked)
 *******************************************************************/
void BrowserCanvas::onScrollPageUp(wxScrollEvent& e)
{
	// Scroll up by one screen
	scrollbar->SetThumbPosition(yoff - GetSize().y);

	// Update y-offset and refresh
	yoff = scrollbar->GetThumbPosition();
	Refresh();
}

/* BrowserCanvas::onScrollPageDown
 * Called when the scrollbar recieves a 'page down' command (ie when
 * the area below the 'thumb' is clicked)
 *******************************************************************/
void BrowserCanvas::onScrollPageDown(wxScrollEvent& e)
{
	// Scroll down by one screen
	scrollbar->SetThumbPosition(yoff + GetSize().y);

	// Update y-offset and refresh
	yoff = scrollbar->GetThumbPosition();
	Refresh();
}

/* BrowserCanvas::onMouseEvent
 * Called when any mouse event is generated (click, scroll, etc)
 *******************************************************************/
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
		scrollbar->SetThumbPosition(yoff + scroll_amount);

		// Update y-offset and refresh
		yoff = scrollbar->GetThumbPosition();
		Refresh();
	}

	// --- Left click ---
	else if (e.GetEventType() == wxEVT_LEFT_DOWN)
	{
		// Clear selection
		item_selected = NULL;

		// Get column clicked & number of columns
		int col_width = GetSize().x / num_cols;
		int col = e.GetPosition().x / col_width;

		// Get row clicked
		int row = (e.GetPosition().y - top_y) / (fullItemSizeY());

		// Select item
		selectItem(top_index + (row * num_cols) + col);
		Refresh();
	}

	e.Skip();
}

/* BrowserCanvas::onMouseEvent
 * Called when a key is pressed within the canvas
 *******************************************************************/
void BrowserCanvas::onKeyDown(wxKeyEvent& e)
{
	int num_cols = GetSize().x / fullItemSizeX();
	int offset;

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
		offset = -1 * num_cols * max(GetSize().y / fullItemSizeY(), 1);

	// Page down
	else if (e.GetKeyCode() == WXK_PAGEDOWN)
		offset = num_cols * max(GetSize().y / fullItemSizeY(), 1);

	else
	{
		e.Skip();
		return;
	}

	// Clamp selection
	int selected = itemIndex(item_selected) + offset;
	if (selected < 0)
		selected = 0;
	else if (selected >= (int)items_filter.size())
		selected = (int)items_filter.size() - 1;

	selectItem(selected);
	showItem(selected, -1 * offset);

	// Refresh canvas
	Refresh();
}

/* BrowserCanvas::onKeyChar
 * Called when a 'character' key is pressed within the canvas
 *******************************************************************/
int bc_chars[] =
{
	'.', ',', '_', '-', '+', '=', '`', '~',
	'!', '@', '#', '$', '(', ')', '[', ']',
	'{', '}', ':', ';', '/', '\\', '<', '>',
	'?', '^', '&', '\'', '\"',
};
int n_bc_chars = 30;
void BrowserCanvas::onKeyChar(wxKeyEvent& e)
{
	// Check the key pressed is actually a character (a-z, 0-9 etc)
	bool isRealChar = false;
	if (e.GetKeyCode() >= 'a' && e.GetKeyCode() <= 'z')			// Lowercase
		isRealChar = true;
	else if (e.GetKeyCode() >= 'A' && e.GetKeyCode() <= 'Z')	// Uppercase
		isRealChar = true;
	else if (e.GetKeyCode() >= '0' && e.GetKeyCode() <= '9')	// Number
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
		int selected = itemIndex(item_selected);
		if (selected < 0) selected = 0;

		// Build search string
		search += e.GetKeyCode();
		search.MakeUpper();

		// Search for match from the current focus, and if failed
		// start a new search from after the current focus.
		if (!searchItemFrom(selected))
		{
			search = S_FMT("%c", e.GetKeyCode());
			search.MakeUpper();
			searchItemFrom(selected+1);
		}

		// Refresh canvas
		Refresh();
	}
	else
	{
		search = "";
		e.Skip();
	}
}

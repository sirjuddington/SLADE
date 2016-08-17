
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    BrowserWindow.cpp
 * Description: The browser window implementation. A frame that
 *              contains a tree of item categories/subcategories,
 *              and an OGLCanvas where the browser items under
 *              the currently selected category are displayed
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
#include "BrowserWindow.h"
#include "General/Misc.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, browser_maximised, false, CVAR_SAVE)


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, browser_item_size)


/*******************************************************************
 * BROWSERTREENODE CLASS FUNCTIONS
 *******************************************************************/

/* BrowserTreeNode::BrowserTreeNode
 * BrowserTreeNode class constructor
 *******************************************************************/
BrowserTreeNode::BrowserTreeNode(BrowserTreeNode* parent) : STreeNode(parent)
{
}

/* BrowserTreeNode::~BrowserTreeNode
 * BrowserTreeNode class destructor
 *******************************************************************/
BrowserTreeNode::~BrowserTreeNode()
{
	clearItems();
}

/* BrowserTreeNode::clearItems
 * Clears all items in the node
 *******************************************************************/
void BrowserTreeNode::clearItems()
{
	for (unsigned a = 0; a < items.size(); a++)
		delete items[a];
	items.clear();
}

/* BrowserTreeNode::getItem
 * Gets the item at [index], or NULL if [index] is out of bounds
 *******************************************************************/
BrowserItem* BrowserTreeNode::getItem(unsigned index)
{
	// Check index
	if (index >= items.size())
		return NULL;

	// Return the item
	return items[index];
}

/* BrowserTreeNode::addItem
 * Adds [item] to the node at [index], or at the end if [index] is
 * out of bounds
 *******************************************************************/
void BrowserTreeNode::addItem(BrowserItem* item, unsigned index)
{
	// Check where to add item
	if (index >= items.size())
		items.push_back(item);
	else
		items.insert(items.begin() + index, item);
}


/*******************************************************************
 * BROWSERTREEITEMDATA CLASS
 *******************************************************************
 * wxTreeItemData class needed to associate BrowserTreeNodes with tree items
 */
class BrowserTreeItemData : public wxClientData
{
private:
	BrowserTreeNode*	node;

public:
	BrowserTreeItemData(BrowserTreeNode* node = NULL) { this->node = node; }
	~BrowserTreeItemData() {}

	BrowserTreeNode*	getNode() { return node; }
};


/*******************************************************************
 * BROWSERWINDOW CLASS FUNCTIONS
 *******************************************************************/

/* BrowserWindow::BrowserWindow
 * BrowserWindow class constructor
 *******************************************************************/
BrowserWindow::BrowserWindow(wxWindow* parent)
	: wxDialog(parent, -1, "Browser", wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxMAXIMIZE_BOX)
{
	// Init size/pos
	Misc::winf_t info = Misc::getWindowInfo("browser");
	if (!info.id.IsEmpty())
	{
		SetClientSize(info.width, info.height);
		SetPosition(wxPoint(info.left, info.top));
	}
	else
		Misc::setWindowInfo("browser", 768, 600, 0, 0);

	// Init variables
	items_root = new BrowserTreeNode();
	items_root->setName("All");
	truncate_names = false;

	// Setup layout
	wxBoxSizer* m_vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_vbox);

	//wxSplitterWindow* swin = new wxSplitterWindow(this, -1);
	wxBoxSizer* m_hbox = new wxBoxSizer(wxHORIZONTAL);
	m_vbox->Add(m_hbox, 1, wxEXPAND|wxALL, 4);

	// Browser tree
	tree_items = new wxTreeListCtrl(this, -1, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE|wxDV_ROW_LINES);
	m_hbox->Add(tree_items, 0, wxEXPAND|wxALL, 4);

	// Browser area
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	m_hbox->Add(vbox, 1, wxEXPAND|wxALL, 4);

	// Zoom
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND|wxBOTTOM, 4);
	slider_zoom = new wxSlider(this, -1, browser_item_size, 64, 256);
	slider_zoom->SetLineSize(16);
	slider_zoom->SetPageSize(32);
	hbox->Add(new wxStaticText(this, -1, "Zoom:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	hbox->Add(slider_zoom, 1, wxEXPAND);

	// Sorting
	choice_sort = new wxChoice(this, -1);
	hbox->AddStretchSpacer();
	hbox->Add(new wxStaticText(this, -1, "Sort:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	hbox->Add(choice_sort, 0, wxEXPAND|wxRIGHT, 4);

	// Filter
	text_filter = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	hbox->Add(new wxStaticText(this, -1, "Filter:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	hbox->Add(text_filter, 0, wxEXPAND|wxRIGHT, 4);

	// Browser canvas
	hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 1, wxEXPAND|wxBOTTOM, 4);
	canvas = new BrowserCanvas(this);
	hbox->Add(canvas, 1, wxEXPAND);

	// Canvas scrollbar
	wxScrollBar* scrollbar = new wxScrollBar(this, -1, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
	hbox->Add(scrollbar, 0, wxEXPAND);
	canvas->setScrollBar(scrollbar);

	// Bottom sizer
	sizer_bottom = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(sizer_bottom, 0, wxEXPAND|wxBOTTOM, 4);

	// Buttons and info label
	label_info = new wxStaticText(this, -1, "Info goes here");
	wxSizer* buttonsizer = CreateButtonSizer(wxOK|wxCANCEL);
	buttonsizer->Insert(0, label_info, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 4);

	m_vbox->Add(buttonsizer, 0, wxEXPAND|wxBOTTOM, 4);

	// Setup sorting options
	addSortType("Index");
	addSortType("Name (Alphabetical)");
	choice_sort->SetSelection(0);

	// Bind events
	tree_items->Bind(wxEVT_TREELIST_SELECTION_CHANGED, &BrowserWindow::onTreeItemSelected, this);
	choice_sort->Bind(wxEVT_CHOICE, &BrowserWindow::onChoiceSortChanged, this);
	canvas->Bind(wxEVT_LEFT_DCLICK, &BrowserWindow::onCanvasDClick, this);
	text_filter->Bind(wxEVT_TEXT, &BrowserWindow::onTextFilterChanged, this);
	slider_zoom->Bind(wxEVT_SLIDER, &BrowserWindow::onZoomChanged, this);
	Bind(wxEVT_BROWSERCANVAS_SELECTION_CHANGED, &BrowserWindow::onCanvasSelectionChanged, this, canvas->GetId());
	canvas->Bind(wxEVT_CHAR, &BrowserWindow::onCanvasKeyChar, this);

	Layout();
	SetMinSize(wxSize(540, 400));

	if (browser_maximised)
		Maximize();
	else
		CenterOnParent();

	// Set focus to canvas
	canvas->SetFocus();
}

/* BrowserWindow::~BrowserWindow
 * BrowserWindow class destructor
 *******************************************************************/
BrowserWindow::~BrowserWindow()
{
	browser_maximised = IsMaximized();
	if (!IsMaximized())
		Misc::setWindowInfo("browser", GetClientSize().x, GetClientSize().y, GetPosition().x, GetPosition().y);
}

/* BrowserWindow::addItem
 * Adds [item] to the browser tree at the tree path [where]. This
 * will be created if it doesn't exist
 *******************************************************************/
bool BrowserWindow::addItem(BrowserItem* item, string where)
{
	item->parent = this;
	BrowserTreeNode* target = (BrowserTreeNode*)items_root->addChild(where);
	if (target)
	{
		target->addItem(item);
		return true;
	}
	else
		return false;
}

/* BrowserWindow::addGlobalItem
 * Adds [item] to the global items list. Global items will show up
 * no matter what category is currently selected
 *******************************************************************/
void BrowserWindow::addGlobalItem(BrowserItem* item)
{
	item->parent = this;
	items_global.push_back(item);
}

/* BrowserWindow::clearItems
 * Removes all items from [node] and its children recursively
 *******************************************************************/
void BrowserWindow::clearItems(BrowserTreeNode* node)
{
	// Check node was given to begin clear
	if (!node)
		node = items_root;

	// Clear all items from node
	node->clearItems();

	// Clear all child nodes
	while (node->nChildren() > 0)
	{
		BrowserTreeNode* child = (BrowserTreeNode*)node->getChild(0);
		clearItems(child);
		node->removeChild(child);
		delete child;
	}
}

/* BrowserWindow::reloadItems
 * Reloads (clears) all item images in [node] and its children
 * recursively
 *******************************************************************/
void BrowserWindow::reloadItems(BrowserTreeNode* node)
{
	// Check node was given to begin reload
	if (!node)
		node = items_root;

	// Go through items in this node
	for (unsigned a = 0; a < node->nItems(); a++)
		node->getItem(a)->clearImage();

	// Go through child nodes
	for (unsigned a = 0; a < node->nChildren(); a++)
		reloadItems((BrowserTreeNode*)node->getChild(a));
}

BrowserItem* BrowserWindow::getSelectedItem()
{
	return canvas->getSelectedItem();
}

/* BrowserWindow::selectItem
 * Finds the item matching [name] in the tree, starting from [node].
 * If the item is found, its parent node is opened in the browser
 * and the item is selected
 *******************************************************************/
bool BrowserWindow::selectItem(string name, BrowserTreeNode* node)
{
	// Check node was given, if not start from root
	if (!node)
		node = items_root;

	// Check global items
	for (unsigned a = 0; a < items_global.size(); a++)
	{
		if (S_CMPNOCASE(name, items_global[a]->getName()))
		{
			openTree(node);
			canvas->selectItem(items_global[a]);
			canvas->showSelectedItem();
			return true;
		}
	}

	// Go through all items in this node
	for (unsigned a = 0;  a < node->nItems(); a++)
	{
		// Check for name match (not case-sensitive)
		if (S_CMPNOCASE(node->getItem(a)->getName(), name))
		{
			// Open this node in the browser and select the item
			openTree(node);
			canvas->selectItem(node->getItem(a));
			canvas->showSelectedItem();
			tree_items->Select(node->getTreeId());
			tree_items->Expand(node->getTreeId());
			return true;
		}
	}

	// Item not found in this one, try its child nodes
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		if (selectItem(name, (BrowserTreeNode*)node->getChild(a)))
			return true;
	}

	// Item not found
	return false;
}

// Sorting functions
bool sortBIIndex(BrowserItem* left, BrowserItem* right)
{
	return left->getIndex() < right->getIndex();
}
bool sortBIName(BrowserItem* left, BrowserItem* right)
{
	return left->getName() < right->getName();
}

/* BrowserWindow::addSortType
 * Adds a sorting type [name] to the window
 *******************************************************************/
unsigned BrowserWindow::addSortType(string name)
{
	choice_sort->AppendString(name);
	return choice_sort->GetCount() - 1;
}

/* BrowserWindow::doSort
 * Performs sorting of the items currently being browsed, the sort
 * criteria depending on [sort_type]. Default sorting types are by
 * index (0) and by name (1), more can be added to BrowserWindow
 * child classes if needed
 *******************************************************************/
void BrowserWindow::doSort(unsigned sort_type)
{
	// Get item list
	vector<BrowserItem*>& items = canvas->itemList();

	// Do sorting

	// 0: By Index
	if (sort_type == 0)
		std::sort(items.begin(), items.end(), sortBIIndex);

	// 1: By Name (Alphabetical)
	else if (sort_type == 1)
		std::sort(items.begin(), items.end(), sortBIName);

	// Refresh canvas
	canvas->showSelectedItem();
	canvas->Refresh();
}

/* BrowserWindow::setSortType
 * Sets the current sorting method to [type]
 *******************************************************************/
void BrowserWindow::setSortType(int type)
{
	// Check type index
	if (type < 0 || (unsigned)type >= choice_sort->GetCount())
		return;

	// Select sorting type
	choice_sort->SetSelection(type);

	// Do sorting
	doSort(type);
}

/* BrowserWindow::openTree
 * 'Opens' the items in [node] and all its children, adding them
 * to the browser canvas' list of items. If [clear] is true, the
 * current list contents will be cleared
 *******************************************************************/
void BrowserWindow::openTree(BrowserTreeNode* node, bool clear)
{
	// Clear if needed
	if (clear)
	{
		canvas->clearItems();

		// Also add global items
		for (unsigned a = 0; a < items_global.size(); a++)
			canvas->addItem(items_global[a]);
	}

	// Add all items in the node
	for (unsigned a = 0; a < node->nItems(); a++)
	{
		canvas->addItem(node->getItem(a));
		node->getItem(a)->parent = this;
	}

	// Add all child nodes' items
	for (unsigned a = 0; a < node->nChildren(); a++)
		openTree((BrowserTreeNode*)node->getChild(a), false);

	// If the list was cleared, sort it, filter it and update the canvas scrollbar
	if (clear)
	{
		doSort(choice_sort->GetSelection());
		canvas->filterItems(text_filter->GetValue());
		canvas->showSelectedItem();
	}
}

int expandtree(wxTreeListCtrl* tree, wxTreeListItem& item, bool expand, int depth)
{
	if (!item.IsOk())
		return depth;

	if (expand)
		tree->Expand(item);

	// Expand first sibling
	wxTreeListItem sibling = tree->GetNextSibling(item);
	expandtree(tree, sibling, expand, depth);

	// Expand first child
	wxTreeListItem child = tree->GetFirstChild(item);
	depth = expandtree(tree, child, expand, depth + 1);

	if (!expand)
		tree->Collapse(item);

	return depth;
}

/* BrowserWindow::populateItemTree
 * Populates the wxTreeCtrl with the contents of the browser item
 * category tree
 *******************************************************************/
void BrowserWindow::populateItemTree(bool collapse_all)
{
	// Clear current tree
	tree_items->DeleteAllItems();
	tree_items->ClearColumns();

	// Add root item
	tree_items->AppendColumn("Categories", wxCOL_WIDTH_AUTOSIZE);
	wxTreeListItem item = tree_items->AppendItem(tree_items->GetRootItem(), "All");
	tree_items->SetItemData(item, new BrowserTreeItemData(items_root));

	// Add tree
	addItemTree(items_root, item);

	// Update window layout
	int depth = expandtree(tree_items, item, true, 0);
	int colwidth = tree_items->GetColumnWidth(0);
#ifndef __WXMSW__
	if (colwidth < 140) colwidth = 200;
#endif
	tree_items->SetMinSize(wxSize(colwidth + 16, -1));
	Layout();
	if (collapse_all)
		expandtree(tree_items, item, false, 0);
}

/* BrowserWindow::addItemTree
 * Adds [node] to the wxTreeCtrl after [item]
 *******************************************************************/
void BrowserWindow::addItemTree(BrowserTreeNode* node, wxTreeListItem& item)
{
	// Go through child items
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		// Add tree item
		BrowserTreeNode* child = (BrowserTreeNode*)node->getChild(a);
		wxTreeListItem i = tree_items->AppendItem(item, child->getName(), -1, -1, new BrowserTreeItemData(child));
		child->setTreeId(i);

		// Add children
		addItemTree(child, i);
	}
}

/* BrowserWindow::setFont
 * Sets the font to be used for item names
 *******************************************************************/
void BrowserWindow::setFont(int font)
{
	canvas->setFont(font);
}

/* BrowserWindow::showSelectedNameOnly
 * Sets the type of item names to show (in normal view mode)
 *******************************************************************/
void BrowserWindow::setItemNameType(int type)
{
	canvas->setItemNameType(type);
}

/* BrowserWindow::setItemSize
 * Sets the item size (0 or less to use zoom slider)
 *******************************************************************/
void BrowserWindow::setItemSize(int size)
{
	canvas->setItemSize(size);

	if (size <= 0)
		slider_zoom->Enable(true);
	else
		slider_zoom->Enable(false);

	Layout();
	Refresh();
}

/* BrowserWindow::setItemViewType
 * Sets the item view type
 *******************************************************************/
void BrowserWindow::setItemViewType(int type)
{
	canvas->setItemViewType(type);
}


/*******************************************************************
 * BROWSERWINDOW CLASS EVENTS
 *******************************************************************/

/* BrowserWindow::onTreeItemSelected
 * Called when an item on the category wxTreeCtrl is selected
 *******************************************************************/
void BrowserWindow::onTreeItemSelected(wxTreeListEvent& e)
{
	BrowserTreeItemData* data = (BrowserTreeItemData*)tree_items->GetItemData(e.GetItem());
	if (data)
		openTree(data->getNode());
	canvas->Refresh();
}

/* BrowserWindow::onChoiceSortChanged
 * Called when the 'Sort By' dropdown selection is changed
 *******************************************************************/
void BrowserWindow::onChoiceSortChanged(wxCommandEvent& e)
{
	// Re-sort items
	doSort(choice_sort->GetSelection());
}

/* BrowserWindow::onCanvasDClick
 * Called when the browser canvas is double-clicked
 *******************************************************************/
void BrowserWindow::onCanvasDClick(wxMouseEvent& e)
{
	// End modal if an item was double-clicked
	EndModal(wxID_OK);
}

/* BrowserWindow::onTextFilterChanged
 * Called when the name filter is changed
 *******************************************************************/
void BrowserWindow::onTextFilterChanged(wxCommandEvent& e)
{
	// Filter canvas items
	canvas->filterItems(text_filter->GetValue());
}

/* BrowserWindow::sliderZoomChanged
 * Called when the zoom slider is changed
 *******************************************************************/
void BrowserWindow::onZoomChanged(wxCommandEvent& e)
{
	// Get zoom value
	int item_size = slider_zoom->GetValue();

	// Lock to increments of 16
	int remainder = item_size % 16;
	item_size -= remainder;

	// Set slider value to locked increment
	slider_zoom->SetValue(item_size);

	// Update zoom label
	//label_current_zoom->SetLabel(S_FMT("%d%%", zoom_percent));

	// Update item size and refresh
	if (item_size != browser_item_size)
	{
		int viewed_index = canvas->getViewedIndex();
		browser_item_size = item_size;
		canvas->updateLayout(viewed_index);
	}
}

/* BrowserWindow::onCanvasSelectionChanged
 * Called when the selection changes on the browser canvas
 *******************************************************************/
void BrowserWindow::onCanvasSelectionChanged(wxEvent& e)
{
	// Get selected item
	BrowserItem* item = canvas->getSelectedItem();
	if (!item)
	{
		// Clear info if nothing selected
		label_info->SetLabel("");
		Refresh();
		return;
	}

	// Build info string
	string info = item->getName();
	string info_extra = item->itemInfo();
	if (!info_extra.IsEmpty())
	{
		info += ": ";
		info += info_extra;
	}

	// Set info label
	label_info->SetLabel(info);
	Refresh();
	return;
}

/* BrowserWindow::onCanvasKeyChar
 * Called when a key is pressed in the browser canvas
 *******************************************************************/
int bw_chars[] =
{
	'.', ',', '_', '-', '+', '=', '`', '~',
	'!', '@', '#', '$', '(', ')', '[', ']',
	'{', '}', ':', ';', '/', '\\', '<', '>',
	'?', '^', '&', '*', '\'', '\"',
};
int n_bw_chars = 31;
void BrowserWindow::onCanvasKeyChar(wxKeyEvent& e)
{
	// Backspace
	if (e.GetKeyCode() == WXK_BACK && text_filter->GetValue().size() > 0)
	{
		string filter = text_filter->GetValue();
		filter.RemoveLast(1);
		text_filter->SetValue(filter);
		e.Skip();
		return;
	}

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
		for (int a = 0; a < n_bw_chars; a++)
		{
			if (e.GetKeyCode() == bw_chars[a])
			{
				isRealChar = true;
				break;
			}
		}
	}

	if (isRealChar)
	{
		string filter = text_filter->GetValue();
		filter += e.GetKeyCode();
		filter.MakeUpper();
		text_filter->SetValue(filter);
	}
}

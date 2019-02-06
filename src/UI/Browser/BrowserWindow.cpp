
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    BrowserWindow.cpp
// Description: The browser window implementation. A frame that contains a tree
//              of item categories/subcategories, and an OGLCanvas where the
//              browser items under the currently selected category are
//              displayed
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
#include "BrowserWindow.h"
#include "General/Misc.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, browser_maximised, false, CVar::Flag::Save)
namespace
{
int bw_chars[] = {
	'.', ',', '_', '-', '+', '=', '`',  '~', '!', '@', '#', '$', '(', ')',  '[',
	']', '{', '}', ':', ';', '/', '\\', '<', '>', '?', '^', '&', '*', '\'', '\"',
};
int n_bw_chars = 31;
} // namespace


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, browser_item_size)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
int expandTree(wxTreeListCtrl* tree, wxTreeListItem& item, bool expand, int depth)
{
	if (!item.IsOk())
		return depth;

	if (expand)
		tree->Expand(item);

	// Expand first sibling
	auto sibling = tree->GetNextSibling(item);
	expandTree(tree, sibling, expand, depth);

	// Expand first child
	auto child = tree->GetFirstChild(item);
	depth      = expandTree(tree, child, expand, depth + 1);

	if (!expand)
		tree->Collapse(item);

	return depth;
}
} // namespace


// -----------------------------------------------------------------------------
//
// BrowserTreeNode Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Clears all items in the node
// -----------------------------------------------------------------------------
void BrowserTreeNode::clearItems()
{
	items_.clear();
}

// -----------------------------------------------------------------------------
// Gets the item at [index], or NULL if [index] is out of bounds
// -----------------------------------------------------------------------------
BrowserItem* BrowserTreeNode::item(unsigned index)
{
	// Check index
	if (index >= items_.size())
		return nullptr;

	// Return the item
	return items_[index].get();
}

// -----------------------------------------------------------------------------
// Adds [item] to the node at [index], or at the end if [index] is out of
// bounds
// -----------------------------------------------------------------------------
void BrowserTreeNode::addItem(BrowserItem* item, unsigned index)
{
	// Check where to add item
	if (index >= items_.size())
		items_.emplace_back(item);
	else
		items_.emplace(items_.begin() + index, item);
}


// -----------------------------------------------------------------------------
// BrowserTreeItemData Class
//
// wxTreeItemData class needed to associate BrowserTreeNodes with tree items
// -----------------------------------------------------------------------------
class BrowserTreeItemData : public wxClientData
{
public:
	BrowserTreeItemData(BrowserTreeNode* node = nullptr) : node_{ node } {}
	~BrowserTreeItemData() = default;

	BrowserTreeNode* node() const { return node_; }

private:
	BrowserTreeNode* node_;
};


// -----------------------------------------------------------------------------
//
// BrowserWindow Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// BrowserWindow class constructor
// -----------------------------------------------------------------------------
BrowserWindow::BrowserWindow(wxWindow* parent, bool truncate_names) :
	wxDialog{ parent,        -1,
			  "Browser",     wxDefaultPosition,
			  wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX },
	truncate_names_{ truncate_names }
{
	// Init size/pos
	auto info = Misc::getWindowInfo("browser");
	if (!info.id.IsEmpty())
	{
		SetClientSize(info.width, info.height);
		SetPosition(wxPoint(info.left, info.top));
	}
	else
		Misc::setWindowInfo("browser", 768, 600, 0, 0);

	// Init variables
	items_root_ = new BrowserTreeNode();
	items_root_->setName("All");

	// Setup layout
	auto m_vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_vbox);

	auto m_hbox = new wxBoxSizer(wxHORIZONTAL);
	m_vbox->Add(m_hbox, 1, wxEXPAND | wxALL, UI::padLarge());

	// Browser tree
	tree_items_ = new wxTreeListCtrl(this, -1, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE | wxDV_ROW_LINES);
	m_hbox->Add(tree_items_, 0, wxEXPAND | wxRIGHT, UI::pad());

	// Browser area
	auto vbox = new wxBoxSizer(wxVERTICAL);
	m_hbox->Add(vbox, 1, wxEXPAND);

	// Zoom
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::pad());
	slider_zoom_ = new wxSlider(this, -1, browser_item_size, 64, 256);
	slider_zoom_->SetLineSize(16);
	slider_zoom_->SetPageSize(32);
	hbox->Add(new wxStaticText(this, -1, "Zoom:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(slider_zoom_, 1, wxEXPAND);

	// Sorting
	choice_sort_ = new wxChoice(this, -1);
	hbox->AddStretchSpacer();
	hbox->Add(new wxStaticText(this, -1, "Sort:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(choice_sort_, 0, wxEXPAND | wxRIGHT, UI::pad());

	// Filter
	text_filter_ = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	hbox->Add(new wxStaticText(this, -1, "Filter:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(text_filter_, 0, wxEXPAND);

	// Browser canvas
	hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 1, wxEXPAND);
	canvas_ = new BrowserCanvas(this);
	hbox->Add(canvas_, 1, wxEXPAND);

	// Canvas scrollbar
	auto scrollbar = new wxScrollBar(this, -1, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
	hbox->Add(scrollbar, 0, wxEXPAND);
	canvas_->setScrollBar(scrollbar);

	// Bottom sizer
	sizer_bottom_ = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(sizer_bottom_, 0, wxEXPAND | wxBOTTOM, UI::pad());

	// Buttons and info label
	label_info_      = new wxStaticText(this, -1, "");
	auto buttonsizer = CreateButtonSizer(wxOK | wxCANCEL);
	buttonsizer->Insert(0, label_info_, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, UI::pad());

	m_vbox->Add(buttonsizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::padLarge());

	// Setup sorting options
	addSortType("Index");
	addSortType("Name (Alphabetical)");
	choice_sort_->SetSelection(0);

	// Bind events
	tree_items_->Bind(wxEVT_TREELIST_SELECTION_CHANGED, &BrowserWindow::onTreeItemSelected, this);
	choice_sort_->Bind(wxEVT_CHOICE, &BrowserWindow::onChoiceSortChanged, this);
	canvas_->Bind(wxEVT_LEFT_DCLICK, &BrowserWindow::onCanvasDClick, this);
	text_filter_->Bind(wxEVT_TEXT, &BrowserWindow::onTextFilterChanged, this);
	slider_zoom_->Bind(wxEVT_SLIDER, &BrowserWindow::onZoomChanged, this);
	Bind(wxEVT_BROWSERCANVAS_SELECTION_CHANGED, &BrowserWindow::onCanvasSelectionChanged, this, canvas_->GetId());
	canvas_->Bind(wxEVT_CHAR, &BrowserWindow::onCanvasKeyChar, this);

	wxWindowBase::Layout();
	wxTopLevelWindowBase::SetMinSize(WxUtils::scaledSize(540, 400));

	if (browser_maximised)
		wxTopLevelWindow::Maximize();
	else
		CenterOnParent();

	// Set focus to canvas
	canvas_->SetFocus();
}

// -----------------------------------------------------------------------------
// BrowserWindow class destructor
// -----------------------------------------------------------------------------
BrowserWindow::~BrowserWindow()
{
	browser_maximised = wxTopLevelWindow::IsMaximized();
	if (!wxTopLevelWindow::IsMaximized())
		Misc::setWindowInfo("browser", GetClientSize().x, GetClientSize().y, GetPosition().x, GetPosition().y);
}

// -----------------------------------------------------------------------------
// Adds [item] to the browser tree at the tree path [where].
// This will be created if it doesn't exist
// -----------------------------------------------------------------------------
bool BrowserWindow::addItem(BrowserItem* item, const wxString& where)
{
	item->parent_ = this;
	auto target   = dynamic_cast<BrowserTreeNode*>(items_root_->addChild(where));
	if (target)
	{
		target->addItem(item);
		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Adds [item] to the global items list. Global items will show up no matter
// what category is currently selected
// -----------------------------------------------------------------------------
void BrowserWindow::addGlobalItem(BrowserItem* item)
{
	item->parent_ = this;
	items_global_.push_back(item);
}

// -----------------------------------------------------------------------------
// Removes all items from [node] and its children recursively
// -----------------------------------------------------------------------------
void BrowserWindow::clearItems(BrowserTreeNode* node) const
{
	// Check node was given to begin clear
	if (!node)
		node = items_root_;

	// Clear all items from node
	node->clearItems();

	// Clear all child nodes
	while (node->nChildren() > 0)
	{
		auto child = dynamic_cast<BrowserTreeNode*>(node->child(0));
		clearItems(child);
		node->removeChild(child);
		delete child;
	}
}

// -----------------------------------------------------------------------------
// Reloads (clears) all item images in [node] and its children recursively
// -----------------------------------------------------------------------------
void BrowserWindow::reloadItems(BrowserTreeNode* node) const
{
	// Check node was given to begin reload
	if (!node)
		node = items_root_;

	// Go through items in this node
	for (unsigned a = 0; a < node->nItems(); a++)
		node->item(a)->clearImage();

	// Go through child nodes
	for (unsigned a = 0; a < node->nChildren(); a++)
		reloadItems(dynamic_cast<BrowserTreeNode*>(node->child(a)));
}

// -----------------------------------------------------------------------------
// Returns the currently selected item
// -----------------------------------------------------------------------------
BrowserItem* BrowserWindow::selectedItem() const
{
	return canvas_->selectedItem();
}

// -----------------------------------------------------------------------------
// Finds the item matching [name] in the tree, starting from [node].
// If the item is found, its parent node is opened in the browser and the item
// is selected
// -----------------------------------------------------------------------------
bool BrowserWindow::selectItem(const wxString& name, BrowserTreeNode* node)
{
	// Check node was given, if not start from root
	if (!node)
		node = items_root_;

	// Check global items
	for (auto& item : items_global_)
	{
		if (S_CMPNOCASE(name, item->name()))
		{
			openTree(node);
			canvas_->selectItem(item);
			canvas_->showSelectedItem();
			return true;
		}
	}

	// Go through all items in this node
	for (unsigned a = 0; a < node->nItems(); a++)
	{
		// Check for name match (not case-sensitive)
		if (S_CMPNOCASE(node->item(a)->name(), name))
		{
			// Open this node in the browser and select the item
			openTree(node);
			canvas_->selectItem(node->item(a));
			canvas_->showSelectedItem();
			tree_items_->Select(node->treeId());
			tree_items_->Expand(node->treeId());
			return true;
		}
	}

	// Item not found in this one, try its child nodes
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		if (selectItem(name, dynamic_cast<BrowserTreeNode*>(node->child(a))))
			return true;
	}

	// Item not found
	return false;
}

// Sorting functions
bool sortBIIndex(BrowserItem* left, BrowserItem* right)
{
	return left->index() < right->index();
}
bool sortBIName(BrowserItem* left, BrowserItem* right)
{
	return left->name() < right->name();
}

// -----------------------------------------------------------------------------
// Adds a sorting type [name] to the window
// -----------------------------------------------------------------------------
unsigned BrowserWindow::addSortType(const wxString& name) const
{
	choice_sort_->AppendString(name);
	return choice_sort_->GetCount() - 1;
}

// -----------------------------------------------------------------------------
// Performs sorting of the items currently being browsed, the sort criteria
// depending on [sort_type]. Default sorting types are by index (0) and by name
// (1), more can be added to BrowserWindow child classes if needed
// -----------------------------------------------------------------------------
void BrowserWindow::doSort(unsigned sort_type)
{
	// Get item list
	auto& items = canvas_->itemList();

	// Do sorting

	// 0: By Index
	if (sort_type == 0)
		std::sort(items.begin(), items.end(), sortBIIndex);

	// 1: By Name (Alphabetical)
	else if (sort_type == 1)
		std::sort(items.begin(), items.end(), sortBIName);

	// Refresh canvas
	canvas_->showSelectedItem();
	canvas_->Refresh();
}

// -----------------------------------------------------------------------------
// Sets the current sorting method to [type]
// -----------------------------------------------------------------------------
void BrowserWindow::setSortType(int type)
{
	// Check type index
	if (type < 0 || (unsigned)type >= choice_sort_->GetCount())
		return;

	// Select sorting type
	choice_sort_->SetSelection(type);

	// Do sorting
	doSort(type);
}

// -----------------------------------------------------------------------------
// 'Opens' the items in [node] and all its children, adding them to the browser
// canvas' list of items. If [clear] is true, the current list contents will be
// cleared
// -----------------------------------------------------------------------------
void BrowserWindow::openTree(BrowserTreeNode* node, bool clear)
{
	// Clear if needed
	if (clear)
	{
		canvas_->clearItems();

		// Also add global items
		for (auto& item : items_global_)
			canvas_->addItem(item);
	}

	// Add all items in the node
	for (unsigned a = 0; a < node->nItems(); a++)
	{
		canvas_->addItem(node->item(a));
		node->item(a)->parent_ = this;
	}

	// Add all child nodes' items
	for (unsigned a = 0; a < node->nChildren(); a++)
		openTree(dynamic_cast<BrowserTreeNode*>(node->child(a)), false);

	// If the list was cleared, sort it, filter it and update the canvas scrollbar
	if (clear)
	{
		doSort(choice_sort_->GetSelection());
		canvas_->filterItems(text_filter_->GetValue());
		canvas_->showSelectedItem();
	}
}

// -----------------------------------------------------------------------------
// Populates the wxTreeCtrl with the contents of the browser item category tree
// -----------------------------------------------------------------------------
void BrowserWindow::populateItemTree(bool collapse_all)
{
	// Clear current tree
	tree_items_->DeleteAllItems();
	tree_items_->ClearColumns();

	// Add root item
	tree_items_->AppendColumn("Categories", wxCOL_WIDTH_AUTOSIZE);
	auto item = tree_items_->AppendItem(tree_items_->GetRootItem(), "All");
	tree_items_->SetItemData(item, new BrowserTreeItemData(items_root_));

	// Add tree
	addItemTree(items_root_, item);

	// Update window layout
	int depth    = expandTree(tree_items_, item, true, 0);
	int colwidth = tree_items_->GetColumnWidth(0);
#ifndef __WXMSW__
	if (colwidth < 140)
		colwidth = 200;
#endif
	tree_items_->SetMinSize(wxSize(colwidth + 16, -1));
	Layout();
	if (collapse_all)
		expandTree(tree_items_, item, false, 0);
}

// -----------------------------------------------------------------------------
// Adds [node] to the wxTreeCtrl after [item]
// -----------------------------------------------------------------------------
void BrowserWindow::addItemTree(BrowserTreeNode* node, wxTreeListItem& item) const
{
	// Go through child items
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		// Add tree item
		auto child = dynamic_cast<BrowserTreeNode*>(node->child(a));
		auto id    = tree_items_->AppendItem(item, child->name(), -1, -1, new BrowserTreeItemData(child));
		child->setTreeId(id);

		// Add children
		addItemTree(child, id);
	}
}

// -----------------------------------------------------------------------------
// Sets the font to be used for item names
// -----------------------------------------------------------------------------
void BrowserWindow::setFont(Drawing::Font font) const
{
	canvas_->setFont(font);
}

// -----------------------------------------------------------------------------
// Sets the type of item names to show (in normal view mode)
// -----------------------------------------------------------------------------
void BrowserWindow::setItemNameType(BrowserCanvas::NameType type) const
{
	canvas_->setItemNameType(type);
}

// -----------------------------------------------------------------------------
// Sets the item size (0 or less to use zoom slider)
// -----------------------------------------------------------------------------
void BrowserWindow::setItemSize(int size)
{
	canvas_->setItemSize(size);

	if (size <= 0)
		slider_zoom_->Enable(true);
	else
		slider_zoom_->Enable(false);

	Layout();
	Refresh();
}

// -----------------------------------------------------------------------------
// Sets the item view type
// -----------------------------------------------------------------------------
void BrowserWindow::setItemViewType(BrowserCanvas::ItemView type) const
{
	canvas_->setItemViewType(type);
}


// -----------------------------------------------------------------------------
//
// BrowserWindow Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when an item on the category wxTreeCtrl is selected
// -----------------------------------------------------------------------------
void BrowserWindow::onTreeItemSelected(wxTreeListEvent& e)
{
	auto data = dynamic_cast<BrowserTreeItemData*>(tree_items_->GetItemData(e.GetItem()));
	if (data)
		openTree(data->node());
	canvas_->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the 'Sort By' dropdown selection is changed
// -----------------------------------------------------------------------------
void BrowserWindow::onChoiceSortChanged(wxCommandEvent& e)
{
	// Re-sort items
	doSort(choice_sort_->GetSelection());
}

// -----------------------------------------------------------------------------
// Called when the browser canvas is double-clicked
// -----------------------------------------------------------------------------
void BrowserWindow::onCanvasDClick(wxMouseEvent& e)
{
	// End modal if an item was double-clicked
	EndModal(wxID_OK);
}

// -----------------------------------------------------------------------------
// Called when the name filter is changed
// -----------------------------------------------------------------------------
void BrowserWindow::onTextFilterChanged(wxCommandEvent& e)
{
	// Filter canvas items
	canvas_->filterItems(text_filter_->GetValue());
}

// -----------------------------------------------------------------------------
// Called when the zoom slider is changed
// -----------------------------------------------------------------------------
void BrowserWindow::onZoomChanged(wxCommandEvent& e)
{
	// Get zoom value
	int item_size = slider_zoom_->GetValue();

	// Lock to increments of 16
	int remainder = item_size % 16;
	item_size -= remainder;

	// Set slider value to locked increment
	slider_zoom_->SetValue(item_size);

	// Update zoom label
	// label_current_zoom->SetLabel(S_FMT("%d%%", zoom_percent));

	// Update item size and refresh
	if (item_size != browser_item_size)
	{
		int viewed_index  = canvas_->getViewedIndex();
		browser_item_size = item_size;
		canvas_->updateLayout(viewed_index);
	}
}

// -----------------------------------------------------------------------------
// Called when the selection changes on the browser canvas
// -----------------------------------------------------------------------------
void BrowserWindow::onCanvasSelectionChanged(wxEvent& e)
{
	// Get selected item
	auto item = canvas_->selectedItem();
	if (!item)
	{
		// Clear info if nothing selected
		label_info_->SetLabel("");
		Refresh();
		return;
	}

	// Build info string
	wxString info       = item->name();
	wxString info_extra = item->itemInfo();
	if (!info_extra.IsEmpty())
	{
		info += ": ";
		info += info_extra;
	}

	// Set info label
	label_info_->SetLabel(info);
	Refresh();
	return;
}

// -----------------------------------------------------------------------------
// Called when a key is pressed in the browser canvas
// -----------------------------------------------------------------------------
void BrowserWindow::onCanvasKeyChar(wxKeyEvent& e)
{
	// Backspace
	if (e.GetKeyCode() == WXK_BACK && !text_filter_->GetValue().empty())
	{
		wxString filter = text_filter_->GetValue();
		filter.RemoveLast(1);
		text_filter_->SetValue(filter);
		e.Skip();
		return;
	}

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
		wxString filter = text_filter_->GetValue();
		filter += e.GetKeyCode();
		filter.MakeUpper();
		text_filter_->SetValue(filter);
	}
}

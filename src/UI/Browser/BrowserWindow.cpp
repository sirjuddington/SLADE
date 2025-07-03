
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "BrowserCanvas.h"
#include "BrowserItem.h"
#include "Graphics/Palette/Palette.h"
#include "UI/Layout.h"
#include "UI/State.h"
#include "UI/UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
int bw_chars[] = {
	'.', ',', '_', '-', '+', '=', '`',  '~', '!', '@', '#', '$', '(', ')',  '[',
	']', '{', '}', ':', ';', '/', '\\', '<', '>', '?', '^', '&', '*', '\'', '\"',
};
int n_bw_chars = 30;
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
int expandTree(wxTreeListCtrl* tree, const wxTreeListItem& item, bool expand, int depth)
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
// BrowserTreeNode class constructor
// -----------------------------------------------------------------------------
BrowserTreeNode::BrowserTreeNode(BrowserTreeNode* parent) : STreeNode(parent) {}

// -----------------------------------------------------------------------------
// BrowserTreeNode class destructor
// -----------------------------------------------------------------------------
BrowserTreeNode::~BrowserTreeNode()
{
	clearItems();
}

// -----------------------------------------------------------------------------
// Clears all items in the node
// -----------------------------------------------------------------------------
void BrowserTreeNode::clearItems()
{
	items_.clear();
}

// -----------------------------------------------------------------------------
// Returns the number of items
// -----------------------------------------------------------------------------
unsigned BrowserTreeNode::nItems() const
{
	return items_.size();
}

// -----------------------------------------------------------------------------
// Gets the item at [index], or NULL if [index] is out of bounds
// -----------------------------------------------------------------------------
BrowserItem* BrowserTreeNode::item(unsigned index) const
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
	~BrowserTreeItemData() override = default;

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
	wxDialog{ parent,         -1,
			  wxS("Browser"), wxDefaultPosition,
			  wxDefaultSize,  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX },
	palette_{ new Palette },
	truncate_names_{ truncate_names }
{
	auto lh = ui::LayoutHelper(this);

	// Init size/pos
	auto info = ui::getWindowInfo(this, "browser");
	if (!info.id.empty())
	{
		SetClientSize(info.width, info.height);
		SetPosition(wxPoint(info.left, info.top));
	}
	else
		ui::setWindowInfo(this, "browser", FromDIP(768), FromDIP(600), 0, 0);

	// Init variables
	items_root_ = new BrowserTreeNode();
	items_root_->setName("All");

	// Setup layout
	auto m_vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_vbox);

	auto m_hbox = new wxBoxSizer(wxHORIZONTAL);
	m_vbox->Add(m_hbox, lh.sfWithLargeBorder(1).Expand());

	// Browser tree
	tree_items_ = new wxTreeListCtrl(this, -1, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE | wxDV_ROW_LINES);
	m_hbox->Add(tree_items_, lh.sfWithBorder(0, wxRIGHT).Expand());

	// Browser area
	auto vbox = new wxBoxSizer(wxVERTICAL);
	m_hbox->Add(vbox, wxSizerFlags(1).Expand());

	// Zoom
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, lh.sfWithBorder(0, wxBOTTOM).Expand());
	slider_zoom_ = new wxSlider(this, -1, browser_item_size, 64, 256);
	slider_zoom_->SetLineSize(16);
	slider_zoom_->SetPageSize(32);
	hbox->Add(new wxStaticText(this, -1, wxS("Zoom:")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	hbox->Add(slider_zoom_, wxSizerFlags(1).Expand());

	// Sorting
	choice_sort_ = new wxChoice(this, -1);
	hbox->AddStretchSpacer();
	hbox->Add(new wxStaticText(this, -1, wxS("Sort:")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	hbox->Add(choice_sort_, lh.sfWithBorder(0, wxRIGHT).Expand());

	// Filter
	text_filter_ = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	hbox->Add(new wxStaticText(this, -1, wxS("Filter:")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	hbox->Add(text_filter_, wxSizerFlags().Expand());

	// Browser canvas
	hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, wxSizerFlags(1).Expand());
	canvas_ = new BrowserCanvas(this);
	hbox->Add(canvas_, wxSizerFlags(1).Expand());

	// Canvas scrollbar
	auto scrollbar = new wxScrollBar(this, -1, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
	hbox->Add(scrollbar, wxSizerFlags().Expand());
	canvas_->setScrollBar(scrollbar);

	// Bottom sizer
	sizer_bottom_ = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(sizer_bottom_, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Buttons and info label
	label_info_      = new wxStaticText(this, -1, wxEmptyString);
	auto buttonsizer = CreateButtonSizer(wxOK | wxCANCEL);
	buttonsizer->Insert(0, label_info_, lh.sfWithBorder(1, wxLEFT | wxRIGHT).CenterVertical());

	m_vbox->Add(buttonsizer, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

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
	wxTopLevelWindowBase::SetMinSize(lh.size(540, 400));

	if (ui::getStateBool(ui::BROWSERWINDOW_MAXIMIZED))
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
	auto scale = wxWindowBase::GetContentScaleFactor();
	ui::saveStateBool(ui::BROWSERWINDOW_MAXIMIZED, wxTopLevelWindow::IsMaximized());
	const wxSize ClientSize = GetClientSize() * scale;
	if (!wxTopLevelWindow::IsMaximized())
		ui::setWindowInfo(
			this, "browser", ClientSize.x, ClientSize.y, GetPosition().x * scale, GetPosition().y * scale);
}

// -----------------------------------------------------------------------------
// Sets the browser palette to [pal]
// -----------------------------------------------------------------------------
void BrowserWindow::setPalette(const Palette* pal) const
{
	palette_->copyPalette(pal);
}

// -----------------------------------------------------------------------------
// Adds [item] to the browser tree at the tree path [where].
// This will be created if it doesn't exist
// -----------------------------------------------------------------------------
bool BrowserWindow::addItem(BrowserItem* item, const string& where)
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
void BrowserWindow::reloadItems(const BrowserTreeNode* node) const
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
bool BrowserWindow::selectItem(string_view name, const BrowserTreeNode* node)
{
	// Check node was given, if not start from root
	if (!node)
		node = items_root_;

	// Check global items
	for (auto& item : items_global_)
	{
		if (strutil::equalCI(name, item->name()))
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
		if (strutil::equalCI(node->item(a)->name(), name))
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
bool sortBIIndex(const BrowserItem* left, const BrowserItem* right)
{
	return left->index() < right->index();
}
bool sortBIName(const BrowserItem* left, const BrowserItem* right)
{
	return left->name() < right->name();
}

// -----------------------------------------------------------------------------
// Adds a sorting type [name] to the window
// -----------------------------------------------------------------------------
unsigned BrowserWindow::addSortType(const string& name) const
{
	choice_sort_->AppendString(wxString::FromUTF8(name));
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
	if (type < 0 || static_cast<unsigned>(type) >= choice_sort_->GetCount())
		return;

	// Select sorting type
	choice_sort_->SetSelection(type);

	// Do sorting
	doSort(type);
}

// -----------------------------------------------------------------------------
// 'Opens' the items in [node] and all its children, adding them to the browser
// canvas' list of items. If [clear] is true, the current list contents will be
// cleared. If [show] is true, expands the tree to show the item.
// -----------------------------------------------------------------------------
void BrowserWindow::openTree(const BrowserTreeNode* node, bool clear, bool show)
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
		canvas_->filterItems(text_filter_->GetValue().utf8_string());
		canvas_->showSelectedItem();
	}

	// Expand the tree to show the node
	if (show)
	{
		tree_items_->Expand(node->treeId());
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
	tree_items_->AppendColumn(wxS("Categories"), wxCOL_WIDTH_AUTOSIZE);
	auto item = tree_items_->AppendItem(tree_items_->GetRootItem(), wxS("All"));
	tree_items_->SetItemData(item, new BrowserTreeItemData(items_root_));

	// Add tree
	addItemTree(items_root_, item);

	// Update window layout
	expandTree(tree_items_, item, true, 0);
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
void BrowserWindow::addItemTree(const BrowserTreeNode* node, const wxTreeListItem& item) const
{
	// Go through child items
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		// Add tree item
		auto child = dynamic_cast<BrowserTreeNode*>(node->child(a));
		auto id    = tree_items_->AppendItem(
            item, wxString::FromUTF8(child->name()), -1, -1, new BrowserTreeItemData(child));
		child->setTreeId(id);

		// Add children
		addItemTree(child, id);
	}
}

// -----------------------------------------------------------------------------
// Sets the font to be used for item names
// -----------------------------------------------------------------------------
void BrowserWindow::setFont(gl::draw2d::Font font) const
{
	canvas_->setFont(font);
}

// -----------------------------------------------------------------------------
// Sets the type of item names to show (in normal view mode)
// -----------------------------------------------------------------------------
void BrowserWindow::setItemNameType(browser::NameType type) const
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
void BrowserWindow::setItemViewType(browser::ItemView type) const
{
	canvas_->setItemViewType(type);
}


// -----------------------------------------------------------------------------
//
// BrowserWindow Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

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
	canvas_->filterItems(text_filter_->GetValue().utf8_string());
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
	// label_current_zoom->SetLabel(wxString::Format("%d%%", zoom_percent));

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
		label_info_->SetLabel(wxEmptyString);
		Refresh();
		return;
	}

	// Build info string
	canvas_->activateContext(); // Need to do this as itemInfo() may use the GL context
	auto info       = item->name();
	auto info_extra = item->itemInfo();
	if (!info_extra.empty())
	{
		info += ": ";
		info += info_extra;
	}

	// Set info label
	label_info_->SetLabel(wxString::FromUTF8(info));
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when a key is pressed in the browser canvas
// -----------------------------------------------------------------------------
void BrowserWindow::onCanvasKeyChar(wxKeyEvent& e)
{
	// Backspace
	if (e.GetKeyCode() == WXK_BACK && !text_filter_->GetValue().empty())
	{
		auto filter = text_filter_->GetValue();
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
		auto filter = text_filter_->GetValue();
		filter += e.GetKeyCode();
		filter.MakeUpper();
		text_filter_->SetValue(filter);
	}
}

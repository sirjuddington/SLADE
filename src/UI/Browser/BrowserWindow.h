#pragma once

#include "BrowserCanvas.h"
#include "BrowserItem.h"
#include "Graphics/Palette/Palette.h"
#include "Utility/Tree.h"

namespace Drawing
{
enum class Font;
}
class wxChoice;
class wxTextCtrl;
class wxSlider;
class wxStaticText;

class BrowserTreeNode : public STreeNode
{
public:
	BrowserTreeNode(BrowserTreeNode* parent = nullptr) : STreeNode(parent) {}
	~BrowserTreeNode() { clearItems(); }

	string         name() override { return name_; }
	wxTreeListItem treeId() const { return tree_id_; }
	void           setName(string name) override { this->name_ = name; }
	void           setTreeId(wxTreeListItem id) { this->tree_id_ = id; }

	void         clearItems();
	unsigned     nItems() const { return items_.size(); }
	BrowserItem* item(unsigned index);
	void         addItem(BrowserItem* item, unsigned index = 0xFFFFFFFF);

private:
	string                    name_;
	vector<BrowserItem::UPtr> items_;
	wxTreeListItem            tree_id_;

	STreeNode* createChild(string name) override
	{
		auto node   = new BrowserTreeNode();
		node->name_ = name;
		return node;
	}
};

class BrowserWindow : public wxDialog
{
public:
	BrowserWindow(wxWindow* parent, bool truncate_names = false);
	~BrowserWindow();

	bool truncateNames() const { return truncate_names_; }

	Palette* palette() { return &palette_; }
	void     setPalette(Palette* pal) { palette_.copyPalette(pal); }

	bool         addItem(BrowserItem* item, const string& where = "");
	void         addGlobalItem(BrowserItem* item);
	void         clearItems(BrowserTreeNode* node = nullptr) const;
	void         reloadItems(BrowserTreeNode* node = nullptr) const;
	BrowserItem* selectedItem() const;
	bool         selectItem(const string& name, BrowserTreeNode* node = nullptr);

	unsigned     addSortType(const string& name) const;
	virtual void doSort(unsigned sort_type = 0);
	void         setSortType(int type);

	void openTree(BrowserTreeNode* node, bool clear = true);
	void populateItemTree(bool collapse_all = true);
	void addItemTree(BrowserTreeNode* node, wxTreeListItem& item) const;

	// Canvas display options
	void setFont(Drawing::Font font) const;
	void setItemNameType(BrowserCanvas::NameType type) const;
	void setItemSize(int size);
	void setItemViewType(BrowserCanvas::ItemView type) const;

protected:
	BrowserTreeNode*     items_root_   = nullptr;
	wxBoxSizer*          sizer_bottom_ = nullptr;
	Palette              palette_;
	BrowserCanvas*       canvas_ = nullptr;
	vector<BrowserItem*> items_global_;
	bool                 truncate_names_ = false;

private:
	wxTreeListCtrl* tree_items_  = nullptr;
	wxChoice*       choice_sort_ = nullptr;
	wxTextCtrl*     text_filter_ = nullptr;
	wxSlider*       slider_zoom_ = nullptr;
	wxStaticText*   label_info_  = nullptr;

	// Events
	void onTreeItemSelected(wxTreeListEvent& e);
	void onChoiceSortChanged(wxCommandEvent& e);
	void onCanvasDClick(wxMouseEvent& e);
	void onTextFilterChanged(wxCommandEvent& e);
	void onZoomChanged(wxCommandEvent& e);
	void onCanvasSelectionChanged(wxEvent& e);
	void onCanvasKeyChar(wxKeyEvent& e);
};

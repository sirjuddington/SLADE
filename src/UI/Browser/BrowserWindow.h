#pragma once

#include "BrowserCanvas.h"
#include "BrowserItem.h"
#include "Graphics/Palette/Palette.h"
#include "Utility/Tree.h"
#include "common.h"

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
	BrowserTreeNode(BrowserTreeNode* parent = nullptr);
	~BrowserTreeNode();

	string         getName() override { return name_; }
	wxTreeListItem treeId() { return tree_id_; }
	void           setName(string name) override { this->name_ = name; }
	void           setTreeId(wxTreeListItem id) { this->tree_id_ = id; }

	void         clearItems();
	unsigned     nItems() { return items_.size(); }
	BrowserItem* getItem(unsigned index);
	void         addItem(BrowserItem* item, unsigned index = 0xFFFFFFFF);

private:
	string               name_;
	vector<BrowserItem*> items_;
	wxTreeListItem       tree_id_;

	STreeNode* createChild(string name) override
	{
		BrowserTreeNode* node = new BrowserTreeNode();
		node->name_           = name;
		return node;
	}
};

class BrowserWindow : public wxDialog
{
public:
	BrowserWindow(wxWindow* parent);
	~BrowserWindow();

	bool truncateNames() { return truncate_names_; }

	Palette* getPalette() { return &palette_; }
	void     setPalette(Palette* pal) { palette_.copyPalette(pal); }

	bool         addItem(BrowserItem* item, string where = "");
	void         addGlobalItem(BrowserItem* item);
	void         clearItems(BrowserTreeNode* node = nullptr);
	void         reloadItems(BrowserTreeNode* node = nullptr);
	BrowserItem* getSelectedItem();
	bool         selectItem(string name, BrowserTreeNode* root = nullptr);

	unsigned     addSortType(string name);
	virtual void doSort(unsigned sort_type = 0);
	void         setSortType(int type);

	void openTree(BrowserTreeNode* node, bool clear = true);
	void populateItemTree(bool collapse_all = true);
	void addItemTree(BrowserTreeNode* node, wxTreeListItem& item);

	// Canvas display options
	void setFont(Drawing::Font font);
	void setItemNameType(int type);
	void setItemSize(int size);
	void setItemViewType(int type);

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

#pragma once

#include "Utility/Tree.h"

namespace slade
{
class BrowserCanvas;
class BrowserItem;
namespace browser
{
	enum class ItemView;
	enum class NameType;
} // namespace browser
namespace gl::draw2d
{
	enum class Font;
}

class BrowserTreeNode : public STreeNode
{
public:
	BrowserTreeNode(BrowserTreeNode* parent = nullptr);
	~BrowserTreeNode() override;

	const string&  name() const override { return name_; }
	wxTreeListItem treeId() const { return tree_id_; }
	void           setName(string_view name) override { name_ = name; }
	void           setTreeId(wxTreeListItem id) { tree_id_ = id; }

	void         clearItems();
	unsigned     nItems() const;
	BrowserItem* item(unsigned index) const;
	void         addItem(BrowserItem* item, unsigned index = 0xFFFFFFFF);

private:
	string                          name_;
	vector<unique_ptr<BrowserItem>> items_;
	wxTreeListItem                  tree_id_;

	STreeNode* createChild(string_view name) override
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
	~BrowserWindow() override;

	bool truncateNames() const { return truncate_names_; }

	Palette* palette() const { return palette_.get(); }
	void     setPalette(const Palette* pal) const;

	bool         addItem(BrowserItem* item, const string& where = "");
	void         addGlobalItem(BrowserItem* item);
	void         clearItems(BrowserTreeNode* node = nullptr) const;
	void         reloadItems(const BrowserTreeNode* node = nullptr) const;
	BrowserItem* selectedItem() const;
	bool         selectItem(string_view name, const BrowserTreeNode* node = nullptr);

	unsigned     addSortType(const string& name) const;
	virtual void doSort(unsigned sort_type = 0);
	void         setSortType(int type);

	void openTree(const BrowserTreeNode* node, bool clear = true, bool show = false);
	void populateItemTree(bool collapse_all = true);
	void addItemTree(const BrowserTreeNode* node, const wxTreeListItem& item) const;

	// Canvas display options
	void setFont(gl::draw2d::Font font) const;
	void setItemNameType(browser::NameType type) const;
	void setItemSize(int size);
	void setItemViewType(browser::ItemView type) const;

protected:
	BrowserTreeNode*     items_root_   = nullptr;
	wxBoxSizer*          sizer_bottom_ = nullptr;
	unique_ptr<Palette>  palette_;
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
} // namespace slade

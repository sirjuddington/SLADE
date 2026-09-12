#pragma once

#include "General/Sigslot.h"

namespace slade
{
class CTexture;
class TextureXList;
} // namespace slade

namespace slade::texeditor
{
class TextureEditor;

class TextureTreeModel : public wxDataViewModel
{
public:
	enum class Column : u8
	{
		Index = 0,
		Name,
		Size,
		Type,
		Patches,

		Count
	};

	TextureTreeModel()           = default;
	~TextureTreeModel() override = default;

	void open(const TextureEditor& editor);

	CTexture*      textureForItem(const wxDataViewItem& item) const;
	wxDataViewItem itemForTexList(const TextureXList* list) const;

	vector<wxDataViewItem> texListItems() const;

private:
	const TextureEditor* editor_ = nullptr;
	ScopedConnectionList connections_;

	struct RootItem
	{
		TextureXList*        list = nullptr;
		unique_ptr<CTexture> id; // Dummy texture used for the wxDataViewItem ID
	};
	vector<RootItem> root_items_;

	// wxDataViewModel
	unsigned int   GetColumnCount() const override { return static_cast<unsigned int>(Column::Count); }
	bool           HasValue(const wxDataViewItem& item, unsigned col) const override { return true; }
	void           GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
	bool           GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const override;
	bool           SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
	wxDataViewItem GetParent(const wxDataViewItem& item) const override;
	bool           IsContainer(const wxDataViewItem& item) const override;
	unsigned int   GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;
	bool           IsListModel() const override { return false; }
	bool           HasDefaultCompare() const override { return true; }
	int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending)
		const override;

	static std::unordered_map<string, wxBitmapBundle>& iconCache();
};
} // namespace slade::texeditor

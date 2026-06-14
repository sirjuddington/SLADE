
#include "Main.h"
#include "TextureTreeModel.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Graphics/Icons.h"
#include "TextureEditor/TextureEditor.h"
#include "Utility/PropertyList.h"

using namespace slade;
using namespace texeditor;

void TextureTreeModel::open(const TextureEditor& editor)
{
	editor_ = &editor;

	// Build top-level items list (one per TextureXList)
	list_items_.clear();
	for (size_t i = 0; i < editor_->nTextureLists(); ++i)
		list_items_.emplace_back(Item{ .list = editor_->textureList(i), .index = -1 });

	// Refresh (will load all items)
	Cleared();
}

CTexture* TextureTreeModel::textureForItem(const wxDataViewItem& item)
{
	if (auto i = static_cast<Item*>(item.GetID()))
	{
		if (i->index >= 0)
			return i->list->texture(i->index);
	}

	return nullptr;
}

wxDataViewItem TextureTreeModel::itemForTexList(TextureXList* list)
{
	return wxDataViewItem(new Item{ .list = list, .index = -1 });
}

vector<wxDataViewItem> TextureTreeModel::texListItems()
{
	vector<wxDataViewItem> items;
	for (auto& item : list_items_)
		items.emplace_back(&item);
	return items;
}

void TextureTreeModel::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const
{
	auto i = static_cast<Item*>(item.GetID());
	if (!i)
		return;

	switch (static_cast<Column>(col))
	{
	case Column::Index: variant = wxString::FromUTF8(i->index < 0 ? " " : fmt::format("{}", i->index)); break;

	case Column::Name:
	{
		// Determine icon
		string icon;
		if (i->index >= 0)
		{
			icon     = "tlist_texture";
			auto tex = i->list->texture(i->index);
			if (strutil::equalCI(tex->type(), "sprite"))
				icon = "tlist_sprite";
			else if (strutil::equalCI(tex->type(), "graphic"))
				icon = "tlist_graphic";
			// else if (strutil::equalCI(tex->type(), "walltexture"))
			// 	icon = "tlist_walltexture";
			// else if (strutil::equalCI(tex->type(), "flat"))
			// 	icon = "tlist_flat";
		}
		else
			icon = "tlist_folder";

		// Find icon in cache
		auto&  icon_cache = iconCache();
		if (!icon_cache.contains(icon))
		{
			// Not found, add to cache
			const auto pad    = Point2i{ 1, /*elist_icon_padding*/ 1 };
			const auto bundle = icons::getIcon(icons::Type::Any, icon, /*elist_icon_size*/ 16, pad);
			icon_cache[icon]  = bundle;
		}

		// Name
		wxString name;
		if (i->index < 0)
			name = wxString::FromUTF8(editor_->textureListName(*i->list));
		else
			name = wxString::FromUTF8(i->list->texture(i->index)->name());

		variant << wxDataViewIconText(name, icon_cache[icon]);

		break;
	}

	case Column::Size:
		if (i->index < 0)
			variant = WX_FMT("{} texture{}", i->list->textures().size(), i->list->textures().size() == 1 ? "" : "s");
		else
		{
			auto tex = i->list->texture(i->index);
			variant  = WX_FMT("{} x {}", tex->width(), tex->height());
		}
		break;

	case Column::Type:
		if (i->index < 0)
			variant = wxString::FromUTF8(i->list->textureXFormatString());
		else
			variant = wxString::FromUTF8(i->list->texture(i->index)->type());
		break;

	case Column::Patches:
		if (i->index >= 0)
			variant = WX_FMT("{}", i->list->texture(i->index)->nPatches());
		break;

	default: break;
	}
}

bool TextureTreeModel::GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const
{
	return wxDataViewModel::GetAttr(item, col, attr);
}

bool TextureTreeModel::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col)
{
	return false;
}

wxDataViewItem TextureTreeModel::GetParent(const wxDataViewItem& item) const
{
	if (auto i = static_cast<Item*>(item.GetID()))
	{
		if (i->index < 0)
			return {}; // Top-level item has no parent

		// Find the stable item for this item's parent list
		for (auto& list_item : list_items_)
			if (list_item.list == i->list)
				return wxDataViewItem(const_cast<Item*>(&list_item));
	}

	return {};
}

bool TextureTreeModel::IsContainer(const wxDataViewItem& item) const
{
	if (auto i = static_cast<Item*>(item.GetID()))
		return i->index < 0;

	return editor_->nTextureLists() > 0;
}

unsigned int TextureTreeModel::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
{
	if (auto i = static_cast<Item*>(item.GetID()))
	{
		if (i->index == -1)
		{
			// Index of -1 means this item is a TextureXList, add all textures in the list as children
			for (int idx = 0; idx < i->list->textures().size(); ++idx)
			{
				auto child = new Item{ .list = i->list, .index = idx };
				children.Add(wxDataViewItem(child));
			}

			return i->list->textures().size();
		}
	}

	if (!item.IsOk())
	{
		// Top-level: return the stable list items
		for (auto& child : list_items_)
			children.Add(wxDataViewItem(const_cast<Item*>(&child)));

		return static_cast<unsigned int>(list_items_.size());
	}

	// No children
	return 0;
}

std::unordered_map<string, wxBitmapBundle>& TextureTreeModel::iconCache()
{
	static std::unordered_map<string, wxBitmapBundle> cache;
	return cache;
}

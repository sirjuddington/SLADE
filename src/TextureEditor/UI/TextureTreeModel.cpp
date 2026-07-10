
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

	root_items_.clear();
	for (size_t i = 0; i < editor_->nTextureLists(); ++i)
	{
		auto tex_id = std::make_unique<CTexture>();
		tex_id->setList(editor_->textureList(i));
		root_items_.emplace_back(RootItem{ .list = editor_->textureList(i), .id = std::move(tex_id) });
	}

	// Refresh (will load all items)
	Cleared();


	// --- Connect to TextureEditor signals ---

	// Texture added
	connections_ += editor.signals().texture_added.connect(
		[this](TextureXList* list, CTexture* tex)
		{
			if (auto parent = itemForTexList(list); parent.IsOk())
				ItemAdded(wxDataViewItem(parent), wxDataViewItem(tex));
			else
				log::warning("Texture \"{}\" added to unknown list", tex->name());
		});

	// Texture deleted
	connections_ += editor.signals().texture_deleted.connect(
		[this](TextureXList* list, CTexture* tex)
		{
			if (auto parent = itemForTexList(list); parent.IsOk())
				ItemDeleted(parent, wxDataViewItem(tex));
			else
				log::warning("Texture \"{}\" deleted from unknown list", tex->name());
		});

	// Textures swapped
	connections_ += editor.signals().textures_swapped.connect(
		[this](TextureXList* list, unsigned index1, unsigned index2)
		{
			if (auto parent = itemForTexList(list); parent.IsOk())
			{
				auto tex1 = list->texture(index1);
				auto tex2 = list->texture(index2);
				if (tex1 && tex2)
				{
					ItemChanged(wxDataViewItem(tex1));
					ItemChanged(wxDataViewItem(tex2));
				}
			}
			else
				log::warning("Textures swapped in unknown list");
		});
}

CTexture* TextureTreeModel::textureForItem(const wxDataViewItem& item) const
{
	if (auto ctex = static_cast<CTexture*>(item.GetID()); ctex && ctex->index() >= 0)
		return ctex;

	return nullptr;
}

wxDataViewItem TextureTreeModel::itemForTexList(const TextureXList* list) const
{
	for (auto& item : root_items_)
		if (item.list == list)
			return wxDataViewItem(item.id.get());

	return {};
}

vector<wxDataViewItem> TextureTreeModel::texListItems() const
{
	vector<wxDataViewItem> items;
	for (auto& item : root_items_)
		items.emplace_back(item.id.get());
	return items;
}

void TextureTreeModel::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const
{
	auto tex = static_cast<CTexture*>(item.GetID());
	if (!tex)
		return;

	switch (static_cast<Column>(col))
	{
	case Column::Index: variant = wxString::FromUTF8(tex->index() < 0 ? " " : fmt::format("{}", tex->index())); break;

	case Column::Name:
	{
		// Determine icon
		string icon;
		if (tex->index() >= 0)
		{
			icon = "tlist_texture";
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
		auto& icon_cache = iconCache();
		if (!icon_cache.contains(icon))
		{
			// Not found, add to cache
			const auto pad    = Point2i{ 1, /*elist_icon_padding*/ 1 };
			const auto bundle = icons::getIcon(icons::Type::Any, icon, /*elist_icon_size*/ 16, pad);
			icon_cache[icon]  = bundle;
		}

		// Name
		wxString name;
		if (tex->index() < 0)
			name = wxString::FromUTF8(editor_->textureListName(*tex->list()));
		else
			name = wxString::FromUTF8(tex->name());

		variant << wxDataViewIconText(name, icon_cache[icon]);

		break;
	}

	case Column::Size:
		if (tex->index() < 0)
			variant = WX_FMT(
				"{} texture{}", tex->list()->textures().size(), tex->list()->textures().size() == 1 ? "" : "s");
		else
		{
			variant = WX_FMT("{} x {}", tex->width(), tex->height());
		}
		break;

	case Column::Type:
		if (tex->index() < 0)
			variant = wxString::FromUTF8(tex->list()->textureXFormatString());
		else
			variant = wxString::FromUTF8(tex->type());
		break;

	case Column::Patches:
		if (tex->index() >= 0)
			variant = WX_FMT("{}", tex->list()->texture(tex->index())->nPatches());
		break;

	default: break;
	}
}

bool TextureTreeModel::GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const
{
	auto tex = static_cast<CTexture*>(item.GetID());
	if (!tex)
		return false;

	bool has_attr = false;

	// Status colour
	static wxColour col_text_modified(0, 0, 0, 0);
	static wxColour col_text_new(0, 0, 0, 0);
	if (tex->index() >= 0)
	{
		// Init precalculated status text colours if necessary
		if (col_text_modified.Alpha() == 0)
		{
			const auto     col_modified = ColRGBA(0, 85, 255);
			const auto     col_new      = ColRGBA(0, 255, 0);
			const auto     col_text     = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
			constexpr auto intensity    = 0.65;

			col_text_modified.Set(
				static_cast<uint8_t>(col_modified.r * intensity + col_text.Red() * (1.0 - intensity)),
				static_cast<uint8_t>(col_modified.g * intensity + col_text.Green() * (1.0 - intensity)),
				static_cast<uint8_t>(col_modified.b * intensity + col_text.Blue() * (1.0 - intensity)),
				255);

			col_text_new.Set(
				static_cast<uint8_t>(col_new.r * intensity + col_text.Red() * (1.0 - intensity)),
				static_cast<uint8_t>(col_new.g * intensity + col_text.Green() * (1.0 - intensity)),
				static_cast<uint8_t>(col_new.b * intensity + col_text.Blue() * (1.0 - intensity)),
				255);
		}

		if (tex->state() == CTexture::State::Modified)
		{
			attr.SetColour(col_text_modified);
			has_attr = true;
		}
		else if (tex->state() == CTexture::State::New)
		{
			attr.SetColour(col_text_new);
			has_attr = true;
		}
	}

	return has_attr;
}

bool TextureTreeModel::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col)
{
	return false;
}

wxDataViewItem TextureTreeModel::GetParent(const wxDataViewItem& item) const
{
	if (auto tex = static_cast<CTexture*>(item.GetID()))
	{
		if (tex->index() < 0)
			return {}; // Top-level item has no parent

		// Find the root item for this item's parent list
		for (auto& root : root_items_)
			if (root.list == tex->list())
				return wxDataViewItem(root.id.get());
	}

	return {};
}

bool TextureTreeModel::IsContainer(const wxDataViewItem& item) const
{
	if (auto tex = static_cast<CTexture*>(item.GetID()))
		return tex->index() < 0;

	return editor_->nTextureLists() > 0;
}

unsigned int TextureTreeModel::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
{
	if (auto tex = static_cast<CTexture*>(item.GetID()))
	{
		if (tex->index() < 0)
		{
			// Index of -1 means this item is a TextureXList, add all textures in the list as children
			for (const auto& t : tex->list()->textures())
				children.Add(wxDataViewItem(t.get()));

			return tex->list()->textures().size();
		}
	}

	if (!item.IsOk())
	{
		// Top-level: return the root items
		for (auto& root : root_items_)
			children.Add(wxDataViewItem(root.id.get()));

		return static_cast<unsigned int>(root_items_.size());
	}

	// No children
	return 0;
}

int TextureTreeModel::Compare(
	const wxDataViewItem& item1,
	const wxDataViewItem& item2,
	unsigned int          column,
	bool                  ascending) const
{
	auto tex1 = static_cast<CTexture*>(item1.GetID());
	auto tex2 = static_cast<CTexture*>(item2.GetID());
	if (tex1 && tex1->index() >= 0 && tex2 && tex2->index() >= 0)
	{
		switch (static_cast<Column>(column))
		{
		case Column::Name: return tex1->name() < tex2->name() ? -1 : (tex1->name() > tex2->name() ? 1 : 0);
		case Column::Size: return (tex1->width() * tex1->height()) - (tex2->width() * tex2->height());
		case Column::Type:
			return tex1->typeEnum() < tex2->typeEnum() ? -1 : (tex1->typeEnum() > tex2->typeEnum() ? 1 : 0);
		case Column::Patches: return static_cast<int>(tex1->nPatches()) - static_cast<int>(tex2->nPatches());
		default:              return tex1->index() - tex2->index();
		}
	}

	return wxDataViewModel::Compare(item1, item2, column, ascending);
}

std::unordered_map<string, wxBitmapBundle>& TextureTreeModel::iconCache()
{
	static std::unordered_map<string, wxBitmapBundle> cache;
	return cache;
}

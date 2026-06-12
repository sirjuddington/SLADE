
#include "Main.h"
#include "TextureEditor.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace texeditor;

TextureEditor::TextureEditor(shared_ptr<Archive> archive) : archive_{ archive }
{
	if (!archive_)
		return;

	patch_table_ = std::make_unique<PatchTable>(archive_.get());

	// Find patch table (if any)
	if (auto pnames = archive_->findLast({ .match_type = EntryType::fromId("pnames") }))
		patch_table_->loadPNAMES(pnames);

	// Find all TEXTUREx entries in the archive
	auto texturex = archive_->findAll({ .match_type = EntryType::fromId("texturex") });
	for (auto& entry : texturex)
	{
		// Add to list if we can read it successfully
		if (auto tx_list = std::make_unique<TextureXList>(); tx_list->readTEXTUREXData(entry, *patch_table_))
			texturex_entries_.emplace_back(
				TextureXEntry{ .entry = entry->getShared(), .texturex = std::move(tx_list) });
	}

	// Find all TEXTURES entries in the archive
	auto textures = archive_->findAll({ .match_type = EntryType::fromId("zdtextures") });
	for (auto& entry : textures)
	{
		// Add to list if we can read it successfully
		if (auto tx_list = std::make_unique<TextureXList>(); tx_list->readTEXTURESData(entry))
			texturex_entries_.emplace_back(
				TextureXEntry{ .entry = entry->getShared(), .texturex = std::move(tx_list) });
	}
}

TextureEditor::~TextureEditor() = default;

TextureXList* TextureEditor::textureList(unsigned index) const
{
	if (index >= texturex_entries_.size())
		return nullptr;
	return texturex_entries_[index].texturex.get();
}

ArchiveEntry* TextureEditor::textureListEntry(unsigned index) const
{
	if (index >= texturex_entries_.size())
		return nullptr;
	return texturex_entries_[index].entry.lock().get();
}

string TextureEditor::textureListName(const TextureXList& list) const
{
	for (const auto& tx : texturex_entries_)
	{
		if (auto entry = tx.entry.lock(); entry && tx.texturex.get() == &list)
		{
			string name = entry->name();

			// Remove common extensions
			// (we want to keep other extensions, eg. TEXTURES.floors)
			if (strutil::endsWithCI(name, ".txt") || strutil::endsWithCI(name, ".lmp"))
				name = name.substr(0, name.size() - 4);

			return name;
		}
	}

	return {};
}

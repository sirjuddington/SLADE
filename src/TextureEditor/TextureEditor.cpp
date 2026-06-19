
#include "Main.h"
#include "TextureEditor.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Graphics/Translation.h"
#include "Utility/StringUtils.h"
#include "Utility/Vector.h"

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

void TextureEditor::openTexture(CTexture& texture)
{
	if (!tex_current_)
		tex_current_ = std::make_unique<CTexture>();

	tex_current_source_ = &texture;
	tex_current_->copyTexture(texture);
	tex_current_->setState(CTexture::State::Unmodified);
}

void TextureEditor::closeTexture()
{
	tex_current_.reset();
	selected_patches_.clear();
}

void TextureEditor::saveTexture() const
{
	if (!tex_current_ || !tex_current_source_)
		return;

	tex_current_source_->copyTexture(*tex_current_);
	tex_current_source_->setState(CTexture::State::Modified);
	tex_current_->setState(CTexture::State::Unmodified);
}

void TextureEditor::revertTexture()
{
	if (!tex_current_ || !tex_current_source_)
		return;

	tex_current_->copyTexture(*tex_current_source_);
	tex_current_->setState(CTexture::State::Unmodified);
	selected_patches_.clear();
}

void TextureEditor::selectPatch(unsigned index, bool selected)
{
	if (selected)
		vectorAddUnique(selected_patches_, index);
	else
		vectorRemoveVal(selected_patches_, index);
}

void TextureEditor::setTextureModified(bool update_texture, bool update_patches) const
{
	tex_current_->setState(CTexture::State::Modified);

	if (update_texture)
		tex_current_->signals().texture_modified();
	if (update_patches)
		tex_current_->signals().patches_modified(selected_patches_);
}

void TextureEditor::setTextureSize(int width, int height) const
{
	if (!tex_current_)
		return;

	if (width > 0)
		tex_current_->setWidth(width);
	if (height > 0)
		tex_current_->setHeight(height);

	tex_current_->signals().texture_modified();
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setTextureScaleX(double scale) const
{
	if (!tex_current_)
		return;

	tex_current_->setScaleX(scale);

	tex_current_->signals().texture_modified();
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setTextureScaleY(double scale) const
{
	if (!tex_current_)
		return;

	tex_current_->setScaleY(scale);

	tex_current_->signals().texture_modified();
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setTextureFlag(string_view flag, bool on) const
{
	if (!tex_current_)
		return;

	if (strutil::equalCI(flag, "worldpanning"))
		tex_current_->setWorldPanning(on);
	else if (strutil::equalCI(flag, "optional"))
		tex_current_->setOptional(on);
	else if (strutil::equalCI(flag, "nodecals"))
		tex_current_->setNoDecals(on);
	else if (strutil::equalCI(flag, "nulltexture"))
		tex_current_->setNullTexture(on);
	else if (strutil::equalCI(flag, "notrim"))
		tex_current_->setNoTrim(on);
	else
		return; // Unknown flag

	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setTextureType(CTexture::Type type) const
{
	if (!tex_current_)
		return;
	tex_current_->setType(type);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setTextureOffsetX(int offset) const
{
	if (!tex_current_)
		return;
	tex_current_->setOffsetX(offset);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setTextureOffsetY(int offset) const
{
	if (!tex_current_)
		return;
	tex_current_->setOffsetY(offset);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setPatchOffsetX(int offset) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			patch->setOffsetX(offset);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setPatchOffsetY(int offset) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			patch->setOffsetY(offset);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setPatchBlendType(CTPatchEx::BlendType type) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setBlendType(type);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setPatchFlag(string_view flag, bool on) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setFlag(flag, on);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setPatchRotation(int rotation) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setRotation(rotation);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setPatchAlpha(double alpha) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setAlpha(alpha);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setPatchAlphaStyle(string_view style) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setStyle(style);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setPatchColour(const ColRGBA& colour) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setColour(colour);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setPatchTintAmount(double amount) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setTintAmount(amount);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::setPatchTranslation(string_view translation) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	Translation t;
	if (!translation.empty())
		t.parse(translation);

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setTranslation(t);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::addPatch(string_view patch) const
{
	if (!tex_current_)
		return;

	tex_current_->addPatch(patch);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::removePatch()
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	// Clear selection
	auto to_remove = selected_patches_;
	selected_patches_.clear();

	// Remove patches
	tex_current_->removePatches(to_remove);
	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::replacePatch(string_view patch) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	if (selected_patches_.size() == 1)
		tex_current_->replacePatch(selected_patches_[0], patch);
	else
		tex_current_->replacePatches(selected_patches_, patch);

	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::duplicatePatch(int xoff, int yoff)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	if (selected_patches_.size() == 1)
		tex_current_->duplicatePatch(selected_patches_[0], xoff, yoff);
	else
		tex_current_->duplicatePatches(selected_patches_, xoff, yoff);

	// Adjust selection to select the new duplicated patches
	for (unsigned& index : selected_patches_)
		index++;

	tex_current_->setState(CTexture::State::Modified);
}

void TextureEditor::patchForward()
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	if (vectorContains(selected_patches_, static_cast<unsigned>(tex_current_->nPatches() - 1)))
		return; // Can't move if last patch is selected

	if (selected_patches_.size() == 1)
		tex_current_->swapPatches(selected_patches_[0], selected_patches_[0] + 1);
	else
	{
		// Sort selection in descending order so we can safely swap patches in order
		auto sorted_selection = selected_patches_;
		std::ranges::sort(sorted_selection, std::greater());

		// Swap selected patches forward,
		// blocking patch_list_changed signal until all swaps are done
		tex_current_->signals().patch_list_changed.block();
		for (unsigned& index : sorted_selection)
			tex_current_->swapPatches(index, index + 1);
		tex_current_->signals().patch_list_changed.unblock();
		tex_current_->signals().patch_list_changed();
	}

	// Update selection to match new patch positions
	for (unsigned& index : selected_patches_)
		index++;
}

void TextureEditor::patchBack()
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	if (vectorContains(selected_patches_, 0u))
		return; // Can't move if first patch is selected

	if (selected_patches_.size() == 1)
		tex_current_->swapPatches(selected_patches_[0], selected_patches_[0] - 1);
	else
	{
		// Sort selection in ascending order so we can safely swap patches in order
		auto sorted_selection = selected_patches_;
		std::ranges::sort(sorted_selection);

		// Swap selected patches backward,
		// blocking patch_list_changed signal until all swaps are done
		tex_current_->signals().patch_list_changed.block();
		for (unsigned index : sorted_selection)
			tex_current_->swapPatches(index, index - 1);
		tex_current_->signals().patch_list_changed.unblock();
		tex_current_->signals().patch_list_changed();
	}

	// Update selection to match new patch positions
	for (unsigned& index : selected_patches_)
		index--;
}

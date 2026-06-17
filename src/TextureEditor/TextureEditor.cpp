
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

void TextureEditor::openTexture(const CTexture& texture)
{
	if (!tex_current_)
		tex_current_ = std::make_unique<CTexture>();

	tex_current_->copyTexture(texture);
	tex_modified_ = false;
}

void TextureEditor::closeTexture()
{
	tex_current_.reset();
	tex_modified_ = false;
}

void TextureEditor::selectPatch(unsigned index, bool selected)
{
	if (selected)
		vectorAddUnique(selected_patches_, index);
	else
		vectorRemoveVal(selected_patches_, index);
}

void TextureEditor::setTextureSize(int width, int height)
{
	if (!tex_current_)
		return;

	if (width > 0)
		tex_current_->setWidth(width);
	if (height > 0)
		tex_current_->setHeight(height);

	tex_current_->signals().texture_modified();
	tex_modified_ = true;
}

void TextureEditor::setTextureScaleX(double scale)
{
	if (!tex_current_)
		return;

	tex_current_->setScaleX(scale);

	tex_current_->signals().texture_modified();
	tex_modified_ = true;
}

void TextureEditor::setTextureScaleY(double scale)
{
	if (!tex_current_)
		return;

	tex_current_->setScaleY(scale);

	tex_current_->signals().texture_modified();
	tex_modified_ = true;
}

void TextureEditor::setTextureFlag(string_view flag, bool on)
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

	tex_modified_ = true;
}

void TextureEditor::setTextureType(CTexture::Type type)
{
	if (!tex_current_)
		return;
	tex_current_->setType(type);
	tex_modified_ = true;
}

void TextureEditor::setPatchOffsetX(int offset)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			patch->setOffsetX(offset);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_modified_ = true;
}

void TextureEditor::setPatchOffsetY(int offset)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			patch->setOffsetY(offset);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_modified_ = true;
}

void TextureEditor::setPatchBlendType(CTPatchEx::BlendType type)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setBlendType(type);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_modified_ = true;
}

void TextureEditor::setPatchFlag(string_view flag, bool on)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setFlag(flag, on);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_modified_ = true;
}

void TextureEditor::setPatchRotation(int rotation)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setRotation(rotation);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_modified_ = true;
}

void TextureEditor::setPatchAlpha(double alpha)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setAlpha(alpha);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_modified_ = true;
}

void TextureEditor::setPatchAlphaStyle(string_view style)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setStyle(style);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_modified_ = true;
}

void TextureEditor::setPatchColour(const ColRGBA& colour)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setColour(colour);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_modified_ = true;
}

void TextureEditor::setPatchTintAmount(double amount)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
				patch_ex->setTintAmount(amount);

	tex_current_->signals().patches_modified(selected_patches_);
	tex_modified_ = true;
}

void TextureEditor::setPatchTranslation(string_view translation)
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
	tex_modified_ = true;
}

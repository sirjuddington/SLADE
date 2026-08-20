
#include "Main.h"
#include "TextureEditor.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Misc.h"
#include "General/UndoRedo.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Graphics/SImage/SIFormat.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "UndoSteps.h"
#include "Utility/Colour.h"
#include "Utility/StringUtils.h"
#include "Utility/Vector.h"


using namespace slade;
using namespace texeditor;

TextureEditor::TextureEditor(shared_ptr<Archive> archive) : archive_{ archive }
{
	if (!archive_)
		return;

	patch_table_  = std::make_unique<PatchTable>(archive_.get());
	undo_manager_ = std::make_unique<UndoManager>();

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

	// Lock all opened entries
	for (auto& entry : texturex_entries_)
		if (auto e = entry.entry.lock(); e)
			e->lock();
}

TextureEditor::~TextureEditor()
{
	// Unlock entries
	for (auto& entry : texturex_entries_)
		if (auto e = entry.entry.lock(); e)
			e->unlock();
}

void TextureEditor::saveAll() const
{
	for (unsigned i = 0; i < texturex_entries_.size(); ++i)
		saveTextureList(i);
}

bool TextureEditor::saveTextureList(unsigned index) const
{
	if (!textureListModified(index))
		return false;

	auto entry = textureListEntry(index);
	auto list  = textureList(index);
	if (!entry || !list)
		return false;

	// Write list to entry, in the correct format
	entry->unlock(); // Have to unlock the entry first
	bool ok = false;
	if (list->format() == TextureXList::Format::Textures)
		ok = list->writeTEXTURESData(entry);
	else
		ok = list->writeTEXTUREXData(entry, *patch_table_);

	// Redetect type and lock it up
	EntryType::detectEntryType(*entry);
	entry->lock();

	// Set all textures to unmodified
	for (unsigned a = 0; a < list->size(); a++)
		list->texture(a)->setState(CTexture::State::Unmodified);

	return ok;
}

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

string TextureEditor::textureListName(unsigned index) const
{
	if (index >= texturex_entries_.size())
		return {};

	if (auto entry = texturex_entries_[index].entry.lock(); entry)
	{
		string name = entry->name();

		// Remove common extensions
		// (we want to keep other extensions, eg. TEXTURES.floors)
		if (strutil::endsWithCI(name, ".txt") || strutil::endsWithCI(name, ".lmp"))
			name = name.substr(0, name.size() - 4);

		return name;
	}

	return {};
}

string TextureEditor::textureListName(const TextureXList& list) const
{
	for (unsigned i = 0; i < texturex_entries_.size(); ++i)
		if (texturex_entries_[i].texturex.get() == &list)
			return textureListName(i);

	return {};
}

bool TextureEditor::textureListModified(unsigned index) const
{
	auto list = textureList(index);
	if (!list)
		return false;

	for (unsigned i = 0; i < list->size(); ++i)
		if (list->texture(i)->state() == CTexture::State::Modified)
			return true;

	return false;
}

void TextureEditor::openTexture(CTexture& texture)
{
	tex_current_ = &texture;

	setupTextureBackup(texture);

	selected_patches_.clear();
	undo_manager_->setResetPoint();
}

void TextureEditor::closeTexture()
{
	tex_current_ = nullptr;
	selected_patches_.clear();
}

void TextureEditor::revertTexture()
{
	if (!tex_current_)
		return;

	if (auto backup = getTextureBackup(*tex_current_))
	{
		undo_manager_->beginRecord(fmt::format("{}: Revert", tex_current_->name()));
		undo_manager_->recordUndoStep<TextureModificationUS>(*this, *tex_current_);

		tex_current_->copyTexture(*backup);
		tex_current_->setState(backup->state());
		selected_patches_.clear();
		signals_.current_texture_modified(true, true);

		undo_manager_->endRecord(true);
	}
	else
		log::warning("No backup found for texture {}, unable to revert", tex_current_->name());
}

void TextureEditor::selectPatch(unsigned index, bool selected)
{
	if (selected)
		vectorAddUnique(selected_patches_, index);
	else
		vectorRemoveVal(selected_patches_, index);
}

bool TextureEditor::undo() const
{
	return !undo_manager_->undo().empty();
}

bool TextureEditor::redo() const
{
	return !undo_manager_->redo().empty();
}

string TextureEditor::importPatchFile(string_view filename, bool add_to_patch_table) const
{
	// Load the file into a temporary ArchiveEntry
	auto entry = std::make_shared<ArchiveEntry>();
	entry->importFile(filename);

	// Determine type
	EntryType::detectEntryType(*entry);

	// If it's not a valid image type, don't add it
	if (!entry->type()->extraProps().contains("image"))
	{
		log::warning("{} is not a valid image file", filename);
		return {};
	}

	// Ask for name for texture
	auto file_name = strutil::Path::fileNameOf(filename);
	auto tex_name  = strutil::upper(strutil::truncate(file_name, 8));
	auto name      = wxGetTextFromUser(
                    WX_FMT("Enter a texture name for {}:", file_name), wxS("New Texture"), wxString::FromUTF8(tex_name))
					.Truncate(8)
					.utf8_string();

	// Add patch to archive
	entry->setName(name);
	entry->setExtensionByType();
	archive_->addEntry(entry, "patches");

	// Add patch to patch table if needed
	// TODO: Abort if patch already exists in table
	if (add_to_patch_table)
		patch_table_->addPatch(name);

	return name;
}

void TextureEditor::newTexture(
	TextureXList* list,
	string_view   name,
	int           index,
	int           width,
	int           height,
	string_view   patch) const
{
	// Process name
	string tex_name{ name };
	if (list->format() != TextureXList::Format::Textures)
	{
		strutil::upperIP(tex_name);
		strutil::truncateIP(tex_name, 8);
	}

	// Create new texture
	auto tex = std::make_unique<CTexture>(tex_name);
	tex->setState(CTexture::State::New);

	// Setup texture scale
	if (list->format() == TextureXList::Format::Textures)
	{
		tex->setScale({ 1., 1. });
		tex->setExtended(true);
	}
	else
		tex->setScale({ 0., 0. });

	// Add patch if specified
	if (!patch.empty())
	{
		tex->addPatch(patch);
		patch_table_->updatePatchUsage(tex.get());

		// Load patch image (to determine dimensions)
		SImage image;
		tex->loadPatchImage(0, image);
		width  = image.width();
		height = image.height();
	}

	// Set texture size
	tex->setWidth(width);
	tex->setHeight(height);

	auto undo_recording = undo_manager_->currentlyRecording();
	if (!undo_recording)
		undo_manager_->beginRecord(fmt::format("New Texture: {}", tex_name));

	// Add it to the list
	auto added_tex = tex.get();
	list->addTexture(std::move(tex), index);
	signals_.texture_added(list, added_tex);

	undo_manager_->recordUndoStep<TextureCreateDeleteUS>(*this, list, added_tex->index());

	if (!undo_recording)
		undo_manager_->endRecord(true);
}

void TextureEditor::deleteTextures(const vector<CTexture*>& textures) const
{
	if (textures.empty())
		return;

	auto undo_recording = undo_manager_->currentlyRecording();
	if (!undo_recording)
	{
		if (textures.size() == 1)
			undo_manager_->beginRecord(fmt::format("Delete Texture: {}", textures[0]->name()));
		else
			undo_manager_->beginRecord(fmt::format("Delete {} Textures", textures.size()));
	}

	bool any_deleted = false;
	for (auto texture : textures)
	{
		auto list  = texture->list();
		auto index = texture->index();
		if (index < 0 || !list)
			continue;

		auto removed = list->removeTexture(index);
		signals_.texture_deleted(list, removed.get());
		undo_manager_->recordUndoStep<TextureCreateDeleteUS>(*this, list, std::move(removed), index);
		any_deleted = true;
	}

	if (!undo_recording)
		undo_manager_->endRecord(any_deleted);
}

void TextureEditor::moveTextures(const vector<CTexture*>& textures, Direction direction) const
{
	// Sort in ascending/descending index order depending on direction
	auto sorted = textures;
	if (direction == Direction::Up)
		std::ranges::sort(sorted, [](const CTexture* a, const CTexture* b) { return a->index() < b->index(); });
	else
		std::ranges::sort(sorted, [](const CTexture* a, const CTexture* b) { return a->index() > b->index(); });

	auto undo_recording = undo_manager_->currentlyRecording();
	if (!undo_recording)
		undo_manager_->beginRecord(fmt::format("Move Texture {}", direction == Direction::Up ? "Up" : "Down"));

	for (auto texture : sorted)
	{
		auto list  = texture->list();
		auto index = texture->index();
		if (!list || index < 0)
			return;

		int new_index = direction == Direction::Up ? index - 1 : index + 1;
		if (new_index < 0 || std::cmp_greater_equal(new_index, list->size()))
			return;

		list->swapTextures(index, new_index);

		signals_.textures_swapped(list, index, new_index);

		undo_manager_->recordUndoStep<TextureSwapUS>(*this, *list, index, new_index);
	}

	if (!undo_recording)
		undo_manager_->endRecord(true);
}

void TextureEditor::sortTextures(const vector<CTexture*>& textures) const
{
	if (textures.empty())
		return;

	auto list = textures[0]->list();
	if (!list)
		return;

	// Find first and last indices of the textures to sort
	unsigned first = textures[0]->index();
	unsigned last  = first;
	for (auto tex : textures)
	{
		if (tex->list() != list)
			return; // All textures must be from the same list

		auto index = tex->index();
		if (index < 0)
			return; // Invalid texture index

		first = std::cmp_less(first, index) ? first : index;
		last  = std::cmp_greater(last, index) ? last : index;
	}

	// Backup pre-sorted texture order to determine index swaps for undo step
	vector<CTexture*> before_order;
	before_order.reserve(last - first + 1);
	for (unsigned i = first; i <= last; ++i)
		before_order.push_back(list->texture(i));

	auto undo_recording = undo_manager_->currentlyRecording();
	if (!undo_recording)
		undo_manager_->beginRecord("Sort Textures");

	list->sortTextures(first, last);

	// Build map of swapped texture indices for undo step
	std::map<unsigned, unsigned> index_swaps;
	for (unsigned new_index = first; new_index <= last; ++new_index)
	{
		auto tex = list->texture(new_index);
		auto it  = std::ranges::find(before_order, tex);
		if (it == before_order.end())
			return;

		auto old_index = first + static_cast<unsigned>(std::distance(before_order.begin(), it));
		if (old_index != new_index)
			index_swaps[old_index] = new_index;
	}

	bool any_swaps = false;
	if (!index_swaps.empty())
	{
		undo_manager_->recordUndoStep<TextureListReorderUS>(*this, *list, first, last, std::move(index_swaps));
		any_swaps = true;
	}

	if (!undo_recording)
		undo_manager_->endRecord(any_swaps);

	signals_.textures_modified(textures);
}

void TextureEditor::renameTextures(const vector<CTexture*>& textures, bool each) const
{
	auto wad_force_uppercase = CVar::getBool("wad_force_uppercase");

	// Define alphabet
	static constexpr string_view alphabet       = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	static constexpr string_view alphabet_lower = "abcdefghijklmnopqrstuvwxyz";

	// Check any are selected
	if (each || textures.size() == 1)
	{
		bool renamed = false;

		// Begin undo level here if multiple textures are being renamed
		if (textures.size() > 1)
			undo_manager_->beginRecord("Rename Textures");

		// If only one entry is selected, or "rename each" mode is desired, just do basic rename
		for (auto texture : textures)
		{
			// Prompt for a new name
			auto new_name = wxGetTextFromUser(
								wxS("Enter new texture name: (* = unchanged)"),
								wxS("Rename"),
								wxString::FromUTF8(texture->name()))
								.utf8_string();
			if (wad_force_uppercase)
				strutil::upperIP(new_name);

			// Rename entry (if needed)
			if (!new_name.empty() && texture->name() != new_name)
			{
				// Begin undo level if single texture rename
				if (textures.size() == 1)
					undo_manager_->beginRecord(fmt::format("Rename Texture: {} -> {}", texture->name(), new_name));

				// Record undo steps for name and state
				undo_manager_->recordUndoStep<TexturePropertyChangeUS>(*this, *texture, "name", texture->name());
				if (texture->state() == CTexture::State::Unmodified)
					undo_manager_->recordUndoStep<TexturePropertyChangeUS>(
						*this, *texture, "state", static_cast<int>(texture->state()));

				texture->setName(new_name);
				texture->setState(CTexture::State::Modified);

				renamed = true;
			}
		}

		undo_manager_->endRecord(renamed);
	}
	else if (textures.size() > 1)
	{
		// Get a list of entry names
		vector<string> names;
		names.reserve(textures.size());
		for (auto& texture : textures)
			names.push_back(texture->name());

		// Get filter string
		auto filter = misc::massRenameFilter(names);

		// Prompt for a new name
		auto new_name = wxGetTextFromUser(
							wxS("Enter new texture name: (* = unchanged, ^ = alphabet letter, ^^ = lower case\n")
							"% = alphabet repeat number, & = texture number, %% or && = n-1)",
							wxS("Rename"),
							wxString::FromUTF8(filter))
							.utf8_string();
		if (wad_force_uppercase)
			strutil::upperIP(new_name);

		// Apply mass rename to list of names
		if (!new_name.empty())
		{
			undo_manager_->beginRecord("Rename Textures");

			misc::doMassRename(names, new_name);

			// Go through the list
			bool renamed = false;
			for (size_t a = 0; a < textures.size(); a++)
			{
				// Rename the entry (if needed)
				if (textures[a]->name() != names[a])
				{
					auto filename = names[a];
					int  num      = a / alphabet.size();
					int  cn       = a - (num * alphabet.size());
					strutil::replaceIP(filename, "^^", { alphabet_lower.data() + cn, 1 });
					strutil::replaceIP(filename, "^", { alphabet.data() + cn, 1 });
					strutil::replaceIP(filename, "%%", fmt::format("{}", num));
					strutil::replaceIP(filename, "%", fmt::format("{}", num + 1));
					strutil::replaceIP(filename, "&&", fmt::format("{}", a));
					strutil::replaceIP(filename, "&", fmt::format("{}", a + 1));

					// Record undo steps for name and state
					undo_manager_->recordUndoStep<TexturePropertyChangeUS>(
						*this, *textures[a], "name", textures[a]->name());
					if (textures[a]->state() == CTexture::State::Unmodified)
						undo_manager_->recordUndoStep<TexturePropertyChangeUS>(
							*this, *textures[a], "state", static_cast<int>(textures[a]->state()));

					textures[a]->setName(filename);
					textures[a]->setState(CTexture::State::Modified);

					renamed = true;
				}
			}

			undo_manager_->endRecord(renamed);
		}
	}

	if (!textures.empty())
		signals_.textures_modified(textures);
}

bool TextureEditor::exportAsPNG(const CTexture& texture, string_view filename, const Palette* palette, bool force_rgba)
{
	// Create image from entry
	SImage image;
	if (!texture.toImage(image, nullptr, palette, force_rgba))
	{
		log::error("Error converting {}: {}", texture.name(), global::error);
		return false;
	}

	// Write png data
	MemChunk png;
	auto     fmt_png = SIFormat::getFormat("png");
	if (!fmt_png->saveImage(image, png, palette))
	{
		log::error("Error converting {}", texture.name());
		return false;
	}

	// Export file
	return png.exportFile(filename);
}

void TextureEditor::setTextureSize(int width, int height) const
{
	if (!tex_current_)
		return;

	undo_manager_->beginRecord(fmt::format("{}: Resize Texture", tex_current_->name()));

	if (width > 0)
	{
		undo_manager_->recordUndoStep<TexturePropertyChangeUS>(
			*this, *tex_current_, "width", static_cast<unsigned>(tex_current_->width()));
		tex_current_->setWidth(width);
	}
	if (height > 0)
	{
		undo_manager_->recordUndoStep<TexturePropertyChangeUS>(
			*this, *tex_current_, "height", static_cast<unsigned>(tex_current_->height()));
		tex_current_->setHeight(height);
	}

	signalCurrentTextureModified(true, false, false);

	undo_manager_->endRecord(true);
}

void TextureEditor::setTextureScale(optional<double> x, optional<double> y) const
{
	if (!tex_current_)
		return;

	undo_manager_->beginRecord(fmt::format("{}: Change Scale", tex_current_->name()));

	if (x.has_value())
	{
		undo_manager_->recordUndoStep<TexturePropertyChangeUS>(*this, *tex_current_, "scale_x", tex_current_->scaleX());
		tex_current_->setScaleX(x.value());
	}
	if (y.has_value())
	{
		undo_manager_->recordUndoStep<TexturePropertyChangeUS>(*this, *tex_current_, "scale_y", tex_current_->scaleY());
		tex_current_->setScaleY(y.value());
	}

	signalCurrentTextureModified(true, false, false);

	undo_manager_->endRecord(true);
}

void TextureEditor::setTextureFlag(CTexture::Flag flag, bool on) const
{
	if (!tex_current_)
		return;

	auto prev = tex_current_->setFlag(flag, on);

	if (prev != on)
	{
		string flag_name;
		switch (flag)
		{
		case CTexture::Flag::WorldPanning: flag_name = "worldpanning"; break;
		case CTexture::Flag::Optional:     flag_name = "optional"; break;
		case CTexture::Flag::NoDecals:     flag_name = "nodecals"; break;
		case CTexture::Flag::NullTexture:  flag_name = "nulltexture"; break;
		case CTexture::Flag::NoTrim:       flag_name = "notrim"; break;
		default:                           return;
		}

		undo_manager_->beginRecord(fmt::format("{}: \"{}\" {}", tex_current_->name(), flag_name, on ? "ON" : "OFF"));
		undo_manager_->recordUndoStep<TexturePropertyChangeUS>(*this, *tex_current_, flag_name, prev);
		signalCurrentTextureModified(false, false, false);
		undo_manager_->endRecord(true);
	}
}

void TextureEditor::setTextureType(CTexture::Type type) const
{
	if (!tex_current_)
		return;

	undo_manager_->beginRecord(fmt::format("{}: Change Type", tex_current_->name()));
	undo_manager_->recordUndoStep<TexturePropertyChangeUS>(*this, *tex_current_, "type", tex_current_->type());
	tex_current_->setType(type);
	signalCurrentTextureModified(false, false, false);
	undo_manager_->endRecord(true);
}

void TextureEditor::setTextureOffset(optional<int> x, optional<int> y) const
{
	if (!tex_current_)
		return;

	undo_manager_->beginRecord(fmt::format("{}: Change Offsets", tex_current_->name()));

	if (x.has_value())
	{
		undo_manager_->recordUndoStep<TexturePropertyChangeUS>(
			*this, *tex_current_, "offset_x", tex_current_->offsetX());
		tex_current_->setOffsetX(x.value());
	}
	if (y.has_value())
	{
		undo_manager_->recordUndoStep<TexturePropertyChangeUS>(
			*this, *tex_current_, "offset_y", tex_current_->offsetY());
		tex_current_->setOffsetY(y.value());
	}

	signalCurrentTextureModified(true, false, false);

	undo_manager_->endRecord(true);
}

void TextureEditor::setPatchOffset(optional<int> x, optional<int> y) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Change Patch Offsets", tex_current_->name()));

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
		{
			if (x.has_value())
			{
				undo_manager_->recordUndoStep<PatchPropertyChangeUS>(
					*this, *tex_current_, index, "offset_x", patch->offset().x);
				patch->setOffsetX(x.value());
			}
			if (y.has_value())
			{
				undo_manager_->recordUndoStep<PatchPropertyChangeUS>(
					*this, *tex_current_, index, "offset_y", patch->offset().y);
				patch->setOffsetY(y.value());
			}
		}

	signalCurrentTextureModified(false, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::movePatch(const Vec2i& offset) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Move Patch(es)", tex_current_->name()));
	undo_manager_->recordUndoStep<PatchMoveUS>(*this, *tex_current_, selected_patches_, offset);

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
		{
			patch->setOffsetX(patch->offset().x + offset.x);
			patch->setOffsetY(patch->offset().y + offset.y);
		}

	signalCurrentTextureModified(true, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::setPatchBlendType(CTPatchEx::BlendType type) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Change Patch Blend Type", tex_current_->name()));

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
			{
				undo_manager_->recordUndoStep<PatchPropertyChangeUS>(
					*this, *tex_current_, index, "blend_type", static_cast<int>(patch_ex->blendType()));
				patch_ex->setBlendType(type);
			}

	signalCurrentTextureModified(true, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::setPatchFlag(string_view flag, bool on) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Patch \"{}\" {}", tex_current_->name(), flag, on ? "ON" : "OFF"));

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
			{
				undo_manager_->recordUndoStep<PatchPropertyChangeUS>(
					*this, *tex_current_, index, flag, patch_ex->hasFlag(flag));
				patch_ex->setFlag(flag, on);
			}

	signalCurrentTextureModified(false, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::setPatchRotation(int rotation) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Change Patch Rotation", tex_current_->name()));

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
			{
				undo_manager_->recordUndoStep<PatchPropertyChangeUS>(
					*this, *tex_current_, index, "rotation", static_cast<int>(patch_ex->rotation()));
				patch_ex->setRotation(rotation);
			}

	signalCurrentTextureModified(true, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::setPatchAlpha(double alpha) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Change Patch Alpha", tex_current_->name()));

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
			{
				undo_manager_->recordUndoStep<PatchPropertyChangeUS>(
					*this, *tex_current_, index, "alpha", static_cast<double>(patch_ex->alpha()));
				patch_ex->setAlpha(alpha);
			}

	signalCurrentTextureModified(true, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::setPatchAlphaStyle(string_view style) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Change Patch Alpha Style", tex_current_->name()));

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
			{
				undo_manager_->recordUndoStep<PatchPropertyChangeUS>(
					*this, *tex_current_, index, "style", patch_ex->style());
				patch_ex->setStyle(style);
			}

	signalCurrentTextureModified(true, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::setPatchColour(const ColRGBA& colour) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Change Patch Colour", tex_current_->name()));

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
			{
				undo_manager_->recordUndoStep<PatchPropertyChangeUS>(
					*this, *tex_current_, index, "colour", colour::toInt(patch_ex->colour()));
				patch_ex->setColour(colour);
			}

	signalCurrentTextureModified(true, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::setPatchTintAmount(double amount) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Change Patch Tint Amount", tex_current_->name()));

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
			{
				undo_manager_->recordUndoStep<PatchPropertyChangeUS>(
					*this, *tex_current_, index, "tint_amount", patch_ex->tintAmount());
				patch_ex->setTintAmount(amount);
			}

	signalCurrentTextureModified(true, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::setPatchTranslation(string_view translation) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	Translation t;
	if (!translation.empty())
		t.parse(translation);

	undo_manager_->beginRecord(fmt::format("{}: Change Patch Translation", tex_current_->name()));

	for (unsigned index : selected_patches_)
		if (auto patch = tex_current_->patch(index))
			if (auto patch_ex = dynamic_cast<CTPatchEx*>(patch))
			{
				undo_manager_->recordUndoStep<PatchPropertyChangeUS>(
					*this,
					*tex_current_,
					index,
					"translation",
					patch_ex->translation() ? patch_ex->translation()->asText() : "");
				patch_ex->setTranslation(t);
			}

	signalCurrentTextureModified(true, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::addPatch(string_view patch) const
{
	if (!tex_current_)
		return;

	undo_manager_->beginRecord(fmt::format("{}: Add Patch", tex_current_->name()));
	undo_manager_->recordUndoStep<TexturePatchListChangeUS>(*this, *tex_current_);
	tex_current_->addPatch(patch);
	signalCurrentTextureModified(true, true, false);
	undo_manager_->endRecord(true);
}

void TextureEditor::removePatch()
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	// Clear selection
	auto to_remove = selected_patches_;
	selected_patches_.clear();

	// Remove patches
	undo_manager_->beginRecord(fmt::format("{}: Remove Patch", tex_current_->name()));
	undo_manager_->recordUndoStep<TexturePatchListChangeUS>(*this, *tex_current_);
	tex_current_->removePatches(to_remove);
	signalCurrentTextureModified(true, true, false);
	undo_manager_->endRecord(true);
}

void TextureEditor::replacePatch(string_view patch) const
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Replace Patch", tex_current_->name()));
	undo_manager_->recordUndoStep<TexturePatchListChangeUS>(*this, *tex_current_);

	if (selected_patches_.size() == 1)
		tex_current_->replacePatch(selected_patches_[0], patch);
	else
		tex_current_->replacePatches(selected_patches_, patch);

	signalCurrentTextureModified(true, false, true);

	undo_manager_->endRecord(true);
}

void TextureEditor::duplicatePatch(int xoff, int yoff)
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	undo_manager_->beginRecord(fmt::format("{}: Duplicate Patch", tex_current_->name()));
	undo_manager_->recordUndoStep<TexturePatchListChangeUS>(*this, *tex_current_);

	if (selected_patches_.size() == 1)
		tex_current_->duplicatePatch(selected_patches_[0], xoff, yoff);
	else
		tex_current_->duplicatePatches(selected_patches_, xoff, yoff);

	// Adjust selection to select the new duplicated patches
	for (unsigned& index : selected_patches_)
		index++;

	signalCurrentTextureModified(true, true, false);

	undo_manager_->endRecord(true);
}

void TextureEditor::patchForward()
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	if (vectorContains(selected_patches_, static_cast<unsigned>(tex_current_->nPatches() - 1)))
		return; // Can't move if last patch is selected

	undo_manager_->beginRecord(fmt::format("{}: Move Patch Forward", tex_current_->name()));
	undo_manager_->recordUndoStep<TexturePatchListChangeUS>(*this, *tex_current_);

	if (selected_patches_.size() == 1)
		tex_current_->swapPatches(selected_patches_[0], selected_patches_[0] + 1);
	else
	{
		// Sort selection in descending order so we can safely swap patches in order
		auto sorted_selection = selected_patches_;
		std::ranges::sort(sorted_selection, std::greater());

		// Swap selected patches forward
		for (unsigned& index : sorted_selection)
			tex_current_->swapPatches(index, index + 1);
	}

	// Update selection to match new patch positions
	for (unsigned& index : selected_patches_)
		index++;

	signalCurrentTextureModified(true, true, false);

	undo_manager_->endRecord(true);
}

void TextureEditor::patchBack()
{
	if (!tex_current_ || selected_patches_.empty())
		return;

	if (vectorContains(selected_patches_, 0u))
		return; // Can't move if first patch is selected

	undo_manager_->beginRecord(fmt::format("{}: Move Patch Back", tex_current_->name()));
	undo_manager_->recordUndoStep<TexturePatchListChangeUS>(*this, *tex_current_);

	if (selected_patches_.size() == 1)
		tex_current_->swapPatches(selected_patches_[0], selected_patches_[0] - 1);
	else
	{
		// Sort selection in ascending order so we can safely swap patches in order
		auto sorted_selection = selected_patches_;
		std::ranges::sort(sorted_selection);

		// Swap selected patches backward
		for (unsigned index : sorted_selection)
			tex_current_->swapPatches(index, index - 1);
	}

	// Update selection to match new patch positions
	for (unsigned& index : selected_patches_)
		index--;

	signalCurrentTextureModified(true, true, false);

	undo_manager_->endRecord(true);
}

void TextureEditor::signalCurrentTextureModified(bool texture, bool patch_list, bool patches) const
{
	// Record undo step for texture state change if needed
	if (undo_manager_->currentlyRecording() && tex_current_->state() != CTexture::State::Modified)
		undo_manager_->recordUndoStep<TexturePropertyChangeUS>(
			*this, *tex_current_, "state", static_cast<int>(tex_current_->state()));

	// Mark texture as modified
	tex_current_->setState(CTexture::State::Modified);

	// Signal modification
	if (patches)
		signals_.patches_modified(selected_patches_);
	signals_.current_texture_modified(texture, patch_list);
}

void TextureEditor::setupTextureBackup(const CTexture& texture)
{
	for (auto& tx : texturex_entries_)
	{
		if (tx.texturex.get() == texture.list())
		{
			// Check if we already have a backup for this texture
			if (tx.backup_textures.contains(&texture))
				return;

			// Create a backup of the texture
			tx.backup_textures[&texture] = std::make_unique<CTexture>();
			tx.backup_textures[&texture]->copyTexture(texture);
		}
	}
}

CTexture* TextureEditor::getTextureBackup(const CTexture& texture) const
{
	for (const auto& tx : texturex_entries_)
	{
		if (tx.texturex.get() == texture.list())
		{
			auto it = tx.backup_textures.find(&texture);
			if (it != tx.backup_textures.end())
				return it->second.get();
		}
	}

	return nullptr;
}

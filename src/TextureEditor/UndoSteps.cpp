
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    UndoSteps.cpp
// Description: Various texture editor related UndoSteps
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "UndoSteps.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Graphics/Translation.h"
#include "TextureEditor.h"
#include "Utility/Colour.h"
#include "Utility/PropertyList.h"

using namespace slade;
using namespace texeditor;


// -----------------------------------------------------------------------------
//
// TexturePropertyChangeUS Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TexturePropertyChangeUS class constructor
// -----------------------------------------------------------------------------
TexturePropertyChangeUS::TexturePropertyChangeUS(
	const TextureEditor& editor,
	const CTexture&      texture,
	string_view          property,
	const Property&      prev_value) :
	property_{ property },
	prev_value_{ prev_value },
	tx_list_{ texture.list() },
	index_{ texture.index() },
	editor_{ &editor }
{
}

// -----------------------------------------------------------------------------
// Swaps the texture property value with the previous value
// -----------------------------------------------------------------------------
bool TexturePropertyChangeUS::swapValue()
{
	Property prev_value;
	auto     texture = tx_list_->texture(index_);

	if (property_ == "state")
	{
		prev_value = static_cast<int>(texture->state());
		texture->setState(static_cast<CTexture::State>(std::get<int>(prev_value_)));
	}
	else if (property_ == "width")
	{
		prev_value = static_cast<unsigned>(texture->width());
		texture->setWidth(std::get<unsigned>(prev_value_));
	}
	else if (property_ == "height")
	{
		prev_value = static_cast<unsigned>(texture->height());
		texture->setHeight(std::get<unsigned>(prev_value_));
	}
	else if (property_ == "scale_x")
	{
		prev_value = texture->scaleX();
		texture->setScaleX(std::get<double>(prev_value_));
	}
	else if (property_ == "scale_y")
	{
		prev_value = texture->scaleY();
		texture->setScaleY(std::get<double>(prev_value_));
	}
	else if (property_ == "worldpanning")
		prev_value = texture->setFlag(CTexture::Flag::WorldPanning, std::get<bool>(prev_value_));
	else if (property_ == "optional")
		prev_value = texture->setFlag(CTexture::Flag::Optional, std::get<bool>(prev_value_));
	else if (property_ == "nodecals")
		prev_value = texture->setFlag(CTexture::Flag::NoDecals, std::get<bool>(prev_value_));
	else if (property_ == "nulltexture")
		prev_value = texture->setFlag(CTexture::Flag::NullTexture, std::get<bool>(prev_value_));
	else if (property_ == "notrim")
		prev_value = texture->setFlag(CTexture::Flag::NoTrim, std::get<bool>(prev_value_));
	else if (property_ == "type")
	{
		prev_value = texture->type();
		texture->setType(std::get<string>(prev_value_));
	}
	else if (property_ == "offset_x")
	{
		prev_value = static_cast<int>(texture->offsetX());
		texture->setOffsetX(std::get<int>(prev_value_));
	}
	else if (property_ == "offset_y")
	{
		prev_value = static_cast<int>(texture->offsetY());
		texture->setOffsetY(std::get<int>(prev_value_));
	}
	else if (property_ == "name")
	{
		prev_value = texture->name();
		texture->setName(std::get<string>(prev_value_));
	}
	else
		return false;

	prev_value_ = prev_value;

	if (texture == editor_->currentTexture())
		editor_->signals().current_texture_modified(true, false);
	else
		editor_->signals().texture_modified(texture);

	return true;
}


// -----------------------------------------------------------------------------
//
// TexturePatchListChangeUS Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TexturePatchListChangeUS class constructor
// -----------------------------------------------------------------------------
TexturePatchListChangeUS::TexturePatchListChangeUS(const TextureEditor& editor, const CTexture& texture) :
	tx_list_{ texture.list() },
	index_{ texture.index() },
	editor_{ &editor }
{
	if (texture.isExtended())
	{
		for (auto& patch : texture.patches())
			patches_.push_back(std::make_unique<CTPatchEx>(dynamic_cast<const CTPatchEx&>(*patch)));
	}
	else
	{
		for (auto& patch : texture.patches())
			patches_.push_back(std::make_unique<CTPatch>(*patch));
	}
}

// -----------------------------------------------------------------------------
// Swaps the texture's patch list with the previous list
// -----------------------------------------------------------------------------
bool TexturePatchListChangeUS::swapLists()
{
	auto tex = tx_list_->texture(index_);
	if (!tex)
		return false;

	if (tex->replacePatches(patches_))
	{
		if (editor_->hasPatchTable())
			editor_->patchTable()->updatePatchUsage(tex);

		return true;
	}

	return false;
}


// -----------------------------------------------------------------------------
//
// PatchPropertyChangeUS Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PatchPropertyChangeUS class constructor
// -----------------------------------------------------------------------------
PatchPropertyChangeUS::PatchPropertyChangeUS(
	const TextureEditor& editor,
	const CTexture&      texture,
	int                  patch_index,
	string_view          property,
	const Property&      prev_value) :
	TexturePropertyChangeUS(editor, texture, property, prev_value),
	patch_index_{ patch_index }
{
}

// -----------------------------------------------------------------------------
// Swaps the patch property value with the previous value
// -----------------------------------------------------------------------------
bool PatchPropertyChangeUS::swapValue()
{
	Property prev_value;
	auto     texture = tx_list_->texture(index_);
	if (!texture)
		return false;

	if (patch_index_ < 0 || std::cmp_greater_equal(patch_index_, texture->nPatches()))
		return false;

	auto patch    = texture->patch(patch_index_);
	auto patch_ex = dynamic_cast<CTPatchEx*>(patch);

	if (property_ == "offset_x")
	{
		prev_value = static_cast<int>(patch->offset().x);
		patch->setOffsetX(std::get<int>(prev_value_));
	}
	else if (property_ == "offset_y")
	{
		prev_value = static_cast<int>(patch->offset().y);
		patch->setOffsetY(std::get<int>(prev_value_));
	}
	else if (patch_ex && property_ == "blend_type")
	{
		prev_value = static_cast<int>(patch_ex->blendType());
		patch_ex->setBlendType(static_cast<CTPatchEx::BlendType>(std::get<int>(prev_value_)));
	}
	else if (patch_ex && strutil::equalCI(property_, "FlipX"))
	{
		prev_value = patch_ex->flipX();
		patch_ex->setFlipX(std::get<bool>(prev_value_));
	}
	else if (patch_ex && strutil::equalCI(property_, "FlipY"))
	{
		prev_value = patch_ex->flipY();
		patch_ex->setFlipY(std::get<bool>(prev_value_));
	}
	else if (patch_ex && strutil::equalCI(property_, "UseOffsets"))
	{
		prev_value = patch_ex->useOffsets();
		patch_ex->setUseOffsets(std::get<bool>(prev_value_));
	}
	else if (patch_ex && property_ == "rotation")
	{
		prev_value = static_cast<int>(patch_ex->rotation());
		patch_ex->setRotation(static_cast<int16_t>(std::get<int>(prev_value_)));
	}
	else if (patch_ex && property_ == "alpha")
	{
		prev_value = patch_ex->alpha();
		patch_ex->setAlpha(std::get<double>(prev_value_));
	}
	else if (patch_ex && property_ == "style")
	{
		prev_value = patch_ex->style();
		patch_ex->setStyle(std::get<string>(prev_value_));
	}
	else if (patch_ex && property_ == "colour")
	{
		prev_value = colour::toInt(patch_ex->colour());
		patch_ex->setColour(colour::fromInt(std::get<int>(prev_value_)));
	}
	else if (patch_ex && property_ == "tint_amount")
	{
		prev_value = patch_ex->tintAmount();
		patch_ex->setTintAmount(std::get<double>(prev_value_));
	}
	else if (patch_ex && property_ == "translation")
	{
		prev_value = patch_ex->translation()->asText();
		patch_ex->setTranslation(std::get<string>(prev_value_));
	}
	else
		return false;

	prev_value_ = prev_value;

	if (texture == editor_->currentTexture())
		editor_->signals().patches_modified({ static_cast<unsigned>(patch_index_) });
	else
		editor_->signals().texture_modified(texture);

	return true;
}


// -----------------------------------------------------------------------------
//
// PatchMoveUS Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PatchMoveUS class constructor
// -----------------------------------------------------------------------------
PatchMoveUS::PatchMoveUS(
	const TextureEditor&    editor,
	const CTexture&         texture,
	const vector<unsigned>& patch_indices,
	const Vec2i&            offset) :
	editor_{ &editor },
	tx_list_{ texture.list() },
	index_{ texture.index() },
	patch_indices_{ patch_indices },
	offset_{ offset }
{
}

// -----------------------------------------------------------------------------
// Undoes the patch move operation
// -----------------------------------------------------------------------------
bool PatchMoveUS::doUndo()
{
	auto texture = tx_list_->texture(index_);
	if (!texture)
		return false;

	for (auto index : patch_indices_)
		if (auto patch = texture->patch(index))
		{
			patch->setOffsetX(patch->offset().x - offset_.x);
			patch->setOffsetY(patch->offset().y - offset_.y);
		}

	if (texture == editor_->currentTexture())
		editor_->signals().patches_modified(patch_indices_);
	else
		editor_->signals().texture_modified(texture);

	return true;
}

// -----------------------------------------------------------------------------
// Redoes the patch move operation
// -----------------------------------------------------------------------------
bool PatchMoveUS::doRedo()
{
	auto texture = tx_list_->texture(index_);
	if (!texture)
		return false;

	for (auto index : patch_indices_)
		if (auto patch = texture->patch(index))
		{
			patch->setOffsetX(patch->offset().x + offset_.x);
			patch->setOffsetY(patch->offset().y + offset_.y);
		}

	if (texture == editor_->currentTexture())
		editor_->signals().patches_modified(patch_indices_);
	else
		editor_->signals().texture_modified(texture);

	return true;
}


// -----------------------------------------------------------------------------
//
// TextureCreateDeleteUS Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureCreateDeleteUS class constructor (creation)
// -----------------------------------------------------------------------------
TextureCreateDeleteUS::TextureCreateDeleteUS(const TextureEditor& editor, TextureXList* list, int created_index) :
	editor_{ &editor },
	tx_list_{ list },
	index_{ created_index }
{
}

// -----------------------------------------------------------------------------
// TextureCreateDeleteUS class constructor (deletion)
// -----------------------------------------------------------------------------
TextureCreateDeleteUS::TextureCreateDeleteUS(
	const TextureEditor& editor,
	TextureXList*        list,
	unique_ptr<CTexture> tex_removed,
	int                  removed_index) :
	editor_{ &editor },
	tx_list_{ list },
	tex_removed_{ std::move(tex_removed) },
	index_{ removed_index },
	created_{ false }
{
}

// -----------------------------------------------------------------------------
// TextureCreateDeleteUS class destructor
// -----------------------------------------------------------------------------
TextureCreateDeleteUS::~TextureCreateDeleteUS() = default;

// -----------------------------------------------------------------------------
// Deletes the texture from the relevant list
// -----------------------------------------------------------------------------
bool TextureCreateDeleteUS::deleteTexture()
{
	tex_removed_ = tx_list_->removeTexture(index_);
	editor_->signals().texture_deleted(tx_list_, tex_removed_.get());
	return true;
}

// -----------------------------------------------------------------------------
// Creates the texture in the relevant list
// -----------------------------------------------------------------------------
bool TextureCreateDeleteUS::createTexture()
{
	tx_list_->addTexture(std::move(tex_removed_), index_);
	editor_->signals().texture_added(tx_list_, tx_list_->texture(index_));
	return true;
}

// -----------------------------------------------------------------------------
// Undoes the texture creation/deletion operation
// -----------------------------------------------------------------------------
bool TextureCreateDeleteUS::doUndo()
{
	if (created_)
		return deleteTexture();
	else
		return createTexture();
}

// -----------------------------------------------------------------------------
// Redoes the texture creation/deletion operation
// -----------------------------------------------------------------------------
bool TextureCreateDeleteUS::doRedo()
{
	if (!created_)
		return deleteTexture();
	else
		return createTexture();
}


// -----------------------------------------------------------------------------
//
// TextureModificationUS Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureModificationUS class constructor
// -----------------------------------------------------------------------------
TextureModificationUS::TextureModificationUS(TextureEditor& editor, const CTexture& texture) :
	tx_list_{ texture.list() },
	tex_copy_{ std::make_unique<CTexture>() },
	index_{ texture.index() },
	editor_{ &editor }
{
	tex_copy_->copyTexture(texture);
}

// -----------------------------------------------------------------------------
// TextureModificationUS class destructor
// -----------------------------------------------------------------------------
TextureModificationUS::~TextureModificationUS() = default;

// -----------------------------------------------------------------------------
// Swaps the texture data with the previous data
// -----------------------------------------------------------------------------
bool TextureModificationUS::swapData() const
{
	auto tex = tx_list_->texture(index_);
	if (!tex)
		return false;

	CTexture temp;
	temp.copyTexture(*tex);
	tex->copyTexture(*tex_copy_);
	tex_copy_->copyTexture(temp);

	if (tex == editor_->currentTexture())
		editor_->signals().current_texture_modified(true, true);
	else
		editor_->signals().texture_modified(tex);

	return true;
}


// -----------------------------------------------------------------------------
//
// TextureSwapUS Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureSwapUS class constructor
// -----------------------------------------------------------------------------
TextureSwapUS::TextureSwapUS(const TextureEditor& editor, TextureXList& texturex, int index1, int index2) :
	editor_{ &editor },
	texturex_(&texturex),
	index1_(index1),
	index2_(index2)
{
}

// -----------------------------------------------------------------------------
// TextureSwapUS class destructor
// -----------------------------------------------------------------------------
TextureSwapUS::~TextureSwapUS() = default;

// -----------------------------------------------------------------------------
// Swaps the two textures in the list
// -----------------------------------------------------------------------------
bool TextureSwapUS::doSwap() const
{
	// Swap two adjacent/non-adjacent entries and notify listeners for UI refresh
	texturex_->swapTextures(index1_, index2_);
	editor_->signals().textures_swapped(texturex_, index1_, index2_);
	return true;
}


// -----------------------------------------------------------------------------
//
// TextureListReorderUS Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureListReorderUS class constructor
// -----------------------------------------------------------------------------
TextureListReorderUS::TextureListReorderUS(
	const TextureEditor&         editor,
	TextureXList&                texturex,
	unsigned                     first,
	unsigned                     last,
	std::map<unsigned, unsigned> index_swaps) :
	editor_{ &editor },
	texturex_{ &texturex },
	first_{ first },
	last_{ last },
	index_swaps_{ std::move(index_swaps) }
{
}

// -----------------------------------------------------------------------------
// TextureListReorderUS class destructor
// -----------------------------------------------------------------------------
TextureListReorderUS::~TextureListReorderUS() = default;

// -----------------------------------------------------------------------------
// Swaps the order of the textures in the list
// -----------------------------------------------------------------------------
bool TextureListReorderUS::swapOrder()
{
	// Validate the range and the texture list
	if (!texturex_ || first_ > last_ || std::cmp_greater_equal(last_, texturex_->size()))
		return false;

	// If no swaps, nothing to do
	auto count = last_ - first_ + 1;
	if (index_swaps_.empty())
		return true;

	// Build the current order of textures in the range
	vector<CTexture*> current_order;
	current_order.reserve(count);
	for (unsigned i = first_; i <= last_; ++i)
		current_order.push_back(texturex_->texture(i));

	// Build the target order of textures after the swaps
	vector     target_order{ current_order };
	vector<u8> target_used(count, 0);
	for (const auto& [old_index, new_index] : index_swaps_)
	{
		if (old_index < first_ || old_index > last_ || new_index < first_ || new_index > last_)
			return false;

		auto old_offset = old_index - first_;
		auto new_offset = new_index - first_;
		if (target_used[new_offset])
			return false;

		target_order[new_offset] = current_order[old_offset];
		target_used[new_offset]  = true;
	}

	// Swap textures to match the target order
	for (unsigned target = 0; target < target_order.size(); ++target)
	{
		auto target_index = first_ + target;
		if (texturex_->texture(target_index) == target_order[target])
			continue;

		auto swap_index = target_index + 1;
		for (; swap_index <= last_; ++swap_index)
			if (texturex_->texture(swap_index) == target_order[target])
				break;

		if (swap_index > last_)
			return false;

		texturex_->swapTextures(target_index, swap_index);
		editor_->signals().textures_swapped(texturex_, target_index, swap_index);
	}

	// Invert the index swaps for undo/redo
	std::map<unsigned, unsigned> inverse_swaps;
	for (const auto& [old_index, new_index] : index_swaps_)
		inverse_swaps[new_index] = old_index;
	index_swaps_ = std::move(inverse_swaps);

	return true;
}

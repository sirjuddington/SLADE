
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    CTexture.cpp
// Description: C(omposite)Texture class, represents a composite texture as
//              described in TEXTUREx entries
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
#include "CTexture.h"
#include "App.h"
#include "Archive/ArchiveEntry.h"
#include "General/Misc.h"
#include "General/ResourceManager.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "TextureXList.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// CTPatch Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CTPatch class constructor w/initial values
// -----------------------------------------------------------------------------
CTPatch::CTPatch(string_view name, int16_t offset_x, int16_t offset_y) : name_{ name }, offset_{ offset_x, offset_y } {}

// -----------------------------------------------------------------------------
// CTPatch class destructor
// -----------------------------------------------------------------------------
CTPatch::~CTPatch() = default;

// -----------------------------------------------------------------------------
// Returns the entry (if any) associated with this patch via the resource
// manager. Entries in [parent] will be prioritised over entries in any other
// open archive
// -----------------------------------------------------------------------------
ArchiveEntry* CTPatch::patchEntry(Archive* parent)
{
	// Default patches should be in patches namespace
	auto* entry = app::resources().getPatchEntry(name_, "patches", parent);

	// Not found in patches, check in graphics namespace
	if (!entry)
		entry = app::resources().getPatchEntry(name_, "graphics", parent);

	// Not found in patches, check in stand-alone texture namespace
	if (!entry)
		entry = app::resources().getPatchEntry(name_, "textures", parent);

	return entry;
}


// -----------------------------------------------------------------------------
//
// CTPatchEx Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CTPatchEx class constructor w/basic initial values
// -----------------------------------------------------------------------------
CTPatchEx::CTPatchEx(string_view name, int16_t offset_x, int16_t offset_y, Type type) :
	CTPatch{ name, offset_x, offset_y },
	type_{ type }
{
}

// -----------------------------------------------------------------------------
// CTPatchEx class constructor copying another CTPatch (non-extended)
// -----------------------------------------------------------------------------
CTPatchEx::CTPatchEx(const CTPatch& copy) : CTPatch{ copy } {}

// -----------------------------------------------------------------------------
// CTPatchEx class constructor copying another CTPatchEx
// -----------------------------------------------------------------------------
CTPatchEx::CTPatchEx(const CTPatchEx& copy) :
	CTPatch{ copy },
	type_{ copy.type_ },
	flip_x_{ copy.flip_x_ },
	flip_y_{ copy.flip_y_ },
	use_offsets_{ copy.use_offsets_ },
	rotation_{ copy.rotation_ },
	colour_{ copy.colour_ },
	alpha_{ copy.alpha_ },
	style_{ copy.style_ },
	blendtype_{ copy.blendtype_ }
{
	if (copy.translation_)
	{
		translation_ = std::make_unique<Translation>();
		translation_->copy(*copy.translation_);
	}
}

// -----------------------------------------------------------------------------
// CTPatchEx class destructor
// -----------------------------------------------------------------------------
CTPatchEx::~CTPatchEx() = default;

// -----------------------------------------------------------------------------
// Sets the patch's [translation] (but not blend type)
// -----------------------------------------------------------------------------
void CTPatchEx::setTranslation(const Translation& translation)
{
	if (!translation_)
		translation_ = std::make_unique<Translation>();

	translation_->copy(translation);
}

// -----------------------------------------------------------------------------
// Returns true if the patch has a translation defined
// -----------------------------------------------------------------------------
bool CTPatchEx::hasTranslation() const
{
	return translation_ && !translation_->isEmpty();
}

// -----------------------------------------------------------------------------
// Returns the entry (if any) associated with this patch via the resource
// manager. Entries in [parent] will be prioritised over entries in any other
// open archive
// -----------------------------------------------------------------------------
ArchiveEntry* CTPatchEx::patchEntry(Archive* parent)
{
	// 'Patch' type: patches > graphics
	if (type_ == Type::Patch)
	{
		auto* entry = app::resources().getPatchEntry(name_, "patches", parent);
		if (!entry)
			entry = app::resources().getFlatEntry(name_, parent);
		if (!entry)
			entry = app::resources().getPatchEntry(name_, "graphics", parent);
		return entry;
	}

	// 'Graphic' type: graphics > patches
	if (type_ == Type::Graphic)
	{
		auto* entry = app::resources().getPatchEntry(name_, "graphics", parent);
		if (!entry)
			entry = app::resources().getPatchEntry(name_, "patches", parent);
		if (!entry)
			entry = app::resources().getFlatEntry(name_, parent);
		return entry;
	}
	// Silence warnings
	return nullptr;
}

// -----------------------------------------------------------------------------
// Parses a ZDoom TEXTURES format patch definition
// -----------------------------------------------------------------------------
bool CTPatchEx::parse(Tokenizer& tz, Type type)
{
	// Read basic info
	type_ = type;
	name_ = strutil::upper(tz.next().text);
	tz.adv(); // Skip ,
	offset_.x = tz.next().asInt();
	tz.adv(); // Skip ,
	offset_.y = tz.next().asInt();

	// Check if there is any extended info
	if (tz.advIfNext("{", 2))
	{
		// Parse extended info
		while (!tz.checkOrEnd("}"))
		{
			// FlipX
			if (tz.checkNC("FlipX"))
				flip_x_ = true;

			// FlipY
			if (tz.checkNC("FlipY"))
				flip_y_ = true;

			// UseOffsets
			if (tz.checkNC("UseOffsets"))
				use_offsets_ = true;

			// Rotate
			if (tz.checkNC("Rotate"))
				rotation_ = tz.next().asInt();

			// Translation
			if (tz.checkNC("Translation"))
			{
				// Build translation string
				string translate;
				string temp = tz.next().text;
				if (strutil::contains(temp, '='))
					temp = fmt::format("\"{}\"", temp);
				translate += temp;
				while (tz.checkNext(","))
				{
					translate += tz.next().text; // add ','
					temp = tz.next().text;
					if (strutil::contains(temp, '='))
						temp = fmt::format("\"{}\"", temp);
					translate += temp;
				}
				// Parse whole string
				translation_ = std::make_unique<Translation>();
				translation_->parse(translate);
				blendtype_ = BlendType::Translation;
			}

			// Blend
			if (tz.checkNC("Blend"))
			{
				double   val;
				wxColour col;
				blendtype_ = BlendType::Blend;

				// Read first value
				auto first = tz.next().text;

				// If no second value, it's just a colour string
				if (!tz.checkNext(","))
				{
					col.Set(first);
					colour_.set(col);
				}
				else
				{
					// Second value could be alpha or green
					tz.adv(); // Skip ,
					const double second = tz.next().asFloat();

					// If no third value, it's an alpha value
					if (!tz.checkNext(","))
					{
						col.Set(first);
						colour_.set(col);
						colour_.a  = static_cast<uint8_t>(second * 255.0);
						blendtype_ = BlendType::Tint;
					}
					else
					{
						// Third value exists, must be R,G,B,A format
						// RGB are ints in the 0-255 range; A is float in the 0.0-1.0 range
						tz.adv(); // Skip ,
						strutil::toDouble(first, val);
						colour_.r = static_cast<uint8_t>(val);
						colour_.g = static_cast<uint8_t>(second);
						colour_.b = tz.next().asInt();
						if (!tz.checkNext(","))
						{
							log::error("Invalid TEXTURES definition, expected ',', got '{}'", tz.peek().text);
							return false;
						}
						tz.adv(); // Skip ,
						colour_.a  = static_cast<uint8_t>(tz.next().asFloat() * 255.0);
						blendtype_ = BlendType::Tint;
					}
				}
			}

			// Alpha
			if (tz.checkNC("Alpha"))
				alpha_ = tz.next().asFloat();

			// Style
			if (tz.checkNC("Style"))
				style_ = tz.next().text;

			// Read next property name
			tz.adv();
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns a text representation of the patch in ZDoom TEXTURES format
// -----------------------------------------------------------------------------
string CTPatchEx::asText()
{
	// Init text string
	string typestring = "Patch";
	if (type_ == Type::Graphic)
		typestring = "Graphic";
	auto text = fmt::format("\t{} \"{}\", {}, {}\n", typestring, name_, offset_.x, offset_.y);

	// Check if we need to write any extra properties
	if (!flip_x_ && !flip_y_ && !use_offsets_ && rotation_ == 0 && blendtype_ == BlendType::None && alpha_ == 1.0f
		&& strutil::equalCI(style_, "Copy"))
		return text;
	else
		text += "\t{\n";

	// Write patch properties
	if (flip_x_)
		text += "\t\tFlipX\n";
	if (flip_y_)
		text += "\t\tFlipY\n";
	if (use_offsets_)
		text += "\t\tUseOffsets\n";
	if (rotation_ != 0)
		text += fmt::format("\t\tRotate {}\n", rotation_);
	if (blendtype_ == BlendType::Translation && translation_ && !translation_->isEmpty())
	{
		text += "\t\tTranslation ";
		text += translation_->asText();
		text += "\n";
	}
	if (blendtype_ == BlendType::Blend || blendtype_ == BlendType::Tint)
	{
		wxColour col(colour_.r, colour_.g, colour_.b);
		text += fmt::format("\t\tBlend \"{}\"", col.GetAsString(wxC2S_HTML_SYNTAX).ToStdString());

		if (blendtype_ == BlendType::Tint)
			text += fmt::format(", {:1.1f}\n", static_cast<double>(colour_.a) / 255.0);
		else
			text += "\n";
	}
	if (alpha_ < 1.0f)
		text += fmt::format("\t\tAlpha {:1.2f}\n", alpha_);
	if (!(strutil::equalCI(style_, "Copy")))
		text += fmt::format("\t\tStyle {}\n", style_);

	// Write ending
	text += "\t}\n";

	return text;
}


// -----------------------------------------------------------------------------
//
// CTexture Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// CTexture class destructor
// -----------------------------------------------------------------------------
CTexture::~CTexture() = default;

// -----------------------------------------------------------------------------
// Copies the texture [tex] to this texture.
// If [keep_type] is true, the current texture type (extended/regular) will be
// kept, otherwise it will be converted to the type of [tex]
// -----------------------------------------------------------------------------
void CTexture::copyTexture(const CTexture& tex, bool keep_type)
{
	// Clear current texture
	clear();

	// Copy texture info
	name_          = tex.name_;
	size_          = tex.size_;
	def_size_      = tex.def_size_;
	scale_         = tex.scale_;
	world_panning_ = tex.world_panning_;
	if (!keep_type)
	{
		extended_ = tex.extended_;
		defined_  = tex.defined_;
	}
	optional_     = tex.optional_;
	no_decals_    = tex.no_decals_;
	null_texture_ = tex.null_texture_;
	offset_       = tex.offset_;
	type_         = tex.type_;

	// Update scaling
	if (extended_)
	{
		if (scale_.x == 0)
			scale_.x = 1;
		if (scale_.y == 0)
			scale_.y = 1;
	}
	else if (!extended_ && tex.extended_)
	{
		if (scale_.x == 1)
			scale_.x = 0;
		if (scale_.y == 1)
			scale_.y = 0;
	}

	// Copy patches
	for (unsigned a = 0; a < tex.nPatches(); a++)
	{
		auto* patch = tex.patch(a);

		if (extended_)
		{
			if (tex.extended_)
				patches_.push_back(std::make_unique<CTPatchEx>(*dynamic_cast<CTPatchEx*>(patch)));
			else
				patches_.push_back(std::make_unique<CTPatchEx>(*patch));
		}
		else
			addPatch(patch->name(), patch->xOffset(), patch->yOffset());
	}
}

// -----------------------------------------------------------------------------
// Returns the texture's scale as a multiplication factor
// -----------------------------------------------------------------------------
Vec2d CTexture::scaleFactor() const
{
	Vec2d scale = scale_;
	if (scale.x == 0.0)
		scale.x = 1.0;
	else
		scale.x = 1.0 / scale.x;
	if (scale.y == 0.0)
		scale.y = 1.0;
	else
		scale.y = 1.0 / scale.y;

	return scale;
}

// -----------------------------------------------------------------------------
// Returns the patch at [index], or NULL if [index] is out of bounds
// -----------------------------------------------------------------------------
CTPatch* CTexture::patch(size_t index) const
{
	// Check index
	if (index >= patches_.size())
		return nullptr;

	// Return patch at index
	return patches_[index].get();
}

// -----------------------------------------------------------------------------
// Returns the index of this texture within its parent list
// -----------------------------------------------------------------------------
int CTexture::index() const
{
	// Check if a parent TextureXList exists
	if (!in_list_)
		return index_;

	// Find this texture in the parent list
	return in_list_->textureIndex(name());
}

// -----------------------------------------------------------------------------
// Clears all texture data
// -----------------------------------------------------------------------------
void CTexture::clear()
{
	name_          = "";
	size_          = { 0, 0 };
	def_size_      = { 0, 0 };
	scale_         = { 1., 1. };
	defined_       = false;
	world_panning_ = false;
	optional_      = false;
	no_decals_     = false;
	null_texture_  = false;
	offset_        = { 0, 0 };
	patches_.clear();
}

// -----------------------------------------------------------------------------
// Adds a patch to the texture with the given attributes, at [index].
// If [index] is -1, the patch is added to the end of the list.
// -----------------------------------------------------------------------------
bool CTexture::addPatch(string_view patch, int16_t offset_x, int16_t offset_y, int index)
{
	// Create new patch
	unique_ptr<CTPatch> np;
	if (extended_)
		np = std::make_unique<CTPatchEx>(patch, offset_x, offset_y);
	else
		np = std::make_unique<CTPatch>(patch, offset_x, offset_y);

	// Add it either after [index] or at the end
	if (index >= 0 && static_cast<unsigned>(index) < patches_.size())
		patches_.insert(patches_.begin() + index, std::move(np));
	else
		patches_.push_back(std::move(np));

	// Cannot be a simple define anymore
	defined_ = false;

	// Announce
	signals_.patches_modified(*this);

	return true;
}

// -----------------------------------------------------------------------------
// Removes the patch at [index].
// Returns false if [index] is invalid, true otherwise
// -----------------------------------------------------------------------------
bool CTexture::removePatch(size_t index)
{
	// Check index
	if (index >= patches_.size())
		return false;

	// Remove the patch
	patches_.erase(patches_.begin() + index);

	// Cannot be a simple define anymore
	defined_ = false;

	// Announce
	signals_.patches_modified(*this);

	return true;
}

// -----------------------------------------------------------------------------
// Removes all instances of [patch] from the texture.
// Returns true if any were removed, false otherwise
// -----------------------------------------------------------------------------
bool CTexture::removePatch(string_view patch)
{
	// Go through patches
	bool removed = false;
	for (unsigned a = 0; a < patches_.size(); a++)
	{
		if (patches_[a]->name() == patch)
		{
			patches_.erase(patches_.begin() + a);
			removed = true;
			a--;
		}
	}

	// Cannot be a simple define anymore
	defined_ = false;

	if (removed)
		signals_.patches_modified(*this);

	return removed;
}

// -----------------------------------------------------------------------------
// Replaces the patch at [index] with [newpatch], and updates its associated
// ArchiveEntry with [newentry].
// Returns false if [index] is out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool CTexture::replacePatch(size_t index, string_view newpatch)
{
	// Check index
	if (index >= patches_.size())
		return false;

	// Replace patch at [index] with new
	patches_[index]->setName(newpatch);

	// Announce
	signals_.patches_modified(*this);

	return true;
}

// -----------------------------------------------------------------------------
// Duplicates the patch at [index], placing the duplicated patch at
// [offset_x],[offset_y] from the original.
// Returns false if [index] is out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool CTexture::duplicatePatch(size_t index, int16_t offset_x, int16_t offset_y)
{
	// Check index
	if (index >= patches_.size())
		return false;

	// Get patch info
	auto* dp = patches_[index].get();

	// Add duplicate patch
	if (extended_)
	{
		auto* ex_patch = dynamic_cast<CTPatchEx*>(patches_[index].get());
		patches_.insert(patches_.begin() + index, std::make_unique<CTPatchEx>(*ex_patch));
	}
	else
		patches_.insert(patches_.begin() + index, std::make_unique<CTPatch>(*patches_[index]));

	// Offset patch by given amount
	patches_[index + 1]->setOffsetX(dp->xOffset() + offset_x);
	patches_[index + 1]->setOffsetY(dp->yOffset() + offset_y);

	// Cannot be a simple define anymore
	defined_ = false;

	// Announce
	signals_.patches_modified(*this);

	return true;
}

// -----------------------------------------------------------------------------
// Swaps the patches at [p1] and [p2].
// Returns false if either index is invalid, true otherwise
// -----------------------------------------------------------------------------
bool CTexture::swapPatches(size_t p1, size_t p2)
{
	// Check patch indices are correct
	if (p1 >= patches_.size() || p2 >= patches_.size())
		return false;

	// Swap the patches
	patches_[p1].swap(patches_[p2]);

	// Announce
	signals_.patches_modified(*this);

	return true;
}

// -----------------------------------------------------------------------------
// Parses a TEXTURES format texture definition
// -----------------------------------------------------------------------------
bool CTexture::parse(Tokenizer& tz, string_view type)
{
	// Check if optional
	if (tz.advIfNextNC("optional"))
		optional_ = true;

	// Read basic info
	type_     = type;
	extended_ = true;
	defined_  = false;
	name_     = strutil::upper(tz.next().text);
	tz.adv(); // Skip ,
	size_.x = tz.next().asInt();
	tz.adv(); // Skip ,
	size_.y = tz.next().asInt();

	// Check for extended info
	if (tz.advIfNext("{", 2))
	{
		// Read properties
		while (!tz.check("}"))
		{
			// Check if end of text is reached (error)
			if (tz.atEnd())
			{
				log::error("Error parsing texture {}: End of text found, missing }} perhaps?", name_);
				return false;
			}

			// XScale
			if (tz.checkNC("XScale"))
				scale_.x = tz.next().asFloat();

			// YScale
			else if (tz.checkNC("YScale"))
				scale_.y = tz.next().asFloat();

			// Offset
			else if (tz.checkNC("Offset"))
			{
				offset_.x = tz.next().asInt();
				tz.skipToken(); // Skip ,
				offset_.y = tz.next().asInt();
			}

			// WorldPanning
			else if (tz.checkNC("WorldPanning"))
				world_panning_ = true;

			// NoDecals
			else if (tz.checkNC("NoDecals"))
				no_decals_ = true;

			// NullTexture
			else if (tz.checkNC("NullTexture"))
				null_texture_ = true;

			// Patch
			else if (tz.checkNC("Patch"))
			{
				auto* patch = new CTPatchEx();
				patch->parse(tz);
				patches_.emplace_back(patch);
			}

			// Graphic
			else if (tz.checkNC("Graphic"))
			{
				auto* patch = new CTPatchEx();
				patch->parse(tz, CTPatchEx::Type::Graphic);
				patches_.emplace_back(patch);
			}

			// Read next property
			tz.adv();
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Parses a HIRESTEX define block
// -----------------------------------------------------------------------------
bool CTexture::parseDefine(Tokenizer& tz)
{
	type_       = "Define";
	extended_   = true;
	defined_    = true;
	name_       = strutil::upper(tz.next().text);
	def_size_.x = tz.next().asInt();
	def_size_.y = tz.next().asInt();
	size_       = def_size_;
	auto* entry = app::resources().getPatchEntry(name_);
	if (entry)
	{
		SImage image;
		if (image.open(entry->data()))
		{
			size_.x  = image.width();
			size_.y  = image.height();
			scale_.x = static_cast<double>(size_.x) / static_cast<double>(def_size_.x);
			scale_.y = static_cast<double>(size_.y) / static_cast<double>(def_size_.y);
		}
	}
	patches_.push_back(std::make_unique<CTPatchEx>(name_));
	return true;
}

// -----------------------------------------------------------------------------
// Returns a string representation of the texture, in ZDoom TEXTURES format
// -----------------------------------------------------------------------------
string CTexture::asText()
{
	// Can't write non-extended texture as text
	if (!extended_)
		return "";

	// Define block
	if (defined_)
		return fmt::format("define \"{}\" {} {}\n", name_, def_size_.x, def_size_.y);

	// Init text string
	string text;
	if (optional_)
		text = fmt::format("{} Optional \"{}\", {}, {}\n{{\n", type_, name_, size_.x, size_.y);
	else
		text = fmt::format("{} \"{}\", {}, {}\n{{\n", type_, name_, size_.x, size_.y);

	// Write texture properties
	if (scale_.x != 1.0)
		text += fmt::format("\tXScale {:1.3f}\n", scale_.x);
	if (scale_.y != 1.0)
		text += fmt::format("\tYScale {:1.3f}\n", scale_.y);
	if (offset_.x != 0 || offset_.y != 0)
		text += fmt::format("\tOffset {}, {}\n", offset_.x, offset_.y);
	if (world_panning_)
		text += "\tWorldPanning\n";
	if (no_decals_)
		text += "\tNoDecals\n";
	if (null_texture_)
		text += "\tNullTexture\n";

	// Write patches
	for (auto& patch : patches_)
		text += dynamic_cast<CTPatchEx*>(patch.get())->asText();

	// Write ending
	text += "}\n\n";

	return text;
}

// -----------------------------------------------------------------------------
// Converts the texture to 'extended' (ZDoom TEXTURES) format
// -----------------------------------------------------------------------------
bool CTexture::convertExtended()
{
	// Simple conversion system for defines
	if (defined_)
		defined_ = false;

	// Don't convert if already extended
	if (extended_)
		return true;

	// Convert scale if needed
	if (scale_.x == 0)
		scale_.x = 1;
	if (scale_.y == 0)
		scale_.y = 1;

	// Convert all patches over to extended format
	for (auto& patch : patches_)
	{
		auto* expatch = new CTPatchEx(*patch);
		patch.reset(expatch);
	}

	// Set extended flag and type
	extended_ = true;
	type_     = "WallTexture";

	return true;
}

// -----------------------------------------------------------------------------
// Converts the texture to 'regular' (TEXTURE1/2) format
// -----------------------------------------------------------------------------
bool CTexture::convertRegular()
{
	// Don't convert if already regular
	if (!extended_)
		return true;

	// Convert scale
	if (scale_.x == 1)
		scale_.x = 0;
	else
		scale_.x *= 8;
	if (scale_.y == 1)
		scale_.y = 0;
	else
		scale_.y *= 8;

	// Convert all patches over to normal format
	for (auto& patch : patches_)
	{
		auto* npatch = new CTPatch(patch->name(), patch->xOffset(), patch->yOffset());
		patch.reset(npatch);
	}

	// Unset extended flag
	extended_ = false;
	defined_  = false;

	return true;
}

// -----------------------------------------------------------------------------
// Generates a SImage representation of this texture, using patches from
// [parent] primarily, and the palette [pal]
// -----------------------------------------------------------------------------
bool CTexture::toImage(SImage& image, Archive* parent, const Palette* pal, bool force_rgba)
{
	// Init image
	image.clear();
	image.resize(size_.x, size_.y);

	// Add patches
	SImage            p_img(force_rgba ? SImage::Type::RGBA : SImage::Type::PalMask);
	SImage::DrawProps dp;
	dp.src_alpha = false;
	if (defined_)
	{
		if (!loadPatchImage(0, p_img, parent, pal, force_rgba))
			return false;
		size_.x = p_img.width();
		size_.y = p_img.height();
		image.resize(size_.x, size_.y);
		scale_.x = static_cast<double>(size_.x) / static_cast<double>(def_size_.x);
		scale_.y = static_cast<double>(size_.y) / static_cast<double>(def_size_.y);
		image.drawImage(p_img, 0, 0, dp, pal, pal);
	}
	else if (extended_)
	{
		// Extended texture

		// Add each patch to image
		for (unsigned a = 0; a < patches_.size(); a++)
		{
			auto* patch = dynamic_cast<CTPatchEx*>(patches_[a].get());

			// If the patch has a translation, ensure the image is paletted
			if (patch->blendType() == CTPatchEx::BlendType::Translation && p_img.type() != SImage::Type::PalMask)
				p_img.clear(SImage::Type::PalMask);

			// Load patch entry
			if (!loadPatchImage(a, p_img, parent, pal, force_rgba))
				continue;

			// Handle offsets
			int ofs_x = patch->xOffset();
			int ofs_y = patch->yOffset();
			if (patch->useOffsets())
			{
				ofs_x -= p_img.offset().x;
				ofs_y -= p_img.offset().y;
			}

			// Apply translation before anything in case we're forcing rgba (can't translate rgba images)
			if (patch->blendType() == CTPatchEx::BlendType::Translation && patch->hasTranslation())
				p_img.applyTranslation(patch->translation(), pal, force_rgba);

			// Convert to RGBA if forced
			if (force_rgba)
				p_img.convertRGBA(pal);

			// Flip/rotate if needed
			if (patch->flipX())
				p_img.mirror(false);
			if (patch->flipY())
				p_img.mirror(true);
			if (patch->rotation() != 0)
				p_img.rotate(patch->rotation());

			// Setup transparency blending
			dp.blend     = SImage::BlendType::Normal;
			dp.alpha     = 1.0f;
			dp.src_alpha = false;
			if (patch->style() == "CopyAlpha" || patch->style() == "Overlay")
				dp.src_alpha = true;
			else if (patch->style() == "Translucent" || patch->style() == "CopyNewAlpha")
				dp.alpha = patch->alpha();
			else if (patch->style() == "Add")
			{
				dp.blend = SImage::BlendType::Add;
				dp.alpha = patch->alpha();
			}
			else if (patch->style() == "Subtract")
			{
				dp.blend = SImage::BlendType::Subtract;
				dp.alpha = patch->alpha();
			}
			else if (patch->style() == "ReverseSubtract")
			{
				dp.blend = SImage::BlendType::ReverseSubtract;
				dp.alpha = patch->alpha();
			}
			else if (patch->style() == "Modulate")
			{
				dp.blend = SImage::BlendType::Modulate;
				dp.alpha = patch->alpha();
			}

			// Setup patch colour
			if (patch->blendType() == CTPatchEx::BlendType::Blend)
				p_img.colourise(patch->colour(), pal);
			else if (patch->blendType() == CTPatchEx::BlendType::Tint)
				p_img.tint(patch->colour(), patch->colour().fa(), pal);


			// Add patch to texture image
			image.drawImage(p_img, ofs_x, ofs_y, dp, pal, pal);
		}
	}
	else
	{
		// Normal texture

		// Add each patch to image
		for (auto& patch : patches_)
		{
			if (misc::loadImageFromEntry(&p_img, patch->patchEntry(parent)))
				image.drawImage(p_img, patch->xOffset(), patch->yOffset(), dp, pal, pal);
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads the image for the patch at [pindex] into [image].
// Can deal with textures-as-patches
// -----------------------------------------------------------------------------
bool CTexture::loadPatchImage(unsigned pindex, SImage& image, Archive* parent, const Palette* pal, bool force_rgba)
	const
{
	// Check patch index
	if (pindex >= patches_.size())
		return false;

	auto* patch = patches_[pindex].get();

	// If the texture is extended, search for textures-as-patches first
	// (as long as the patch name is different from this texture's name)
	if (extended_ && !strutil::equalCI(patch->name(), name_))
	{
		// Search the texture list we're in first
		if (in_list_)
		{
			for (unsigned a = 0; a < in_list_->size(); a++)
			{
				auto* tex = in_list_->texture(a);

				// Don't look past this texture in the list
				if (tex->name() == name_)
					break;

				// Check for name match
				if (strutil::equalCI(tex->name(), patch->name()))
				{
					// Load texture to image
					return tex->toImage(image, parent, pal, force_rgba);
				}
			}
		}

		// Otherwise, try the resource manager
		// TODO: Something has to be ignored here. The entire archive or just the current list?
		auto* tex = app::resources().getTexture(patch->name(), "", parent);
		if (tex)
			return tex->toImage(image, parent, pal, force_rgba);
	}

	// Get patch entry
	auto* entry = patch->patchEntry(parent);

	// Load entry to image if valid
	if (entry)
		return misc::loadImageFromEntry(&image, entry);

	// Maybe it's a texture?
	entry = app::resources().getTextureEntry(patch->name(), "", parent);

	if (entry)
		return misc::loadImageFromEntry(&image, entry);

	return false;
}


// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SBrush.cpp
// Description: SBrush class. Handles pixel painting for GfxCanvas.
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
#include "SBrush.h"
#include "Archive/ArchiveManager.h"
#include "General/SAction.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
// SBrushManager* SBrushManager::instance_ = nullptr;
namespace
{
vector<unique_ptr<SBrush>> brushes;
}


// -----------------------------------------------------------------------------
//
// SBrush Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SBrush class constructor
// -----------------------------------------------------------------------------
SBrush::SBrush(const wxString& name) : name_{ name }, icon_{ name.AfterFirst('_') }
{
	auto res = App::archiveManager().programResourceArchive();
	if (res == nullptr)
		return;
	auto file = res->entryAtPath(fmt::format("icons/general/{}.png", icon_.ToStdString()));
	if (file == nullptr || file->size() == 0)
	{
		Log::error(2, wxString::Format("error, no file at icons/general/%s.png", icon_));
		return;
	}
	image_ = std::make_unique<SImage>();
	if (!image_->open(file->data(), 0, "png"))
	{
		Log::error(2, wxString::Format("couldn't load image data for icons/general/%s.png", icon_));
		return;
	}
	image_->convertAlphaMap(SImage::AlphaSource::Alpha);
	center_.x = image_->width() >> 1;
	center_.y = image_->height() >> 1;
}

// -----------------------------------------------------------------------------
// Returns intensity of how much this pixel is affected by the brush;
// [0, 0] is the brush's center
// -----------------------------------------------------------------------------
uint8_t SBrush::pixel(int x, int y) const
{
	x += center_.x;
	y += center_.y;
	if (image_ && x >= 0 && x < image_->width() && y >= 0 && y < image_->height())
		return image_->pixelIndexAt((unsigned)x, (unsigned)y);
	return 0;
}


// -----------------------------------------------------------------------------
//
// SBrush Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Get a brush from its name
// -----------------------------------------------------------------------------
SBrush* SBrush::get(const wxString& name)
{
	for (auto& brush : brushes)
		if (S_CMPNOCASE(name, brush->name()))
			return brush.get();

	return nullptr;
}

// -----------------------------------------------------------------------------
// Init brushes
// -----------------------------------------------------------------------------
bool SBrush::initBrushes()
{
	brushes.emplace_back(new SBrush("pgfx_brush_sq_1"));
	brushes.emplace_back(new SBrush("pgfx_brush_sq_3"));
	brushes.emplace_back(new SBrush("pgfx_brush_sq_5"));
	brushes.emplace_back(new SBrush("pgfx_brush_sq_7"));
	brushes.emplace_back(new SBrush("pgfx_brush_sq_9"));
	brushes.emplace_back(new SBrush("pgfx_brush_ci_5"));
	brushes.emplace_back(new SBrush("pgfx_brush_ci_7"));
	brushes.emplace_back(new SBrush("pgfx_brush_ci_9"));
	brushes.emplace_back(new SBrush("pgfx_brush_di_3"));
	brushes.emplace_back(new SBrush("pgfx_brush_di_5"));
	brushes.emplace_back(new SBrush("pgfx_brush_di_7"));
	brushes.emplace_back(new SBrush("pgfx_brush_di_9"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_a"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_b"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_c"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_d"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_e"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_f"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_g"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_h"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_i"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_j"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_k"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_l"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_m"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_n"));
	brushes.emplace_back(new SBrush("pgfx_brush_pa_o"));
	return true;
}


// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    RenderSpecials.cpp
// Description: MapSpecials related to rendering effects (translucency, etc.)
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
#include "RenderSpecials.h"
#include "Game/Configuration.h"
#include "LineTranslucency.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/SLADEMap.h"

using namespace slade;
using namespace map;


// -----------------------------------------------------------------------------
//
// RenderSpecials Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// RenderSpecials class constructor
// -----------------------------------------------------------------------------
RenderSpecials::RenderSpecials(SLADEMap& map) : map_{ &map } {}

// -----------------------------------------------------------------------------
// Returns the translucency info for [line] if any translucency special applies
// -----------------------------------------------------------------------------
optional<LineTranslucency> RenderSpecials::lineTranslucency(const MapLine& line)
{
	// Sort by line index (descending) if needed
	if (!line_transparency_specials_sorted_)
	{
		std::ranges::sort(
			line_transparency_specials_,
			[](const auto& a, const auto& b) { return a.line->index() > b.line->index(); });
		line_transparency_specials_sorted_ = true;
	}

	// Since the translucency specials are sorted by line index descending,
	// we can just apply the first one we find
	for (const auto& special : line_transparency_specials_)
	{
		if (special.target == &line)
			return LineTranslucency{ .alpha = special.alpha, .additive = special.additive };
	}

	return std::nullopt;
}

void RenderSpecials::processLineSpecial(const MapLine& line)
{
	// ZDoom
	if (game::configuration().currentPort() == "zdoom")
	{
		switch (line.special())
		{
		case 208: addTranslucentLine(line, true); break;
		default:  break;
		}
	}

	// Boom
	if (game::configuration().featureSupported(game::Feature::Boom))
	{
		switch (line.special())
		{
		case 260: addTranslucentLine(line, false, 168); break;
		default:  break;
		}
	}
}

void RenderSpecials::clearSpecials()
{
	line_transparency_specials_.clear();
}

void RenderSpecials::lineUpdated(const MapLine& line)
{
	// Remove existing specials for line
	removeTranslucentLine(line);

	// Re-process
	processLineSpecial(line);
}

void RenderSpecials::addTranslucentLine(const MapLine& line, bool zdoom, u8 alpha, bool additive)
{
	// Get target lines
	vector<const MapLine*> targets;
	auto                   lineid = line.arg(0);
	if (lineid == 0)
		targets.push_back(&line);
	else
		for (auto tl : map_->lines().allWithId(lineid))
			targets.push_back(tl);

	// Get properties
	float fa = static_cast<float>(zdoom ? line.arg(1) : alpha) / 255.0f;
	if (zdoom)
		additive = line.arg(2) > 0;

	// Add specials
	for (auto target : targets)
		line_transparency_specials_.push_back({ .line = &line, .target = target, .alpha = fa, .additive = additive });

	line_transparency_specials_sorted_ = false;
}

void RenderSpecials::removeTranslucentLine(const MapLine& line)
{
	for (unsigned i = 0; i < line_transparency_specials_.size();)
	{
		if (line_transparency_specials_[i].line == &line)
		{
			line_transparency_specials_.erase(line_transparency_specials_.begin() + i);
			continue;
		}

		++i;
	}
}

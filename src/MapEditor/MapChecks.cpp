
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapChecks.cpp
// Description: Various handler classes for map error/problem checks
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
#include "MapChecks.h"
#include "Edit/Edit2D.h"
#include "Game/ActionSpecial.h"
#include "Game/Configuration.h"
#include "Game/Game.h"
#include "Game/ThingType.h"
#include "Geometry/Geometry.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapTextureManager.h"
#include "OpenGL/GLTexture.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/SLADEMap.h"
#include "UI/Browser/BrowserItem.h"
#include "UI/Dialogs/MapTextureBrowser.h"
#include "UI/Dialogs/ThingTypeBrowser.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
struct StandardCheckInfo
{
	string id;
	string description;
};
std::map<MapCheck::StandardCheck, StandardCheckInfo> std_checks = {
	{ MapCheck::MissingTexture, { "missing_tex", "Missing textures" } },
	{ MapCheck::SpecialTag, { "missing_tag", "Missing action special tags" } },
	{ MapCheck::IntersectingLine, { "intersecting_line", "Intersecting lines" } },
	{ MapCheck::OverlappingLine, { "overlapping_line", "Overlapping lines" } },
	{ MapCheck::OverlappingThing, { "overlapping_thing", "Overlapping things" } },
	{ MapCheck::UnknownTexture, { "unknown_texture", "Unknown wall textures" } },
	{ MapCheck::UnknownFlat, { "unknown_flat", "Unknown flat textures" } },
	{ MapCheck::UnknownThingType, { "unknown_thing", "Unknown thing types" } },
	{ MapCheck::StuckThing, { "stuck_thing", "Stuck things" } },
	{ MapCheck::SectorReference, { "sector_ref", "Invalid sector references" } },
	{ MapCheck::InvalidLine, { "invalid_line", "Invalid lines" } },
	{ MapCheck::MissingTagged, { "missing_tagged", "Missing tagged objects" } },
	{ MapCheck::UnknownSector, { "unknown_sector_type", "Unknown sector types" } },
	{ MapCheck::UnknownSpecial, { "unknown_special", "Unknown line and thing specials" } },
	{ MapCheck::ObsoleteThing, { "obsolete_thing", "Obsolete things" } },
};
} // namespace


// -----------------------------------------------------------------------------
// MissingTextureCheck Class
//
// Checks for any missing textures on lines
// -----------------------------------------------------------------------------
class MissingTextureCheck : public MapCheck
{
public:
	MissingTextureCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		string sky_flat = game::configuration().skyFlat();
		for (unsigned a = 0; a < map_->nLines(); a++)
		{
			// Check what textures the line needs
			auto line  = map_->line(a);
			auto side1 = line->s1();
			auto side2 = line->s2();
			int  needs = line->needsTexture();

			// Detect if sky hack might apply
			bool sky_hack = false;
			if (side1 && strutil::equalCI(sky_flat, side1->sector()->ceiling().texture) && side2
				&& strutil::equalCI(sky_flat, side2->sector()->ceiling().texture))
				sky_hack = true;

			// Check for missing textures (front side)
			if (side1)
			{
				// Upper
				if ((needs & MapLine::Part::FrontUpper) > 0 && side1->texUpper() == MapSide::TEX_NONE && !sky_hack)
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontUpper);
				}

				// Middle
				if ((needs & MapLine::Part::FrontMiddle) > 0 && side1->texMiddle() == MapSide::TEX_NONE)
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontMiddle);
				}

				// Lower
				if ((needs & MapLine::Part::FrontLower) > 0 && side1->texLower() == MapSide::TEX_NONE)
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontLower);
				}
			}

			// Check for missing textures (back side)
			if (side2)
			{
				// Upper
				if ((needs & MapLine::Part::BackUpper) > 0 && side2->texUpper() == MapSide::TEX_NONE && !sky_hack)
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackUpper);
				}

				// Middle
				if ((needs & MapLine::Part::BackMiddle) > 0 && side2->texMiddle() == MapSide::TEX_NONE)
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackMiddle);
				}

				// Lower
				if ((needs & MapLine::Part::BackLower) > 0 && side2->texLower() == MapSide::TEX_NONE)
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackLower);
				}
			}
		}

		log::info(3, "Missing Texture Check: {} missing textures", parts_.size());
	}

	unsigned nProblems() override { return lines_.size(); }

	string texName(int part) const
	{
		switch (part)
		{
		case MapLine::Part::FrontUpper:  return "front upper texture";
		case MapLine::Part::FrontMiddle: return "front middle texture";
		case MapLine::Part::FrontLower:  return "front lower texture";
		case MapLine::Part::BackUpper:   return "back upper texture";
		case MapLine::Part::BackMiddle:  return "back middle texture";
		case MapLine::Part::BackLower:   return "back lower texture";
		default:                         break;
		}

		return "";
	}

	string problemDesc(unsigned index) override
	{
		if (index < lines_.size())
			return fmt::format("Line {} missing {}", lines_[index]->index(), texName(parts_[index]));
		else
			return "No missing textures found";
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= lines_.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(mapeditor::windowWx(), mapeditor::TextureType::Texture, "-", map_);
			if (browser.ShowModal() == wxID_OK)
			{
				editor->beginUndoRecord("Change Texture", true, false, false);

				// Set texture if one selected
				auto texture = browser.selectedItem()->name();
				switch (parts_[index])
				{
				case MapLine::Part::FrontUpper: lines_[index]->setStringProperty("side1.texturetop", texture); break;
				case MapLine::Part::FrontMiddle:
					lines_[index]->setStringProperty("side1.texturemiddle", texture);
					break;
				case MapLine::Part::FrontLower: lines_[index]->setStringProperty("side1.texturebottom", texture); break;
				case MapLine::Part::BackUpper:  lines_[index]->setStringProperty("side2.texturetop", texture); break;
				case MapLine::Part::BackMiddle: lines_[index]->setStringProperty("side2.texturemiddle", texture); break;
				case MapLine::Part::BackLower:  lines_[index]->setStringProperty("side2.texturebottom", texture); break;
				default:                        return false;
				}

				editor->endUndoRecord();

				// Remove problem
				lines_.erase(lines_.begin() + index);
				parts_.erase(parts_.begin() + index);
				return true;
			}

			return false;
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index >= lines_.size())
			return nullptr;

		return lines_[index];
	}

	string progressText() override { return "Checking for missing textures..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Browse Texture...";

		return "";
	}

private:
	vector<MapLine*> lines_;
	vector<int>      parts_;
};


// -----------------------------------------------------------------------------
// SpecialTagsCheck Class
//
// Checks for lines with an action special that requires a tag (non-zero) but
// have no tag set
// -----------------------------------------------------------------------------
class SpecialTagsCheck : public MapCheck
{
public:
	SpecialTagsCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		using game::TagType;

		for (auto& line : map_->lines())
		{
			if (line->special() == 0)
				continue;

			// Get action special
			auto tagged = game::configuration().actionSpecial(line->special()).needsTag();

			// Check for back sector that removes need for tagged sector
			if ((tagged == TagType::Back || tagged == TagType::SectorOrBack) && line->backSector())
				continue;

			// Check if tag is required but not set
			if (tagged != TagType::None && line->arg(0) == 0)
				objects_.push_back(line);
		}
		// Hexen and UDMF allow specials on things
		if (map_->currentFormat() == MapFormat::Hexen || map_->currentFormat() == MapFormat::UDMF)
		{
			for (unsigned a = 0; a < map_->nThings(); ++a)
			{
				// Ignore the Heresiarch which does not have a real special
				auto& tt = game::configuration().thingType(map_->thing(a)->type());
				if (tt.flags() & game::ThingType::Flags::Script)
					continue;

				// Get special and tag
				int special = map_->thing(a)->special();
				int tag     = map_->thing(a)->arg(0);

				// Get action special
				auto tagged = game::configuration().actionSpecial(special).needsTag();

				// Check if tag is required but not set
				if (tagged != TagType::None && tagged != TagType::Back && tag == 0)
					objects_.push_back(map_->thing(a));
			}
		}
	}

	unsigned nProblems() override { return objects_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= objects_.size())
			return "No missing special tags found";

		auto mo      = objects_[index];
		int  special = mo->intProperty("special");
		return fmt::format(
			"{} {}: Special {} ({}) requires a tag",
			mo->objType() == MapObject::Type::Line ? "Line" : "Thing",
			mo->index(),
			special,
			game::configuration().actionSpecial(special).name());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		// Begin tag edit
		SActionHandler::doAction("mapw_line_tagedit");

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index >= objects_.size())
			return nullptr;

		return objects_[index];
	}

	string progressText() override { return "Checking for missing special tags..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Set Tagged...";

		return "";
	}

private:
	vector<MapObject*> objects_;
};


// -----------------------------------------------------------------------------
// MissingTaggedCheck Class
//
// Checks for lines with an action special that have a tag that does not point
// to anything that exists
// -----------------------------------------------------------------------------
class MissingTaggedCheck : public MapCheck
{
public:
	MissingTaggedCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		using game::TagType;

		unsigned nlines  = map_->nLines();
		unsigned nthings = 0;
		if (map_->currentFormat() == MapFormat::Hexen || map_->currentFormat() == MapFormat::UDMF)
			nthings = map_->nThings();
		for (unsigned a = 0; a < (nlines + nthings); a++)
		{
			MapObject* mo        = nullptr;
			bool       thingmode = false;
			if (a >= nlines)
			{
				mo        = map_->thing(a - nlines);
				thingmode = true;
			}
			else
				mo = map_->line(a);

			// Ignore the Heresiarch which does not have a real special
			if (thingmode)
			{
				auto& tt = game::configuration().thingType(dynamic_cast<MapThing*>(mo)->type());
				if (tt.flags() & game::ThingType::Flags::Script)
					continue;
			}

			// Get special and tag
			int special = mo->intProperty("special");
			int tag     = mo->intProperty("arg0");

			// Get action special
			auto tagged = game::configuration().actionSpecial(special).needsTag();

			// Check if tag is required and set (not set is a different check...)
			if (tagged != TagType::None && tag != 0)
			{
				bool okay = false;
				switch (tagged)
				{
				case TagType::Sector:
				case TagType::SectorOrBack:
				{
					if (map_->sectors().firstWithId(tag))
						okay = true;
				}
				break;
				case TagType::Line:
				{
					if (map_->lines().firstWithId(tag))
						okay = true;
				}
				break;
				case TagType::Thing:
				{
					if (map_->things().firstWithId(tag))
						okay = true;
				}
				break;
				default:
					// Ignore the rest for now...
					okay = true;
					break;
				}
				if (!okay)
				{
					objects_.push_back(mo);
				}
			}
		}
	}

	unsigned nProblems() override { return objects_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= objects_.size())
			return "No missing tagged objects found";

		int special = objects_[index]->intProperty("special");
		return fmt::format(
			"{} {}: No object tagged {} for Special {} ({})",
			objects_[index]->objType() == MapObject::Type::Line ? "Line" : "Thing",
			objects_[index]->index(),
			objects_[index]->intProperty("arg0"),
			special,
			game::configuration().actionSpecial(special).name());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		// Can't automatically fix that
		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index >= objects_.size())
			return nullptr;

		return objects_[index];
	}

	string progressText() override { return "Checking for missing tagged objects..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "...";

		return "";
	}

private:
	vector<MapObject*> objects_;
};


// -----------------------------------------------------------------------------
// LinesIntersectCheck Class
//
// Checks for any intersecting lines
// -----------------------------------------------------------------------------
class LinesIntersectCheck : public MapCheck
{
public:
	LinesIntersectCheck(SLADEMap* map) : MapCheck(map) {}

	void checkIntersections(const vector<MapLine*>& lines)
	{
		Vec2d    pos;
		MapLine* line1;
		MapLine* line2;

		// Clear existing intersections
		intersections_.clear();

		// Go through lines
		for (unsigned a = 0; a < lines.size(); a++)
		{
			line1 = lines[a];

			// Go through uncompared lines
			for (unsigned b = a + 1; b < lines.size(); b++)
			{
				line2 = lines[b];

				// Check intersection
				if (line1->intersects(line2, pos))
					intersections_.emplace_back(line1, line2, pos.x, pos.y);
			}
		}
	}

	void doCheck() override
	{
		// Get all map lines
		vector<MapLine*> all_lines;
		for (unsigned a = 0; a < map_->nLines(); a++)
			all_lines.push_back(map_->line(a));

		// Check for intersections
		checkIntersections(all_lines);
	}

	unsigned nProblems() override { return intersections_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= intersections_.size())
			return "No intersecting lines found";

		return fmt::format(
			"Lines {} and {} are intersecting at ({:1.2f}, {:1.2f})",
			intersections_[index].line1->index(),
			intersections_[index].line2->index(),
			intersections_[index].intersect_point.x,
			intersections_[index].intersect_point.y);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= intersections_.size())
			return false;

		if (fix_type == 0)
		{
			auto line1 = intersections_[index].line1;
			auto line2 = intersections_[index].line2;

			editor->beginUndoRecord("Split Lines");

			// Create split vertex
			auto nv = map_->createVertex(intersections_[index].intersect_point, -1);

			// Split first line
			map_->splitLine(line1, nv);
			auto nl1 = map_->line(map_->nLines() - 1);

			// Split second line
			map_->splitLine(line2, nv);
			auto nl2 = map_->line(map_->nLines() - 1);

			// Remove intersection
			intersections_.erase(intersections_.begin() + index);

			editor->endUndoRecord();

			// Create list of lines to re-check
			vector<MapLine*> lines;
			lines.push_back(line1);
			lines.push_back(line2);
			lines.push_back(nl1);
			lines.push_back(nl2);
			for (auto& intersection : intersections_)
			{
				VECTOR_ADD_UNIQUE(lines, intersection.line1);
				VECTOR_ADD_UNIQUE(lines, intersection.line2);
			}

			// Re-check intersections
			checkIntersections(lines);

			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index >= intersections_.size())
			return nullptr;

		return intersections_[index].line1;
	}

	string progressText() override { return "Checking for intersecting lines..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Split Lines";

		return "";
	}

private:
	struct Intersection
	{
		MapLine* line1;
		MapLine* line2;
		Vec2d    intersect_point;

		Intersection(MapLine* line1, MapLine* line2, double x, double y) :
			line1{ line1 },
			line2{ line2 },
			intersect_point{ x, y }
		{
		}
	};
	vector<Intersection> intersections_;
};


// -----------------------------------------------------------------------------
// LinesOverlapCheck Class
//
// Checks for any overlapping lines (lines that share both vertices)
// -----------------------------------------------------------------------------
class LinesOverlapCheck : public MapCheck
{
public:
	LinesOverlapCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		// Go through lines
		for (unsigned a = 0; a < map_->nLines(); a++)
		{
			auto line1 = map_->line(a);

			// Go through uncompared lines
			for (unsigned b = a + 1; b < map_->nLines(); b++)
			{
				auto line2 = map_->line(b);

				// Check for overlap (both vertices shared)
				if ((line1->v1() == line2->v1() && line1->v2() == line2->v2())
					|| (line1->v2() == line2->v1() && line1->v1() == line2->v2()))
					overlaps_.emplace_back(line1, line2);
			}
		}
	}

	unsigned nProblems() override { return overlaps_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= overlaps_.size())
			return "No overlapping lines found";

		return fmt::format(
			"Lines {} and {} are overlapping", overlaps_[index].line1->index(), overlaps_[index].line2->index());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= overlaps_.size())
			return false;

		if (fix_type == 0)
		{
			auto line1 = overlaps_[index].line1;
			auto line2 = overlaps_[index].line2;

			editor->beginUndoRecord("Merge Lines");

			// Remove first line and correct sectors
			map_->removeLine(line1);
			map_->correctLineSectors(line2);

			editor->endUndoRecord();

			// Remove any overlaps for line1 (since it was removed)
			for (unsigned a = 0; a < overlaps_.size(); a++)
			{
				if (overlaps_[a].line1 == line1 || overlaps_[a].line2 == line1)
				{
					overlaps_.erase(overlaps_.begin() + a);
					a--;
				}
			}

			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index >= overlaps_.size())
			return nullptr;

		return overlaps_[index].line1;
	}

	string progressText() override { return "Checking for overlapping lines..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Merge Lines";

		return "";
	}

private:
	struct Overlap
	{
		MapLine* line1;
		MapLine* line2;

		Overlap(MapLine* line1, MapLine* line2) : line1{ line1 }, line2{ line2 } {}
	};
	vector<Overlap> overlaps_;
};


// -----------------------------------------------------------------------------
// ThingsOverlapCheck Class
//
// Checks for any overlapping things, taking radius and flags into account
// -----------------------------------------------------------------------------
class ThingsOverlapCheck : public MapCheck
{
public:
	ThingsOverlapCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		double r1, r2;

		// Go through things
		for (unsigned a = 0; a < map_->nThings(); a++)
		{
			auto  thing1 = map_->thing(a);
			auto& tt1    = game::configuration().thingType(thing1->type());
			r1           = tt1.radius() - 1;

			// Ignore if no radius
			if (r1 < 0 || !tt1.solid())
				continue;

			// Go through uncompared things
			auto map_format = map_->currentFormat();
			bool udmf_zdoom =
				(map_format == MapFormat::UDMF && strutil::equalCI(game::configuration().udmfNamespace(), "zdoom"));
			bool udmf_eternity =
				(map_format == MapFormat::UDMF && strutil::equalCI(game::configuration().udmfNamespace(), "eternity"));
			int min_skill = udmf_zdoom || udmf_eternity ? 1 : 2;
			int max_skill = udmf_zdoom ? 17 : 5;
			int max_class = udmf_zdoom ? 17 : 4;

			for (unsigned b = a + 1; b < map_->nThings(); b++)
			{
				auto  thing2 = map_->thing(b);
				auto& tt2    = game::configuration().thingType(thing2->type());
				r2           = tt2.radius() - 1;

				// Ignore if no radius
				if (r2 < 0 || !tt2.solid())
					continue;

				// Check flags
				// Case #1: different skill levels
				bool shareflag = false;
				for (int s = min_skill; s < max_skill; ++s)
				{
					auto skill = fmt::format("skill{}", s);
					if (game::configuration().thingBasicFlagSet(skill, thing1, map_format)
						&& game::configuration().thingBasicFlagSet(skill, thing2, map_format))
					{
						shareflag = true;
						s         = max_skill;
					}
				}
				if (!shareflag)
					continue;

				// Booleans for single, coop, deathmatch, and teamgame status for each thing
				bool s1, s2, c1, c2, d1, d2, t1, t2;
				s1 = game::configuration().thingBasicFlagSet("single", thing1, map_format);
				s2 = game::configuration().thingBasicFlagSet("single", thing2, map_format);
				c1 = game::configuration().thingBasicFlagSet("coop", thing1, map_format);
				c2 = game::configuration().thingBasicFlagSet("coop", thing2, map_format);
				d1 = game::configuration().thingBasicFlagSet("dm", thing1, map_format);
				d2 = game::configuration().thingBasicFlagSet("dm", thing2, map_format);
				t1 = t2 = false;

				// Player starts
				// P1 are automatically S and C; P2+ are automatically C;
				// Deathmatch starts are automatically D, and team start are T.
				if (tt1.flags() & game::ThingType::Flags::CoOpStart)
				{
					c1 = true;
					d1 = t1 = false;
					if (thing1->type() == 1)
						s1 = true;
					else
						s1 = false;
				}
				else if (tt1.flags() & game::ThingType::Flags::DMStart)
				{
					s1 = c1 = t1 = false;
					d1           = true;
				}
				else if (tt1.flags() & game::ThingType::Flags::TeamStart)
				{
					s1 = c1 = d1 = false;
					t1           = true;
				}
				if (tt2.flags() & game::ThingType::Flags::CoOpStart)
				{
					c2 = true;
					d2 = t2 = false;
					if (thing2->type() == 1)
						s2 = true;
					else
						s2 = false;
				}
				else if (tt2.flags() & game::ThingType::Flags::DMStart)
				{
					s2 = c2 = t2 = false;
					d2           = true;
				}
				else if (tt2.flags() & game::ThingType::Flags::TeamStart)
				{
					s2 = c2 = d2 = false;
					t2           = true;
				}

				// Case #2: different game modes (single, coop, dm)
				shareflag = false;
				if ((c1 && c2) || (d1 && d2) || (t1 && t2))
				{
					shareflag = true;
				}
				if (!shareflag && s1 && s2)
				{
					// Case #3: things flagged for single player with different class filters
					for (int c = 1; c < max_class; ++c)
					{
						auto pclass = fmt::format("class{}", c);
						if (game::configuration().thingBasicFlagSet(pclass, thing1, map_format)
							&& game::configuration().thingBasicFlagSet(pclass, thing2, map_format))
						{
							shareflag = true;
							c         = max_class;
						}
					}
				}
				if (!shareflag)
					continue;

				// Also check player start spots in Hexen-style hubs
				if (tt1.flags() & game::ThingType::Flags::CoOpStart && tt2.flags() & game::ThingType::Flags::CoOpStart)
				{
					shareflag = false;
					if (thing1->arg(0) == thing2->arg(0))
						shareflag = true;
				}
				if (!shareflag)
					continue;

				// Check x non-overlap
				if (thing2->xPos() + r2 < thing1->xPos() - r1 || thing2->xPos() - r2 > thing1->xPos() + r1)
					continue;

				// Check y non-overlap
				if (thing2->yPos() + r2 < thing1->yPos() - r1 || thing2->yPos() - r2 > thing1->yPos() + r1)
					continue;

				// Overlap detected
				overlaps_.emplace_back(thing1, thing2);
			}
		}
	}

	unsigned nProblems() override { return overlaps_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= overlaps_.size())
			return "No overlapping things found";

		return fmt::format(
			"Things {} and {} are overlapping", overlaps_[index].thing1->index(), overlaps_[index].thing2->index());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= overlaps_.size())
			return false;

		// Get thing to remove (depending on fix)
		MapThing* thing = nullptr;
		if (fix_type == 0)
			thing = overlaps_[index].thing1;
		else if (fix_type == 1)
			thing = overlaps_[index].thing2;

		if (thing)
		{
			// Remove thing
			editor->beginUndoRecord("Delete Thing", false, false, true);
			map_->removeThing(thing);
			editor->endUndoRecord();

			// Clear any overlaps involving the removed thing
			for (unsigned a = 0; a < overlaps_.size(); a++)
			{
				if (overlaps_[a].thing1 == thing || overlaps_[a].thing2 == thing)
				{
					overlaps_.erase(overlaps_.begin() + a);
					a--;
				}
			}

			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index >= overlaps_.size())
			return nullptr;

		return overlaps_[index].thing1;
	}

	string progressText() override { return "Checking for overlapping things..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (index >= overlaps_.size())
			return "";

		if (fix_type == 0)
			return fmt::format("Delete Thing #{}", overlaps_[index].thing1->index());
		if (fix_type == 1)
			return fmt::format("Delete Thing #{}", overlaps_[index].thing2->index());

		return "";
	}

private:
	struct Overlap
	{
		MapThing* thing1;
		MapThing* thing2;
		Overlap(MapThing* thing1, MapThing* thing2) : thing1{ thing1 }, thing2{ thing2 } {}
	};
	vector<Overlap> overlaps_;
};


// -----------------------------------------------------------------------------
// UnknownTexturesCheck Class
//
// Checks for any unknown/invalid textures on lines
// -----------------------------------------------------------------------------
class UnknownTexturesCheck : public MapCheck
{
public:
	UnknownTexturesCheck(SLADEMap* map, MapTextureManager* texman) : MapCheck(map), texman_{ texman } {}

	void doCheck() override
	{
		bool mixed = game::configuration().featureSupported(game::Feature::MixTexFlats);

		// Go through lines
		for (unsigned a = 0; a < map_->nLines(); a++)
		{
			auto line = map_->line(a);

			// Check front side textures
			if (line->s1())
			{
				// Get textures
				auto upper  = line->s1()->texUpper();
				auto middle = line->s1()->texMiddle();
				auto lower  = line->s1()->texLower();

				// Upper
				if (upper != "-" && texman_->texture(upper, mixed).gl_id == gl::Texture::missingTexture())
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontUpper);
				}

				// Middle
				if (middle != "-" && texman_->texture(middle, mixed).gl_id == gl::Texture::missingTexture())
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontMiddle);
				}

				// Lower
				if (lower != "-" && texman_->texture(lower, mixed).gl_id == gl::Texture::missingTexture())
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontLower);
				}
			}

			// Check back side textures
			if (line->s2())
			{
				// Get textures
				auto upper  = line->s2()->texUpper();
				auto middle = line->s2()->texMiddle();
				auto lower  = line->s2()->texLower();

				// Upper
				if (upper != "-" && texman_->texture(upper, mixed).gl_id == gl::Texture::missingTexture())
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackUpper);
				}

				// Middle
				if (middle != "-" && texman_->texture(middle, mixed).gl_id == gl::Texture::missingTexture())
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackMiddle);
				}

				// Lower
				if (lower != "-" && texman_->texture(lower, mixed).gl_id == gl::Texture::missingTexture())
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackLower);
				}
			}
		}
	}

	unsigned nProblems() override { return lines_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= lines_.size())
			return "No unknown wall textures found";

		string line = fmt::format("Line {} has unknown ", lines_[index]->index());
		switch (parts_[index])
		{
		case MapLine::Part::FrontUpper:
			line += fmt::format("front upper texture \"{}\"", lines_[index]->s1()->texUpper());
			break;
		case MapLine::Part::FrontMiddle:
			line += fmt::format("front middle texture \"{}\"", lines_[index]->s1()->texMiddle());
			break;
		case MapLine::Part::FrontLower:
			line += fmt::format("front lower texture \"{}\"", lines_[index]->s1()->texLower());
			break;
		case MapLine::Part::BackUpper:
			line += fmt::format("back upper texture \"{}\"", lines_[index]->s2()->texUpper());
			break;
		case MapLine::Part::BackMiddle:
			line += fmt::format("back middle texture \"{}\"", lines_[index]->s2()->texMiddle());
			break;
		case MapLine::Part::BackLower:
			line += fmt::format("back lower texture \"{}\"", lines_[index]->s2()->texLower());
			break;
		default: break;
		}

		return line;
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= lines_.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(mapeditor::windowWx(), mapeditor::TextureType::Texture, "-", map_);
			if (browser.ShowModal() == wxID_OK)
			{
				// Set texture if one selected
				auto texture = browser.selectedItem()->name();
				editor->beginUndoRecord("Change Texture", true, false, false);
				switch (parts_[index])
				{
				case MapLine::Part::FrontUpper: lines_[index]->setStringProperty("side1.texturetop", texture); break;
				case MapLine::Part::FrontMiddle:
					lines_[index]->setStringProperty("side1.texturemiddle", texture);
					break;
				case MapLine::Part::FrontLower: lines_[index]->setStringProperty("side1.texturebottom", texture); break;
				case MapLine::Part::BackUpper:  lines_[index]->setStringProperty("side2.texturetop", texture); break;
				case MapLine::Part::BackMiddle: lines_[index]->setStringProperty("side2.texturemiddle", texture); break;
				case MapLine::Part::BackLower:  lines_[index]->setStringProperty("side2.texturebottom", texture); break;
				default:                        return false;
				}

				editor->endUndoRecord();

				// Remove problem
				lines_.erase(lines_.begin() + index);
				parts_.erase(parts_.begin() + index);
				return true;
			}

			return false;
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index >= lines_.size())
			return nullptr;

		return lines_[index];
	}

	string progressText() override { return "Checking for unknown wall textures..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Browse Texture...";

		return "";
	}

private:
	MapTextureManager* texman_ = nullptr;
	vector<MapLine*>   lines_;
	vector<int>        parts_;
};


// -----------------------------------------------------------------------------
// UnknownFlatsCheck Class
//
// Checks for any unknown/invalid flats on sectors
// -----------------------------------------------------------------------------
class UnknownFlatsCheck : public MapCheck
{
public:
	UnknownFlatsCheck(SLADEMap* map, MapTextureManager* texman) : MapCheck(map), texman_{ texman } {}

	void doCheck() override
	{
		bool mixed = game::configuration().featureSupported(game::Feature::MixTexFlats);

		// Go through sectors
		for (unsigned a = 0; a < map_->nSectors(); a++)
		{
			// Check floor texture
			if (texman_->flat(map_->sector(a)->floor().texture, mixed).gl_id == gl::Texture::missingTexture())
			{
				sectors_.push_back(map_->sector(a));
				floor_.push_back(true);
			}

			// Check ceiling texture
			if (texman_->flat(map_->sector(a)->ceiling().texture, mixed).gl_id == gl::Texture::missingTexture())
			{
				sectors_.push_back(map_->sector(a));
				floor_.push_back(false);
			}
		}
	}

	unsigned nProblems() override { return sectors_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= sectors_.size())
			return "No unknown flats found";

		auto sector = sectors_[index];
		if (floor_[index])
			return fmt::format("Sector {} has unknown floor texture \"{}\"", sector->index(), sector->floor().texture);
		else
			return fmt::format(
				"Sector {} has unknown ceiling texture \"{}\"", sector->index(), sector->ceiling().texture);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= sectors_.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(mapeditor::windowWx(), mapeditor::TextureType::Flat, "", map_);
			if (browser.ShowModal() == wxID_OK)
			{
				// Set texture if one selected
				auto texture = browser.selectedItem()->name();
				editor->beginUndoRecord("Change Texture");
				if (floor_[index])
					sectors_[index]->setFloorTexture(texture);
				else
					sectors_[index]->setCeilingTexture(texture);

				editor->endUndoRecord();

				// Remove problem
				sectors_.erase(sectors_.begin() + index);
				floor_.erase(floor_.begin() + index);
				return true;
			}

			return false;
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index >= sectors_.size())
			return nullptr;

		return sectors_[index];
	}

	string progressText() override { return "Checking for unknown flats..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Browse Texture...";

		return "";
	}

private:
	MapTextureManager* texman_ = nullptr;
	vector<MapSector*> sectors_;
	vector<bool>       floor_;
};


// -----------------------------------------------------------------------------
// UnknownThingTypesCheck Class
//
// Checks for any things with an unknown type
// -----------------------------------------------------------------------------
class UnknownThingTypesCheck : public MapCheck
{
public:
	UnknownThingTypesCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		for (unsigned a = 0; a < map_->nThings(); a++)
		{
			auto& tt = game::configuration().thingType(map_->thing(a)->type());
			if (!tt.defined())
				things_.push_back(map_->thing(a));
		}
	}

	unsigned nProblems() override { return things_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= things_.size())
			return "No unknown thing types found";

		return fmt::format("Thing {} has unknown type {}", things_[index]->index(), things_[index]->type());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= things_.size())
			return false;

		if (fix_type == 0)
		{
			ThingTypeBrowser browser(mapeditor::windowWx());
			if (browser.ShowModal() == wxID_OK)
			{
				editor->beginUndoRecord("Change Thing Type");
				things_[index]->setIntProperty("type", browser.selectedType());
				things_.erase(things_.begin() + index);
				editor->endUndoRecord();

				return true;
			}
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index >= things_.size())
			return nullptr;

		return things_[index];
	}

	string progressText() override { return "Checking for unknown thing types..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Browse Type...";

		return "";
	}

private:
	vector<MapThing*> things_;
};


// -----------------------------------------------------------------------------
// StuckThingsCheck Class
//
// Checks for any things that are stuck inside (overlapping) solid lines
// -----------------------------------------------------------------------------
class StuckThingsCheck : public MapCheck
{
public:
	StuckThingsCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		double radius;

		// Get list of lines to check
		vector<MapLine*> check_lines;
		MapLine*         line;
		for (unsigned a = 0; a < map_->nLines(); a++)
		{
			line = map_->line(a);

			// Skip if line is 2-sided and not blocking
			if (line->s2() && !game::configuration().lineBasicFlagSet("blocking", line, map_->currentFormat()))
				continue;

			check_lines.push_back(line);
		}

		// Go through things
		for (unsigned a = 0; a < map_->nThings(); a++)
		{
			auto  thing = map_->thing(a);
			auto& tt    = game::configuration().thingType(thing->type());

			// Skip if not a solid thing
			if (!tt.solid())
				continue;

			radius = tt.radius() - 1;
			Rectf bbox(thing->xPos(), thing->yPos(), radius * 2, radius * 2, true);

			// Go through lines
			for (auto& check_line : check_lines)
			{
				line = check_line;

				// Check intersection
				if (geometry::boxLineIntersect(bbox, line->seg()))
				{
					things_.push_back(thing);
					lines_.push_back(line);
					break;
				}
			}
		}
	}

	unsigned nProblems() override { return things_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= things_.size())
			return "No stuck things found";

		return fmt::format("Thing {} is stuck inside line {}", things_[index]->index(), lines_[index]->index());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= things_.size())
			return false;

		if (fix_type == 0)
		{
			auto thing = things_[index];
			auto line  = lines_[index];

			// Get nearest line point to thing
			auto np = geometry::closestPointOnLine(thing->position(), line->seg());

			// Get distance to move
			double r    = game::configuration().thingType(thing->type()).radius();
			double dist = glm::distance(Vec2d(), Vec2d(r, r));

			editor->beginUndoRecord("Move Thing", true, false, false);

			// Move along line direction
			thing->move(np - line->frontVector() * dist);

			editor->endUndoRecord();

			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index >= things_.size())
			return nullptr;

		return things_[index];
	}

	string progressText() override { return "Checking for things stuck in lines..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Move Thing";

		return "";
	}

private:
	vector<MapLine*>  lines_;
	vector<MapThing*> things_;
};


// -----------------------------------------------------------------------------
// SectorReferenceCheck Class
//
// Checks for any possibly incorrect sidedef sector references
// -----------------------------------------------------------------------------
class SectorReferenceCheck : public MapCheck
{
public:
	SectorReferenceCheck(SLADEMap* map) : MapCheck(map) {}

	void checkLine(MapLine* line)
	{
		// Get 'correct' sectors
		auto s1 = map_->lineSideSector(line, true);
		auto s2 = map_->lineSideSector(line, false);

		// Check front sector
		if (s1 != line->frontSector())
			invalid_refs_.emplace_back(line, true, s1);

		// Check back sector
		if (s2 != line->backSector())
			invalid_refs_.emplace_back(line, false, s2);
	}

	void doCheck() override
	{
		// Go through map lines
		for (unsigned a = 0; a < map_->nLines(); a++)
			checkLine(map_->line(a));
	}

	unsigned nProblems() override { return invalid_refs_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= invalid_refs_.size())
			return "No wrong sector references found";

		string side, sector;
		auto   s1 = invalid_refs_[index].line->frontSector();
		auto   s2 = invalid_refs_[index].line->backSector();
		if (invalid_refs_[index].front)
		{
			side   = "front";
			sector = s1 ? fmt::format("{}", s1->index()) : "(none)";
		}
		else
		{
			side   = "back";
			sector = s2 ? fmt::format("{}", s2->index()) : "(none)";
		}

		return fmt::format(
			"Line {} has potentially wrong {} sector {}", invalid_refs_[index].line->index(), side, sector);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= invalid_refs_.size())
			return false;

		if (fix_type == 0)
		{
			editor->beginUndoRecord("Correct Line Sector");

			// Set sector
			auto ref = invalid_refs_[index];
			if (ref.sector)
				map_->setLineSector(ref.line->index(), ref.sector->index(), ref.front);
			else
			{
				// Remove side if no sector
				if (ref.front && ref.line->s1())
					map_->removeSide(ref.line->s1());
				else if (!ref.front && ref.line->s2())
					map_->removeSide(ref.line->s2());
			}

			// Flip line if needed
			if (!ref.line->s1() && ref.line->s2())
				ref.line->flip();

			editor->endUndoRecord();

			// Remove problem (and any others for the line)
			for (unsigned a = 0; a < invalid_refs_.size(); a++)
			{
				if (invalid_refs_[a].line == ref.line)
				{
					invalid_refs_.erase(invalid_refs_.begin() + a);
					a--;
				}
			}

			// Re-check line
			checkLine(ref.line);

			editor->updateDisplay();

			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index < invalid_refs_.size())
			return invalid_refs_[index].line;

		return nullptr;
	}

	string progressText() override { return "Checking sector references..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
		{
			if (index >= invalid_refs_.size())
				return "Fix Sector reference";

			auto sector = invalid_refs_[index].sector;
			if (sector)
				return fmt::format("Set to Sector #{}", sector->index());
			else
				return "Clear Sector";
		}

		return "";
	}

private:
	struct SectorRef
	{
		MapLine*   line;
		bool       front;
		MapSector* sector;
		SectorRef(MapLine* line, bool front, MapSector* sector) : line{ line }, front{ front }, sector{ sector } {}
	};
	vector<SectorRef> invalid_refs_;
};


// -----------------------------------------------------------------------------
// InvalidLineCheck Class
//
// Checks for any invalid lines (that have no first side)
// -----------------------------------------------------------------------------
class InvalidLineCheck : public MapCheck
{
public:
	InvalidLineCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		// Go through map lines
		lines_.clear();
		for (unsigned a = 0; a < map_->nLines(); a++)
		{
			if (!map_->line(a)->s1())
				lines_.push_back(a);
		}
	}

	unsigned nProblems() override { return lines_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= lines_.size())
			return "No invalid lines found";

		if (map_->line(lines_[index])->s2())
			return fmt::format("Line {} has no front side", lines_[index]);
		else
			return fmt::format("Line {} has no sides", lines_[index]);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= lines_.size())
			return false;

		auto line = map_->line(lines_[index]);
		if (line->s2())
		{
			// Flip
			if (fix_type == 0)
			{
				line->flip();
				return true;
			}

			// Create sector
			else if (fix_type == 1)
			{
				auto pos = line->dirTabPoint(0.1);
				editor->edit2D().createSector(pos);
				doCheck();
				return true;
			}
		}
		else
		{
			// Delete
			if (fix_type == 0)
			{
				map_->removeLine(line);
				doCheck();
				return true;
			}

			// Create sector
			else if (fix_type == 1)
			{
				auto pos = line->dirTabPoint(0.1);
				editor->edit2D().createSector(pos);
				doCheck();
				return true;
			}
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index < lines_.size())
			return map_->line(lines_[index]);

		return nullptr;
	}

	string progressText() override { return "Checking for invalid lines..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (map_->line(lines_[index])->s2())
		{
			if (fix_type == 0)
				return "Flip line";
			else if (fix_type == 1)
				return "Create sector";
		}
		else
		{
			if (fix_type == 0)
				return "Delete line";
			else if (fix_type == 1)
				return "Create sector";
		}

		return "";
	}

private:
	vector<int> lines_;
};


// -----------------------------------------------------------------------------
// UnknownSectorCheck Class
//
// Checks for any unknown sector type
// -----------------------------------------------------------------------------
class UnknownSectorCheck : public MapCheck
{
public:
	UnknownSectorCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		// Go through map lines
		sectors_.clear();
		for (unsigned a = 0; a < map_->nSectors(); a++)
		{
			int special = map_->sector(a)->special();
			if (game::configuration().sectorTypeName(special) == "Unknown")
				sectors_.push_back(a);
		}
	}

	unsigned nProblems() override { return sectors_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= sectors_.size())
			return "No unknown sector types found";

		return fmt::format("Sector {} has no sides", sectors_[index]);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= sectors_.size())
			return false;

		auto sec = map_->sector(sectors_[index]);
		if (fix_type == 0)
		{
			// Try to preserve flags if they exist
			int special = sec->special();
			int base    = game::configuration().baseSectorType(special);
			special &= ~base;
			sec->setIntProperty("special", special);
		}
		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index < sectors_.size())
			return map_->sector(sectors_[index]);

		return nullptr;
	}

	string progressText() override { return "Checking for unknown sector types..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Reset sector type";

		return "";
	}

private:
	vector<int> sectors_;
};


// -----------------------------------------------------------------------------
// UnknownSpecialCheck Class
//
// Checks for any unknown special
// -----------------------------------------------------------------------------
class UnknownSpecialCheck : public MapCheck
{
public:
	UnknownSpecialCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		// Go through map lines
		objects_.clear();
		for (unsigned a = 0; a < map_->nLines(); ++a)
		{
			int special = map_->line(a)->special();
			if (game::configuration().actionSpecialName(special) == "Unknown")
				objects_.push_back(map_->line(a));
		}
		// In Hexen or UDMF, go through map things too since they too can have specials
		if (map_->currentFormat() == MapFormat::Hexen || map_->currentFormat() == MapFormat::UDMF)
		{
			for (unsigned a = 0; a < map_->nThings(); ++a)
			{
				// Ignore the Heresiarch which does not have a real special
				auto& tt = game::configuration().thingType(map_->thing(a)->type());
				if (tt.flags() & game::ThingType::Flags::Script)
					continue;

				// Otherwise, check special
				if (game::configuration().actionSpecialName(map_->thing(a)->special()) == "Unknown")
					objects_.push_back(map_->thing(a));
			}
		}
	}

	unsigned nProblems() override { return objects_.size(); }

	string problemDesc(unsigned index) override
	{
		bool special = (map_->currentFormat() == MapFormat::Hexen || map_->currentFormat() == MapFormat::UDMF);
		if (index >= objects_.size())
			return fmt::format("No unknown {} found", special ? "special" : "line type");

		return fmt::format(
			"{} {} has an unknown {}",
			objects_[index]->objType() == MapObject::Type::Line ? "Line" : "Thing",
			objects_[index]->index(),
			special ? "special" : "type");
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= objects_.size())
			return false;

		// Reset
		if (fix_type == 0)
		{
			objects_[index]->setIntProperty("special", 0);
			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index < objects_.size())
			return objects_[index];

		return nullptr;
	}

	string progressText() override { return "Checking for unknown specials..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Reset special";

		return "";
	}

private:
	vector<MapObject*> objects_;
};


// -----------------------------------------------------------------------------
// ObsoleteThingCheck Class
//
// Checks for any things marked as obsolete
// -----------------------------------------------------------------------------
class ObsoleteThingCheck : public MapCheck
{
public:
	ObsoleteThingCheck(SLADEMap* map) : MapCheck(map) {}

	void doCheck() override
	{
		// Go through map lines
		things_.clear();
		for (unsigned a = 0; a < map_->nThings(); ++a)
		{
			auto  thing = map_->thing(a);
			auto& tt    = game::configuration().thingType(thing->type());
			if (tt.flags() & game::ThingType::Flags::Obsolete)
				things_.push_back(thing);
		}
	}

	unsigned nProblems() override { return things_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= things_.size())
			return "No obsolete things found";

		return fmt::format("Thing {} is obsolete", things_[index]->index());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= things_.size())
			return false;

		// Reset
		if (fix_type == 0)
		{
			editor->beginUndoRecord("Delete Thing", false, false, true);
			map_->removeThing(things_[index]);
			editor->endUndoRecord();
			return true;
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index < things_.size())
			return things_[index];

		return nullptr;
	}

	string progressText() override { return "Checking for obsolete things..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (fix_type == 0)
			return "Delete thing";

		return "";
	}

private:
	vector<MapThing*> things_;
};


// -----------------------------------------------------------------------------
//
// MapCheck Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Creates a standard MapCheck of [type], passing [map] and [texman] to the
// constructor where necessary
// -----------------------------------------------------------------------------
unique_ptr<MapCheck> MapCheck::standardCheck(StandardCheck type, SLADEMap* map, MapTextureManager* texman)
{
	switch (type)
	{
	case MissingTexture:   return std::make_unique<MissingTextureCheck>(map);
	case SpecialTag:       return std::make_unique<SpecialTagsCheck>(map);
	case IntersectingLine: return std::make_unique<LinesIntersectCheck>(map);
	case OverlappingLine:  return std::make_unique<LinesOverlapCheck>(map);
	case OverlappingThing: return std::make_unique<ThingsOverlapCheck>(map);
	case UnknownTexture:   return std::make_unique<UnknownTexturesCheck>(map, texman);
	case UnknownFlat:      return std::make_unique<UnknownFlatsCheck>(map, texman);
	case UnknownThingType: return std::make_unique<UnknownThingTypesCheck>(map);
	case StuckThing:       return std::make_unique<StuckThingsCheck>(map);
	case SectorReference:  return std::make_unique<SectorReferenceCheck>(map);
	case InvalidLine:      return std::make_unique<InvalidLineCheck>(map);
	case MissingTagged:    return std::make_unique<MissingTaggedCheck>(map);
	case UnknownSector:    return std::make_unique<UnknownSectorCheck>(map);
	case UnknownSpecial:   return std::make_unique<UnknownSpecialCheck>(map);
	case ObsoleteThing:    return std::make_unique<ObsoleteThingCheck>(map);
	default:               return std::make_unique<MissingTextureCheck>(map);
	}
}

// -----------------------------------------------------------------------------
// Same as above, but taking a string MapCheck type id instead of an enum value
// -----------------------------------------------------------------------------
unique_ptr<MapCheck> MapCheck::standardCheck(string_view type_id, SLADEMap* map, MapTextureManager* texman)
{
	for (auto& check : std_checks)
		if (check.second.id == type_id)
			return standardCheck(check.first, map, texman);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the description of standard MapCheck [type]
// -----------------------------------------------------------------------------
string MapCheck::standardCheckDesc(StandardCheck type)
{
	return std_checks[type].description;
}

// -----------------------------------------------------------------------------
// Returns the string id of standard MapCheck [type]
// -----------------------------------------------------------------------------
string MapCheck::standardCheckId(StandardCheck type)
{
	return std_checks[type].id;
}

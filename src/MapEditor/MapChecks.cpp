
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "General/SAction.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapTextureManager.h"
#include "SLADEMap/SLADEMap.h"
#include "UI/Dialogs/MapTextureBrowser.h"
#include "UI/Dialogs/ThingTypeBrowser.h"
#include "Utility/MathStuff.h"


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
	{ MapCheck::OverlappingThing, { "unknown_texture", "Unknown wall textures" } },
	{ MapCheck::UnknownTexture, { "unknown_flat", "Unknown flat textures" } },
	{ MapCheck::UnknownFlat, { "unknown_thing", "Unknown thing types" } },
	{ MapCheck::UnknownThingType, { "overlapping_thing", "Overlapping things" } },
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
		string sky_flat = Game::configuration().skyFlat();
		for (unsigned a = 0; a < map_->nLines(); a++)
		{
			// Check what textures the line needs
			MapLine* line  = map_->getLine(a);
			MapSide* side1 = line->s1();
			MapSide* side2 = line->s2();
			int      needs = line->needsTexture();

			// Detect if sky hack might apply
			bool sky_hack = false;
			if (side1 && S_CMPNOCASE(sky_flat, side1->getSector()->ceiling().texture) && side2
				&& S_CMPNOCASE(sky_flat, side2->getSector()->ceiling().texture))
				sky_hack = true;

			// Check for missing textures (front side)
			if (side1)
			{
				// Upper
				if ((needs & MapLine::Part::FrontUpper) > 0 && side1->stringProperty("texturetop") == "-" && !sky_hack)
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontUpper);
				}

				// Middle
				if ((needs & MapLine::Part::FrontMiddle) > 0 && side1->stringProperty("texturemiddle") == "-")
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontMiddle);
				}

				// Lower
				if ((needs & MapLine::Part::FrontLower) > 0 && side1->stringProperty("texturebottom") == "-")
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontLower);
				}
			}

			// Check for missing textures (back side)
			if (side2)
			{
				// Upper
				if ((needs & MapLine::Part::BackUpper) > 0 && side2->stringProperty("texturetop") == "-" && !sky_hack)
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackUpper);
				}

				// Middle
				if ((needs & MapLine::Part::BackMiddle) > 0 && side2->stringProperty("texturemiddle") == "-")
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackMiddle);
				}

				// Lower
				if ((needs & MapLine::Part::BackLower) > 0 && side2->stringProperty("texturebottom") == "-")
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackLower);
				}
			}
		}

		LOG_MESSAGE(3, "Missing Texture Check: %lu missing textures", parts_.size());
	}

	unsigned nProblems() override { return lines_.size(); }

	string texName(int part)
	{
		switch (part)
		{
		case MapLine::Part::FrontUpper: return "front upper texture";
		case MapLine::Part::FrontMiddle: return "front middle texture";
		case MapLine::Part::FrontLower: return "front lower texture";
		case MapLine::Part::BackUpper: return "back upper texture";
		case MapLine::Part::BackMiddle: return "back middle texture";
		case MapLine::Part::BackLower: return "back lower texture";
		}

		return "";
	}

	string problemDesc(unsigned index) override
	{
		if (index < lines_.size())
			return S_FMT("Line %d missing %s", lines_[index]->getIndex(), texName(parts_[index]));
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
			MapTextureBrowser browser(MapEditor::windowWx(), 0, "-", map_);
			if (browser.ShowModal() == wxID_OK)
			{
				editor->beginUndoRecord("Change Texture", true, false, false);

				// Set texture if one selected
				string texture = browser.getSelectedItem()->name();
				switch (parts_[index])
				{
				case MapLine::Part::FrontUpper: lines_[index]->setStringProperty("side1.texturetop", texture); break;
				case MapLine::Part::FrontMiddle:
					lines_[index]->setStringProperty("side1.texturemiddle", texture);
					break;
				case MapLine::Part::FrontLower: lines_[index]->setStringProperty("side1.texturebottom", texture); break;
				case MapLine::Part::BackUpper: lines_[index]->setStringProperty("side2.texturetop", texture); break;
				case MapLine::Part::BackMiddle: lines_[index]->setStringProperty("side2.texturemiddle", texture); break;
				case MapLine::Part::BackLower: lines_[index]->setStringProperty("side2.texturebottom", texture); break;
				default: return false;
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
		using Game::TagType;

		for (unsigned a = 0; a < map_->nLines(); a++)
		{
			// Get special and tag
			int special = map_->getLine(a)->intProperty("special");
			int tag     = map_->getLine(a)->intProperty("arg0");

			// Get action special
			auto tagged = Game::configuration().actionSpecial(special).needsTag();

			// Check for back sector that removes need for tagged sector
			if ((tagged == TagType::Back || tagged == TagType::SectorOrBack) && map_->getLine(a)->backSector())
				continue;

			// Check if tag is required but not set
			if (tagged != TagType::None && tag == 0)
				objects_.push_back(map_->getLine(a));
		}
		// Hexen and UDMF allow specials on things
		if (map_->currentFormat() == MAP_HEXEN || map_->currentFormat() == MAP_UDMF)
		{
			for (unsigned a = 0; a < map_->nThings(); ++a)
			{
				// Ignore the Heresiarch which does not have a real special
				auto& tt = Game::configuration().thingType(map_->getThing(a)->getType());
				if (tt.flags() & Game::ThingType::FLAG_SCRIPT)
					continue;

				// Get special and tag
				int special = map_->getThing(a)->intProperty("special");
				int tag     = map_->getThing(a)->intProperty("arg0");

				// Get action special
				auto tagged = Game::configuration().actionSpecial(special).needsTag();

				// Check if tag is required but not set
				if (tagged != TagType::None && tagged != TagType::Back && tag == 0)
					objects_.push_back(map_->getThing(a));
			}
		}
	}

	unsigned nProblems() override { return objects_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= objects_.size())
			return "No missing special tags found";

		MapObject* mo      = objects_[index];
		int        special = mo->intProperty("special");
		return S_FMT(
			"%s %d: Special %d (%s) requires a tag",
			mo->getObjType() == MapObject::Type::Line ? "Line" : "Thing",
			mo->getIndex(),
			special,
			Game::configuration().actionSpecial(special).name());
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
		using Game::TagType;

		unsigned nlines  = map_->nLines();
		unsigned nthings = 0;
		if (map_->currentFormat() == MAP_HEXEN || map_->currentFormat() == MAP_UDMF)
			nthings = map_->nThings();
		for (unsigned a = 0; a < (nlines + nthings); a++)
		{
			MapObject* mo        = nullptr;
			bool       thingmode = false;
			if (a >= nlines)
			{
				mo        = map_->getThing(a - nlines);
				thingmode = true;
			}
			else
				mo = map_->getLine(a);

			// Ignore the Heresiarch which does not have a real special
			if (thingmode)
			{
				auto& tt = Game::configuration().thingType(((MapThing*)mo)->getType());
				if (tt.flags() & Game::ThingType::FLAG_SCRIPT)
					continue;
			}

			// Get special and tag
			int special = mo->intProperty("special");
			int tag     = mo->intProperty("arg0");

			// Get action special
			auto tagged = Game::configuration().actionSpecial(special).needsTag();

			// Check if tag is required and set (not set is a different check...)
			if (tagged != TagType::None && tag != 0)
			{
				bool okay = false;
				switch (tagged)
				{
				case TagType::Sector:
				case TagType::SectorOrBack:
				{
					std::vector<MapSector*> foundsectors;
					map_->getSectorsByTag(tag, foundsectors);
					if (foundsectors.size() > 0)
						okay = true;
				}
				break;
				case TagType::Line:
				{
					std::vector<MapLine*> foundlines;
					map_->getLinesById(tag, foundlines);
					if (foundlines.size() > 0)
						okay = true;
				}
				break;
				case TagType::Thing:
				{
					std::vector<MapThing*> foundthings;
					map_->getThingsById(tag, foundthings);
					if (foundthings.size() > 0)
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
		return S_FMT(
			"%s %d: No object tagged %d for Special %d (%s)",
			objects_[index]->getObjType() == MapObject::Type::Line ? "Line" : "Thing",
			objects_[index]->getIndex(),
			objects_[index]->intProperty("arg0"),
			special,
			Game::configuration().actionSpecial(special).name());
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

	void checkIntersections(vector<MapLine*> lines)
	{
		double   x, y;
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
				if (map_->linesIntersect(line1, line2, x, y))
					intersections_.push_back(Intersection(line1, line2, x, y));
			}
		}
	}

	void doCheck() override
	{
		// Get all map lines
		vector<MapLine*> all_lines;
		for (unsigned a = 0; a < map_->nLines(); a++)
			all_lines.push_back(map_->getLine(a));

		// Check for intersections
		checkIntersections(all_lines);
	}

	unsigned nProblems() override { return intersections_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= intersections_.size())
			return "No intersecting lines found";

		return S_FMT(
			"Lines %d and %d are intersecting at (%1.2f, %1.2f)",
			intersections_[index].line1->getIndex(),
			intersections_[index].line2->getIndex(),
			intersections_[index].intersect_point.x,
			intersections_[index].intersect_point.y);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= intersections_.size())
			return false;

		if (fix_type == 0)
		{
			MapLine* line1 = intersections_[index].line1;
			MapLine* line2 = intersections_[index].line2;

			editor->beginUndoRecord("Split Lines");

			// Create split vertex
			MapVertex* nv = map_->createVertex(
				intersections_[index].intersect_point.x, intersections_[index].intersect_point.y, -1);

			// Split first line
			map_->splitLine(line1, nv);
			MapLine* nl1 = map_->getLine(map_->nLines() - 1);

			// Split second line
			map_->splitLine(line2, nv);
			MapLine* nl2 = map_->getLine(map_->nLines() - 1);

			// Remove intersection
			intersections_.erase(intersections_.begin() + index);

			editor->endUndoRecord();

			// Create list of lines to re-check
			vector<MapLine*> lines;
			lines.push_back(line1);
			lines.push_back(line2);
			lines.push_back(nl1);
			lines.push_back(nl2);
			for (unsigned a = 0; a < intersections_.size(); a++)
			{
				VECTOR_ADD_UNIQUE(lines, intersections_[a].line1);
				VECTOR_ADD_UNIQUE(lines, intersections_[a].line2);
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
		MapLine*  line1;
		MapLine*  line2;
		fpoint2_t intersect_point;

		Intersection(MapLine* line1, MapLine* line2, double x, double y)
		{
			this->line1 = line1;
			this->line2 = line2;
			intersect_point.set(x, y);
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
			MapLine* line1 = map_->getLine(a);

			// Go through uncompared lines
			for (unsigned b = a + 1; b < map_->nLines(); b++)
			{
				MapLine* line2 = map_->getLine(b);

				// Check for overlap (both vertices shared)
				if ((line1->v1() == line2->v1() && line1->v2() == line2->v2())
					|| (line1->v2() == line2->v1() && line1->v1() == line2->v2()))
					overlaps_.push_back(Overlap(line1, line2));
			}
		}
	}

	unsigned nProblems() override { return overlaps_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= overlaps_.size())
			return "No overlapping lines found";

		return S_FMT(
			"Lines %d and %d are overlapping", overlaps_[index].line1->getIndex(), overlaps_[index].line2->getIndex());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= overlaps_.size())
			return false;

		if (fix_type == 0)
		{
			MapLine* line1 = overlaps_[index].line1;
			MapLine* line2 = overlaps_[index].line2;

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

		Overlap(MapLine* line1, MapLine* line2)
		{
			this->line1 = line1;
			this->line2 = line2;
		}
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
			MapThing* thing1 = map_->getThing(a);
			auto&     tt1    = Game::configuration().thingType(thing1->getType());
			r1               = tt1.radius() - 1;

			// Ignore if no radius
			if (r1 < 0 || !tt1.solid())
				continue;

			// Go through uncompared things
			int  map_format = map_->currentFormat();
			bool udmf_zdoom = (map_format == MAP_UDMF && S_CMPNOCASE(Game::configuration().udmfNamespace(), "zdoom"));
			bool udmf_eternity =
				(map_format == MAP_UDMF && S_CMPNOCASE(Game::configuration().udmfNamespace(), "eternity"));
			int min_skill = udmf_zdoom || udmf_eternity ? 1 : 2;
			int max_skill = udmf_zdoom ? 17 : 5;
			int max_class = udmf_zdoom ? 17 : 4;

			for (unsigned b = a + 1; b < map_->nThings(); b++)
			{
				MapThing* thing2 = map_->getThing(b);
				auto&     tt2    = Game::configuration().thingType(thing2->getType());
				r2               = tt2.radius() - 1;

				// Ignore if no radius
				if (r2 < 0 || !tt2.solid())
					continue;

				// Check flags
				// Case #1: different skill levels
				bool shareflag = false;
				for (int s = min_skill; s < max_skill; ++s)
				{
					string skill = S_FMT("skill%d", s);
					if (Game::configuration().thingBasicFlagSet(skill, thing1, map_format)
						&& Game::configuration().thingBasicFlagSet(skill, thing2, map_format))
					{
						shareflag = true;
						s         = max_skill;
					}
				}
				if (!shareflag)
					continue;

				// Booleans for single, coop, deathmatch, and teamgame status for each thing
				bool s1, s2, c1, c2, d1, d2, t1, t2;
				s1 = Game::configuration().thingBasicFlagSet("single", thing1, map_format);
				s2 = Game::configuration().thingBasicFlagSet("single", thing2, map_format);
				c1 = Game::configuration().thingBasicFlagSet("coop", thing1, map_format);
				c2 = Game::configuration().thingBasicFlagSet("coop", thing2, map_format);
				d1 = Game::configuration().thingBasicFlagSet("dm", thing1, map_format);
				d2 = Game::configuration().thingBasicFlagSet("dm", thing2, map_format);

				// Player starts
				// P1 are automatically S and C; P2+ are automatically C;
				// Deathmatch starts are automatically D, and team start are T.
				if (tt1.flags() & Game::ThingType::FLAG_COOPSTART)
				{
					c1 = true;
					d1 = t1 = false;
					if (thing1->getType() == 1)
						s1 = true;
					else
						s1 = false;
				}
				else if (tt1.flags() & Game::ThingType::FLAG_DMSTART)
				{
					s1 = c1 = t1 = false;
					d1           = true;
				}
				else if (tt1.flags() & Game::ThingType::FLAG_TEAMSTART)
				{
					s1 = c1 = d1 = false;
					t1           = true;
				}
				if (tt2.flags() & Game::ThingType::FLAG_COOPSTART)
				{
					c2 = true;
					d2 = t2 = false;
					if (thing2->getType() == 1)
						s2 = true;
					else
						s2 = false;
				}
				else if (tt2.flags() & Game::ThingType::FLAG_DMSTART)
				{
					s2 = c2 = t2 = false;
					d2           = true;
				}
				else if (tt2.flags() & Game::ThingType::FLAG_TEAMSTART)
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
						string pclass = S_FMT("class%d", c);
						if (Game::configuration().thingBasicFlagSet(pclass, thing1, map_format)
							&& Game::configuration().thingBasicFlagSet(pclass, thing2, map_format))
						{
							shareflag = true;
							c         = max_class;
						}
					}
				}
				if (!shareflag)
					continue;

				// Also check player start spots in Hexen-style hubs
				shareflag = false;
				if (tt1.flags() & Game::ThingType::FLAG_COOPSTART && tt2.flags() & Game::ThingType::FLAG_COOPSTART)
				{
					if (thing1->intProperty("arg0") == thing2->intProperty("arg0"))
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
				overlaps_.push_back(Overlap(thing1, thing2));
			}
		}
	}

	unsigned nProblems() override { return overlaps_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= overlaps_.size())
			return "No overlapping things found";

		return S_FMT(
			"Things %d and %d are overlapping",
			overlaps_[index].thing1->getIndex(),
			overlaps_[index].thing2->getIndex());
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
			return S_FMT("Delete Thing #%d", overlaps_[index].thing1->getIndex());
		if (fix_type == 1)
			return S_FMT("Delete Thing #%d", overlaps_[index].thing2->getIndex());

		return "";
	}

private:
	struct Overlap
	{
		MapThing* thing1;
		MapThing* thing2;
		Overlap(MapThing* thing1, MapThing* thing2)
		{
			this->thing1 = thing1;
			this->thing2 = thing2;
		}
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
	UnknownTexturesCheck(SLADEMap* map, MapTextureManager* texman) : MapCheck(map) { this->texman_ = texman; }

	void doCheck() override
	{
		bool mixed = Game::configuration().featureSupported(Game::Feature::MixTexFlats);

		// Go through lines
		for (unsigned a = 0; a < map_->nLines(); a++)
		{
			MapLine* line = map_->getLine(a);

			// Check front side textures
			if (line->s1())
			{
				// Get textures
				string upper  = line->s1()->stringProperty("texturetop");
				string middle = line->s1()->stringProperty("texturemiddle");
				string lower  = line->s1()->stringProperty("texturebottom");

				// Upper
				if (upper != "-" && texman_->getTexture(upper, mixed) == &(GLTexture::missingTex()))
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontUpper);
				}

				// Middle
				if (middle != "-" && texman_->getTexture(middle, mixed) == &(GLTexture::missingTex()))
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontMiddle);
				}

				// Lower
				if (lower != "-" && texman_->getTexture(lower, mixed) == &(GLTexture::missingTex()))
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::FrontLower);
				}
			}

			// Check back side textures
			if (line->s2())
			{
				// Get textures
				string upper  = line->s2()->stringProperty("texturetop");
				string middle = line->s2()->stringProperty("texturemiddle");
				string lower  = line->s2()->stringProperty("texturebottom");

				// Upper
				if (upper != "-" && texman_->getTexture(upper, mixed) == &(GLTexture::missingTex()))
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackUpper);
				}

				// Middle
				if (middle != "-" && texman_->getTexture(middle, mixed) == &(GLTexture::missingTex()))
				{
					lines_.push_back(line);
					parts_.push_back(MapLine::Part::BackMiddle);
				}

				// Lower
				if (lower != "-" && texman_->getTexture(lower, mixed) == &(GLTexture::missingTex()))
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

		string line = S_FMT("Line %d has unknown ", lines_[index]->getIndex());
		switch (parts_[index])
		{
		case MapLine::Part::FrontUpper:
			line += S_FMT("front upper texture \"%s\"", lines_[index]->s1()->stringProperty("texturetop"));
			break;
		case MapLine::Part::FrontMiddle:
			line += S_FMT("front middle texture \"%s\"", lines_[index]->s1()->stringProperty("texturemiddle"));
			break;
		case MapLine::Part::FrontLower:
			line += S_FMT("front lower texture \"%s\"", lines_[index]->s1()->stringProperty("texturebottom"));
			break;
		case MapLine::Part::BackUpper:
			line += S_FMT("back upper texture \"%s\"", lines_[index]->s2()->stringProperty("texturetop"));
			break;
		case MapLine::Part::BackMiddle:
			line += S_FMT("back middle texture \"%s\"", lines_[index]->s2()->stringProperty("texturemiddle"));
			break;
		case MapLine::Part::BackLower:
			line += S_FMT("back lower texture \"%s\"", lines_[index]->s2()->stringProperty("texturebottom"));
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
			MapTextureBrowser browser(MapEditor::windowWx(), 0, "-", map_);
			if (browser.ShowModal() == wxID_OK)
			{
				// Set texture if one selected
				string texture = browser.getSelectedItem()->name();
				editor->beginUndoRecord("Change Texture", true, false, false);
				switch (parts_[index])
				{
				case MapLine::Part::FrontUpper: lines_[index]->setStringProperty("side1.texturetop", texture); break;
				case MapLine::Part::FrontMiddle:
					lines_[index]->setStringProperty("side1.texturemiddle", texture);
					break;
				case MapLine::Part::FrontLower: lines_[index]->setStringProperty("side1.texturebottom", texture); break;
				case MapLine::Part::BackUpper: lines_[index]->setStringProperty("side2.texturetop", texture); break;
				case MapLine::Part::BackMiddle: lines_[index]->setStringProperty("side2.texturemiddle", texture); break;
				case MapLine::Part::BackLower: lines_[index]->setStringProperty("side2.texturebottom", texture); break;
				default: return false;
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
	MapTextureManager* texman_;
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
	UnknownFlatsCheck(SLADEMap* map, MapTextureManager* texman) : MapCheck(map) { this->texman_ = texman; }

	void doCheck() override
	{
		bool mixed = Game::configuration().featureSupported(Game::Feature::MixTexFlats);

		// Go through sectors
		for (unsigned a = 0; a < map_->nSectors(); a++)
		{
			// Check floor texture
			if (texman_->getFlat(map_->getSector(a)->floor().texture, mixed) == &(GLTexture::missingTex()))
			{
				sectors_.push_back(map_->getSector(a));
				floor_.push_back(true);
			}

			// Check ceiling texture
			if (texman_->getFlat(map_->getSector(a)->ceiling().texture, mixed) == &(GLTexture::missingTex()))
			{
				sectors_.push_back(map_->getSector(a));
				floor_.push_back(false);
			}
		}
	}

	unsigned nProblems() override { return sectors_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= sectors_.size())
			return "No unknown flats found";

		MapSector* sector = sectors_[index];
		if (floor_[index])
			return S_FMT("Sector %d has unknown floor texture \"%s\"", sector->getIndex(), sector->floor().texture);
		else
			return S_FMT("Sector %d has unknown ceiling texture \"%s\"", sector->getIndex(), sector->ceiling().texture);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= sectors_.size())
			return false;

		if (fix_type == 0)
		{
			// Browse textures
			MapTextureBrowser browser(MapEditor::windowWx(), 1, "", map_);
			if (browser.ShowModal() == wxID_OK)
			{
				// Set texture if one selected
				string texture = browser.getSelectedItem()->name();
				editor->beginUndoRecord("Change Texture");
				if (floor_[index])
					sectors_[index]->setStringProperty("texturefloor", texture);
				else
					sectors_[index]->setStringProperty("textureceiling", texture);

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
	MapTextureManager* texman_;
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
			auto& tt = Game::configuration().thingType(map_->getThing(a)->getType());
			if (!tt.defined())
				things_.push_back(map_->getThing(a));
		}
	}

	unsigned nProblems() override { return things_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= things_.size())
			return "No unknown thing types found";

		return S_FMT("Thing %d has unknown type %d", things_[index]->getIndex(), things_[index]->getType());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= things_.size())
			return false;

		if (fix_type == 0)
		{
			ThingTypeBrowser browser(MapEditor::windowWx());
			if (browser.ShowModal() == wxID_OK)
			{
				editor->beginUndoRecord("Change Thing Type");
				things_[index]->setIntProperty("type", browser.getSelectedType());
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
			line = map_->getLine(a);

			// Skip if line is 2-sided and not blocking
			if (line->s2() && !Game::configuration().lineBasicFlagSet("blocking", line, map_->currentFormat()))
				continue;

			check_lines.push_back(line);
		}

		// Go through things
		for (unsigned a = 0; a < map_->nThings(); a++)
		{
			MapThing* thing = map_->getThing(a);
			auto&     tt    = Game::configuration().thingType(thing->getType());

			// Skip if not a solid thing
			if (!tt.solid())
				continue;

			radius = tt.radius() - 1;
			frect_t bbox(thing->xPos(), thing->yPos(), radius * 2, radius * 2, 1);

			// Go through lines
			for (unsigned b = 0; b < check_lines.size(); b++)
			{
				line = check_lines[b];

				// Check intersection
				if (MathStuff::boxLineIntersect(bbox, line->seg()))
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

		return S_FMT("Thing %d is stuck inside line %d", things_[index]->getIndex(), lines_[index]->getIndex());
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= things_.size())
			return false;

		if (fix_type == 0)
		{
			MapThing* thing = things_[index];
			MapLine*  line  = lines_[index];

			// Get nearest line point to thing
			fpoint2_t np = MathStuff::closestPointOnLine(thing->point(), line->seg());

			// Get distance to move
			double r    = Game::configuration().thingType(thing->getType()).radius();
			double dist = MathStuff::distance(fpoint2_t(), fpoint2_t(r, r));

			editor->beginUndoRecord("Move Thing", true, false, false);

			// Move along line direction
			map_->moveThing(
				thing->getIndex(), np.x - (line->frontVector().x * dist), np.y - (line->frontVector().y * dist));

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
		MapSector* s1 = map_->getLineSideSector(line, true);
		MapSector* s2 = map_->getLineSideSector(line, false);

		// Check front sector
		if (s1 != line->frontSector())
			invalid_refs_.push_back(SectorRef(line, true, s1));

		// Check back sector
		if (s2 != line->backSector())
			invalid_refs_.push_back(SectorRef(line, false, s2));
	}

	void doCheck() override
	{
		// Go through map lines
		for (unsigned a = 0; a < map_->nLines(); a++)
			checkLine(map_->getLine(a));
	}

	unsigned nProblems() override { return invalid_refs_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= invalid_refs_.size())
			return "No wrong sector references found";

		string     side, sector;
		MapSector* s1 = invalid_refs_[index].line->frontSector();
		MapSector* s2 = invalid_refs_[index].line->backSector();
		if (invalid_refs_[index].front)
		{
			side   = "front";
			sector = s1 ? S_FMT("%d", s1->getIndex()) : "(none)";
		}
		else
		{
			side   = "back";
			sector = s2 ? S_FMT("%d", s2->getIndex()) : "(none)";
		}

		return S_FMT("Line %d has potentially wrong %s sector %s", invalid_refs_[index].line->getIndex(), side, sector);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= invalid_refs_.size())
			return false;

		if (fix_type == 0)
		{
			editor->beginUndoRecord("Correct Line Sector");

			// Set sector
			SectorRef ref = invalid_refs_[index];
			if (ref.sector)
				map_->setLineSector(ref.line->getIndex(), ref.sector->getIndex(), ref.front);
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

			MapSector* sector = invalid_refs_[index].sector;
			if (sector)
				return S_FMT("Set to Sector #%d", sector->getIndex());
			else
				return S_FMT("Clear Sector");
		}

		return "";
	}

private:
	struct SectorRef
	{
		MapLine*   line;
		bool       front;
		MapSector* sector;
		SectorRef(MapLine* line, bool front, MapSector* sector)
		{
			this->line   = line;
			this->front  = front;
			this->sector = sector;
		}
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
			if (!map_->getLine(a)->s1())
				lines_.push_back(a);
		}
	}

	unsigned nProblems() override { return lines_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= lines_.size())
			return "No invalid lines found";

		if (map_->getLine(lines_[index])->s2())
			return S_FMT("Line %d has no front side", lines_[index]);
		else
			return S_FMT("Line %d has no sides", lines_[index]);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= lines_.size())
			return false;

		MapLine* line = map_->getLine(lines_[index]);
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
				fpoint2_t pos = line->dirTabPoint(0.1);
				editor->edit2D().createSector(pos.x, pos.y);
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
				fpoint2_t pos = line->dirTabPoint(0.1);
				editor->edit2D().createSector(pos.x, pos.y);
				doCheck();
				return true;
			}
		}

		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index < lines_.size())
			return map_->getLine(lines_[index]);

		return nullptr;
	}

	string progressText() override { return "Checking for invalid lines..."; }

	string fixText(unsigned fix_type, unsigned index) override
	{
		if (map_->getLine(lines_[index])->s2())
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
			int special = map_->getSector(a)->getSpecial();
			int base    = Game::configuration().baseSectorType(special);
			if (S_CMP(Game::configuration().sectorTypeName(special), "Unknown"))
				sectors_.push_back(a);
		}
	}

	unsigned nProblems() override { return sectors_.size(); }

	string problemDesc(unsigned index) override
	{
		if (index >= sectors_.size())
			return "No unknown sector types found";

		return S_FMT("Sector %d has no sides", sectors_[index]);
	}

	bool fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) override
	{
		if (index >= sectors_.size())
			return false;

		MapSector* sec = map_->getSector(sectors_[index]);
		if (fix_type == 0)
		{
			// Try to preserve flags if they exist
			int special = sec->getSpecial();
			int base    = Game::configuration().baseSectorType(special);
			special &= ~base;
			sec->setIntProperty("special", special);
		}
		return false;
	}

	MapObject* getObject(unsigned index) override
	{
		if (index < sectors_.size())
			return map_->getSector(sectors_[index]);

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
			int special = map_->getLine(a)->getSpecial();
			if (S_CMP(Game::configuration().actionSpecialName(special), "Unknown"))
				objects_.push_back(map_->getLine(a));
		}
		// In Hexen or UDMF, go through map things too since they too can have specials
		if (map_->currentFormat() == MAP_HEXEN || map_->currentFormat() == MAP_UDMF)
		{
			for (unsigned a = 0; a < map_->nThings(); ++a)
			{
				// Ignore the Heresiarch which does not have a real special
				auto& tt = Game::configuration().thingType(map_->getThing(a)->getType());
				if (tt.flags() & Game::ThingType::FLAG_SCRIPT)
					continue;

				// Otherwise, check special
				int special = map_->getThing(a)->intProperty("special");
				if (S_CMP(Game::configuration().actionSpecialName(special), "Unknown"))
					objects_.push_back(map_->getThing(a));
			}
		}
	}

	unsigned nProblems() override { return objects_.size(); }

	string problemDesc(unsigned index) override
	{
		bool special = (map_->currentFormat() == MAP_HEXEN || map_->currentFormat() == MAP_UDMF);
		if (index >= objects_.size())
			return S_FMT("No unknown %s found", special ? "special" : "line type");

		return S_FMT(
			"%s %d has an unknown %s",
			objects_[index]->getObjType() == MapObject::Type::Line ? "Line" : "Thing",
			objects_[index]->getIndex(),
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
			MapThing* thing = map_->getThing(a);
			auto&     tt    = Game::configuration().thingType(thing->getType());
			if (tt.flags() & Game::ThingType::FLAG_OBSOLETE)
				things_.push_back(thing);
		}
	}

	unsigned nProblems() override { return things_.size(); }

	string problemDesc(unsigned index) override
	{
		bool special = (map_->currentFormat() == MAP_HEXEN || map_->currentFormat() == MAP_UDMF);
		if (index >= things_.size())
			return S_FMT("No obsolete things found");

		return S_FMT("Thing %d is obsolete", things_[index]->getIndex());
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
MapCheck* MapCheck::standardCheck(StandardCheck type, SLADEMap* map, MapTextureManager* texman)
{
	switch (type)
	{
	case MissingTexture: return new MissingTextureCheck(map);
	case SpecialTag: return new SpecialTagsCheck(map);
	case IntersectingLine: return new LinesIntersectCheck(map);
	case OverlappingLine: return new LinesOverlapCheck(map);
	case OverlappingThing: return new ThingsOverlapCheck(map);
	case UnknownTexture: return new UnknownTexturesCheck(map, texman);
	case UnknownFlat: return new UnknownFlatsCheck(map, texman);
	case UnknownThingType: return new UnknownThingTypesCheck(map);
	case StuckThing: return new StuckThingsCheck(map);
	case SectorReference: return new SectorReferenceCheck(map);
	case InvalidLine: return new InvalidLineCheck(map);
	case MissingTagged: return new MissingTaggedCheck(map);
	case UnknownSector: return new UnknownSectorCheck(map);
	case UnknownSpecial: return new UnknownSpecialCheck(map);
	case ObsoleteThing: return new ObsoleteThingCheck(map);
	default: return new MissingTextureCheck(map);
	}
}

// -----------------------------------------------------------------------------
// Same as above, but taking a string MapCheck type id instead of an enum value
// -----------------------------------------------------------------------------
MapCheck* MapCheck::standardCheck(const string& type_id, SLADEMap* map, MapTextureManager* texman)
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

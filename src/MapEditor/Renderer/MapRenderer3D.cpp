
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer3D.cpp
// Description: MapRenderer3D class - handles all rendering related stuff for
//              3d mode
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
#include "MapRenderer3D.h"
#include "App.h"
#include "Game/Configuration.h"
#include "General/ColourConfiguration.h"
#include "General/ResourceManager.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/OpenGL.h"
#include "SLADEMap/SLADEMap.h"
#include "UI/Controls/PaletteChooser.h"
#include "Utility/MathStuff.h"
#include "Utility/StringUtils.h"

using namespace slade;
using ExtraFloor = MapSector::ExtraFloor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Float, render_max_dist, 2000, CVar::Flag::Save)
CVAR(Float, render_max_thing_dist, 2000, CVar::Flag::Save)
CVAR(Int, render_thing_icon_size, 16, CVar::Flag::Save)
CVAR(Bool, render_fog_quality, true, CVar::Flag::Save)
CVAR(Bool, render_max_dist_adaptive, false, CVar::Flag::Save)
CVAR(Int, render_adaptive_ms, 15, CVar::Flag::Save)
CVAR(Bool, render_3d_sky, true, CVar::Flag::Save)
CVAR(Int, render_3d_things, 1, CVar::Flag::Save)
CVAR(Int, render_3d_things_style, 1, CVar::Flag::Save)
CVAR(Int, render_3d_hilight, 1, CVar::Flag::Save)
CVAR(Float, render_3d_brightness, 1, CVar::Flag::Save)
CVAR(Float, render_fog_distance, 1500, CVar::Flag::Save)
CVAR(Bool, render_fog_new_formula, true, CVar::Flag::Save)
CVAR(Bool, render_shade_orthogonal_lines, true, CVar::Flag::Save)
CVAR(Bool, mlook_invert_y, false, CVar::Flag::Save)
CVAR(Float, camera_3d_sensitivity_x, 1.0f, CVar::Flag::Save)
CVAR(Float, camera_3d_sensitivity_y, 1.0f, CVar::Flag::Save)
CVAR(Int, render_fov, 90, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, flats_use_vbo)
EXTERN_CVAR(Bool, use_zeth_icons)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// (Helper for updateLine) Fetches the per-wall-section offset and scale and
// adjusts the existing offsets to match.
// -----------------------------------------------------------------------------
void applyZDoomPerSectionOffsets(
	MapSide*      side,
	const string& section_name,
	double*       xoff,
	double*       yoff,
	double*       sx,
	double*       sy)
{
	if (side->hasProp("offsetx_" + section_name))
		*xoff += side->floatProperty("offsetx_" + section_name);
	if (side->hasProp("offsety_" + section_name))
		*yoff += side->floatProperty("offsety_" + section_name);

	if (side->hasProp("scalex_" + section_name))
		*sx = 1.0 / side->floatProperty("scalex_" + section_name);
	if (side->hasProp("scaley_" + section_name))
		*sy = 1.0 / side->floatProperty("scaley_" + section_name);

	*xoff *= *sx;
	*yoff *= *sy;
}
} // namespace


// -----------------------------------------------------------------------------
//
// MapRenderer3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapRenderer3D class constructor
// -----------------------------------------------------------------------------
MapRenderer3D::MapRenderer3D(SLADEMap* map) : map_{ map }
{
	// Build skybox circle
	buildSkyCircle();

	// Init other
	init();

	// Refresh textures when resources are updated or the main palette is changed
	sc_resources_updated_ = app::resources().signals().resources_updated.connect([this]() { refreshTextures(); });
	sc_palette_changed_   = theMainWindow->paletteChooser()->signals().palette_changed.connect([this]()
                                                                                             { refreshTextures(); });
}

// -----------------------------------------------------------------------------
// MapRenderer3D class destructor
// -----------------------------------------------------------------------------
MapRenderer3D::~MapRenderer3D()
{
	delete[] quads_;
	delete[] flats_;

	if (vbo_flats_ > 0)
		glDeleteBuffers(1, &vbo_flats_);
	if (vbo_walls_ > 0)
		glDeleteBuffers(1, &vbo_walls_);
}

// -----------------------------------------------------------------------------
// Initialises the 3d renderer
// -----------------------------------------------------------------------------
bool MapRenderer3D::init()
{
	// Init camera
	auto bbox = map_->bounds();
	cam_position_.set(bbox.min.x + (bbox.max.x - bbox.min.x) * 0.5, bbox.min.y + (bbox.max.y - bbox.min.y) * 0.5, 64);
	cam_direction_.set(0, 1);
	cam_pitch_ = 0;
	cameraUpdateVectors();

	refresh();

	return true;
}

// -----------------------------------------------------------------------------
// Clears VBOs and cached data
// -----------------------------------------------------------------------------
void MapRenderer3D::refresh()
{
	// Clear any existing map data
	dist_sectors_.clear();
	if (quads_)
	{
		delete[] quads_;
		quads_ = nullptr;
	}
	if (flats_)
	{
		delete[] flats_;
		flats_ = nullptr;
	}

	// Clear VBOs
	if (vbo_flats_ != 0)
	{
		glDeleteBuffers(1, &vbo_flats_);
		vbo_flats_ = 0;
	}

	sector_flats_.clear();

	// Set sky texture
	auto minf     = game::configuration().mapInfo(map_->mapName());
	skytex1_      = minf.sky1;
	skytex2_      = minf.sky2;
	skycol_top_.a = 0;
}

// -----------------------------------------------------------------------------
// Clears texture related data
// -----------------------------------------------------------------------------
void MapRenderer3D::refreshTextures()
{
	// Refresh lines
	for (auto& line : lines_)
	{
		for (auto& quad : line.quads)
			quad.texture = 0;

		line.updated_time = 0;
	}

	// Refresh flats
	for (auto& sector : sector_flats_)
	{
		for (auto& flat : sector)
		{
			flat.texture      = 0;
			flat.updated_time = 0;
		}
	}

	// Refresh things
	for (auto& thing : things_)
	{
		thing.sprite       = 0;
		thing.updated_time = 0;
	}
}

// -----------------------------------------------------------------------------
// Clears all cached rendering data
// -----------------------------------------------------------------------------
void MapRenderer3D::clearData()
{
	// Clear map structures
	lines_.clear();
	things_.clear();
	sector_flats_.clear();

	// Clear everything else
	refresh();
}

// -----------------------------------------------------------------------------
// Generates the array of points (circular) used for the sky
// -----------------------------------------------------------------------------
void MapRenderer3D::buildSkyCircle()
{
	double rot = 0;
	for (auto& pos : sky_circle_)
	{
		pos.set(sin(rot), -cos(rot));
		rot -= (3.1415926535897932384626433832795 * 2) / 32.0;
	}
}

// -----------------------------------------------------------------------------
// Returns the wall quad for wall selection [item]
// -----------------------------------------------------------------------------
MapRenderer3D::Quad* MapRenderer3D::getQuad(mapeditor::Item item)
{
	// Check item type
	if (item.type != mapeditor::ItemType::WallBottom && item.type != mapeditor::ItemType::WallMiddle
		&& item.type != mapeditor::ItemType::WallTop)
		return nullptr;

	// Get side
	auto side = map_->side(item.real_index >= 0 ? item.real_index : item.index);
	if (!side)
		return nullptr;

	// Find matching quad
	int line = side->parentLine()->index();
	for (unsigned a = 0; a < lines_[line].quads.size(); a++)
	{
		Quad* quad = &lines_[line].quads[a];

		// Check side
		if (side == side->parentLine()->s1() && quad->flags & BACK)
			continue;
		if (side == side->parentLine()->s2() && (quad->flags & BACK) == 0)
			continue;

		// Check 3D floor
		if (item.real_index >= 0)
		{
			for (unsigned m = a; m < lines_[line].quads.size(); m++)
			{
				if (lines_[line].quads[m].control_line == item.control_line)
					return &lines_[line].quads[m];
			}
		}

		// Check part
		if (item.type == mapeditor::ItemType::WallBottom)
		{
			if (quad->flags & LOWER)
				return quad;
		}
		if (item.type == mapeditor::ItemType::WallTop)
		{
			if (quad->flags & UPPER)
				return quad;
		}
		if (item.type == mapeditor::ItemType::WallMiddle)
		{
			if ((quad->flags & UPPER) == 0 && (quad->flags & LOWER) == 0)
				return quad;
		}
	}

	// Not found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the flat for sector flat selection [item]
// -----------------------------------------------------------------------------
MapRenderer3D::Flat* MapRenderer3D::getFlat(mapeditor::Item item)
{
	// Check index
	if (static_cast<unsigned>(item.index) >= sector_flats_.size())
		return nullptr;

	int index = (item.real_index == -1) ? item.index : item.real_index;

	// Floor
	if (item.type == mapeditor::ItemType::Floor && index < sector_flats_.size())
		return &sector_flats_[index][0];

	// Ceiling
	else if (item.type == mapeditor::ItemType::Ceiling && index < sector_flats_.size())
		return &sector_flats_[index][1];

	// Wrong type
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Moves the camera the direction it is facing by [distance].
// If [z] is false it will only be moved along x/y axes
// -----------------------------------------------------------------------------
void MapRenderer3D::cameraMove(double distance, bool z)
{
	// Move along direction vector
	if (z)
	{
		cam_position_.x += cam_dir3d_.x * distance;
		cam_position_.y += cam_dir3d_.y * distance;
		cam_position_.z += cam_dir3d_.z * distance;
	}
	else
	{
		cam_position_.x += cam_direction_.x * distance;
		cam_position_.y += cam_direction_.y * distance;
	}
}

// -----------------------------------------------------------------------------
// Rotates the camera by [angle] around the z axis
// -----------------------------------------------------------------------------
void MapRenderer3D::cameraTurn(double angle)
{
	// Find rotated view point
	Vec2d cp2d(cam_position_.x, cam_position_.y);
	Vec2d nd = math::rotatePoint(cp2d, cp2d + cam_direction_, angle);

	// Update direction
	cam_direction_.x = nd.x - cam_position_.x;
	cam_direction_.y = nd.y - cam_position_.y;

	// Update vectors
	cameraUpdateVectors();
}

// -----------------------------------------------------------------------------
// Moves the camera along the z axis by [distance]
// -----------------------------------------------------------------------------
void MapRenderer3D::cameraMoveUp(double distance)
{
	cam_position_.z += distance;
}

// -----------------------------------------------------------------------------
// Moves the camera along the strafe axis by [distance]
// -----------------------------------------------------------------------------
void MapRenderer3D::cameraStrafe(double distance)
{
	// Move along strafe vector
	cam_position_.x += cam_strafe_.x * distance;
	cam_position_.y += cam_strafe_.y * distance;
}

// -----------------------------------------------------------------------------
// Rotates the camera view around the strafe axis by [amount]
// -----------------------------------------------------------------------------
void MapRenderer3D::cameraPitch(double amount)
{
	// Pitch camera
	cam_pitch_ += amount;

	// Clamp
	double rad90 = math::PI * 0.5;
	if (cam_pitch_ > rad90)
		cam_pitch_ = rad90;
	if (cam_pitch_ < -rad90)
		cam_pitch_ = -rad90;

	// Update vectors
	cameraUpdateVectors();
}

// -----------------------------------------------------------------------------
// Updates the strafe and direction vectors for the camera
// -----------------------------------------------------------------------------
void MapRenderer3D::cameraUpdateVectors()
{
	// Normalize direction
	cam_direction_.normalize();

	// Calculate strafe vector
	cam_strafe_ = Vec3d(cam_direction_, 0).cross(Vec3d(0, 0, 1));
	cam_strafe_.normalize();

	// Calculate 3d direction vector
	cam_dir3d_ = math::rotateVector3D(Vec3d(cam_direction_, 0), cam_strafe_, cam_pitch_);
	cam_dir3d_.normalize();
}

// -----------------------------------------------------------------------------
// Sets the camera position to [position], facing [direction]
// -----------------------------------------------------------------------------
void MapRenderer3D::cameraSet(Vec3d position, Vec2d direction)
{
	// Set camera position/direction
	cam_position_  = position;
	cam_direction_ = direction;
	cam_pitch_     = 0;

	// Update camera vectors
	cameraUpdateVectors();
}

// -----------------------------------------------------------------------------
// Moves the camera to [position]
// -----------------------------------------------------------------------------
void MapRenderer3D::cameraSetPosition(Vec3d position)
{
	cam_position_ = position;
}

// -----------------------------------------------------------------------------
// Applies gravity to the camera
// -----------------------------------------------------------------------------
void MapRenderer3D::cameraApplyGravity(double mult)
{
	// Get current sector
	auto sector = map_->sectors().atPos(cam_position_.get2d());
	if (!sector)
		return;

	// Get target height from nearest floor down, including 3D floors
	auto  view_height = game::configuration().playerEyeHeight();
	Vec2d cam2d       = cam_position_.get2d();
	int   fheight     = (int)sector->floor().plane.heightAt(cam2d) + view_height;
	for (const auto& extra : sector->extraFloors())
	{
		// Only check solid floors
		if (extra.floor_type != ExtraFloor::SOLID)
			continue;

		// Allow stepping up from one 3D floor to another by the default Doom step height of 24
		auto* control_sector = map_->sector(extra.control_sector_index);
		int   this_height    = control_sector->ceiling().plane.heightAt(cam2d) + view_height;
		if (this_height <= cam_position_.z + 24 && this_height > fheight)
			fheight = this_height;
	}
	int cheight = sector->ceiling().plane.heightAt(cam2d);
	if (fheight > cheight - 4)
		fheight = cheight - 4;

	if (cam_position_.z > fheight)
	{
		double diff = cam_position_.z - fheight;
		cam_position_.z -= (diff * 0.3 * mult);
		if (cam_position_.z < fheight)
			cam_position_.z = fheight;
	}

	else if (cam_position_.z < fheight)
	{
		double diff = fheight - cam_position_.z;
		cam_position_.z += (diff * 0.5 * mult);
		if (cam_position_.z > fheight)
			cam_position_.z = fheight;
	}
}

// -----------------------------------------------------------------------------
// Moves the camera direction/pitch based on [xrel],[yrel]
// -----------------------------------------------------------------------------
void MapRenderer3D::cameraLook(double xrel, double yrel)
{
	cameraTurn(-xrel * 0.1 * camera_3d_sensitivity_x);
	if (mlook_invert_y)
		cameraPitch(yrel * 0.003 * camera_3d_sensitivity_y);
	else
		cameraPitch(-yrel * 0.003 * camera_3d_sensitivity_y);
}

// -----------------------------------------------------------------------------
// Sets up the OpenGL view/projection for rendering
// -----------------------------------------------------------------------------
void MapRenderer3D::setupView(int width, int height) const
{
	// Calculate aspect ratio
	float aspect = (1.6f / 1.333333f) * ((float)width / (float)height);
	float fovy   = 2 * math::radToDeg(atan(tan(math::degToRad(render_fov) / 2) / aspect));

	// Setup projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float max = render_max_dist * 1.5f;
	if (max < 100)
		max = 20000;
	gluPerspective(fovy, aspect, 0.5f, max);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Calculate up vector
	auto up = cam_strafe_.cross(cam_dir3d_).normalized();

	// Setup camera view
	gluLookAt(
		cam_position_.x,
		cam_position_.y,
		cam_position_.z,
		cam_position_.x + cam_dir3d_.x,
		cam_position_.y + cam_dir3d_.y,
		cam_position_.z + cam_dir3d_.z,
		up.x,
		up.y,
		up.z);
}

// -----------------------------------------------------------------------------
// Sets the OpenGL colour for rendering an object using [colour] and [light]
// level
// -----------------------------------------------------------------------------
void MapRenderer3D::setLight(const ColRGBA& colour, uint8_t light, float alpha) const
{
	// Force 255 light in fullbright mode
	if (fullbright_)
		light = 255;

	// Apply brightness
	else
		light = math::clamp(light * render_3d_brightness, 0, 255);

	// If we have a non-coloured light, darken it a bit to
	// closer resemble the software renderer light level
	float mult = (float)light / 255.0f;
	mult *= (mult * 1.3f);
	glColor4f(colour.fr() * mult, colour.fg() * mult, colour.fb() * mult, colour.fa() * alpha);
}

// -----------------------------------------------------------------------------
// Sets the OpenGL fog for rendering an object using [fogcol]
// -----------------------------------------------------------------------------
void MapRenderer3D::setFog(const ColRGBA& fogcol, uint8_t light)
{
	if (!fog_)
		return;

	// Setup fog colour
	GLfloat fogColor[3] = { fogcol.fr(), fogcol.fg(), fogcol.fb() };
	if (fog_colour_last_.r != fogcol.r || fog_colour_last_.g != fogcol.g || fog_colour_last_.b != fogcol.b)
	{
		glFogfv(GL_FOG_COLOR, fogColor);
		fog_colour_last_ = fogcol;
	}


	// Setup fog depth
	float depth;

	// check if fog color is default
	if (!render_fog_new_formula || (fogColor[0] == 0 && fogColor[1] == 0 && fogColor[2] == 0))
	{
		float lm = light / 170.0f;
		depth    = (lm * lm * 3000.0f);
	}
	else
		depth = render_fog_distance;

	if (fog_depth_last_ != depth)
	{
		glFogf(GL_FOG_END, depth);
		fog_depth_last_ = depth;
	}
}

// -----------------------------------------------------------------------------
// Renders the map in 3d
// -----------------------------------------------------------------------------
void MapRenderer3D::renderMap()
{
	// Setup GL stuff
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(GL_TRUE);
	glAlphaFunc(GL_GREATER, 0.0f);

	// Create flats array if needed
	if (sector_flats_.size() != map_->nSectors())
		sector_flats_.resize(map_->nSectors());

	// Create lines array if empty
	if (lines_.size() != map_->nLines())
		lines_.resize(map_->nLines());

	// Create things array if empty
	if (things_.size() != map_->nThings())
		things_.resize(map_->nThings());

	// Init VBO stuff
	if (gl::vboSupport())
	{
		// Check if any polygon vertex data has changed (in this case we need to refresh the entire vbo)
		bool vbo_updated = false;
		for (unsigned a = 0; a < map_->nSectors(); a++)
		{
			auto poly = map_->sector(a)->polygon();
			if (poly && poly->vboUpdate() > 1)
			{
				updateFlatsVBO();
				vbo_updated = true;
				break;
			}
		}

		// Create VBO if necessary
		if (!vbo_updated && vbo_flats_ == 0)
			updateFlatsVBO();

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	// Quick distance vis check
	sf::Clock clock;
	quickVisDiscard();

	// Build lists of quads and flats to render
	checkVisibleFlats();
	checkVisibleQuads();

	// Render sky
	if (render_3d_sky)
		renderSky();
	gl::setColour(ColRGBA::WHITE);

	if (fog_)
	{
		glEnable(GL_FOG);

		glFogi(GL_FOG_MODE, GL_LINEAR);
		glFogf(GL_FOG_DENSITY, 1.0f);
		glFogf(GL_FOG_START, 0.0f);

		if (render_fog_quality)
			glHint(GL_FOG_HINT, GL_NICEST);
		else
			glHint(GL_FOG_HINT, GL_FASTEST);
	}

	// Render walls
	renderWalls();

	// Render flats
	renderFlats();

	// Render things
	if (render_3d_things > 0)
		renderThings();

	// Render transparent stuff
	renderTransparentWalls();

	// Check elapsed time
	if (render_max_dist_adaptive)
	{
		long ms = clock.getElapsedTime().asMilliseconds();
		if (ms > render_adaptive_ms)
		{
			render_max_dist = render_max_dist - 100;
			if (render_max_dist < 1000)
				render_max_dist = 1000;
		}
		else if (ms < render_adaptive_ms - 5)
		{
			render_max_dist = render_max_dist + 100;
			if (render_max_dist > 20000)
				render_max_dist = 20000;
		}
	}

	// Cleanup gl state
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_FOG);
}

// -----------------------------------------------------------------------------
// Renders a cylindrical 'slice' of the sky between [top] and [bottom] on the z
// axis
// -----------------------------------------------------------------------------
void MapRenderer3D::renderSkySlice(float top, float bottom, float atop, float abottom, float size, float tx, float ty)
	const
{
	float tc_x  = 0.0f;
	float tc_y1 = (-top + 1.0f) * (ty * 0.5f);
	float tc_y2 = (-bottom + 1.0f) * (ty * 0.5f);

	glBegin(GL_QUADS);

	// Go through circular points
	for (unsigned a = 0; a < 31; a++)
	{
		// Top
		glColor4f(1.0f, 1.0f, 1.0f, atop);
		glTexCoord2f(tc_x + tx, tc_y1);
		glVertex3f(
			cam_position_.x + (sky_circle_[a + 1].x * size),
			cam_position_.y - (sky_circle_[a + 1].y * size),
			cam_position_.z + (top * size));
		glTexCoord2f(tc_x, tc_y1);
		glVertex3f(
			cam_position_.x + (sky_circle_[a].x * size),
			cam_position_.y - (sky_circle_[a].y * size),
			cam_position_.z + (top * size));

		// Bottom
		glColor4f(1.0f, 1.0f, 1.0f, abottom);
		glTexCoord2f(tc_x, tc_y2);
		glVertex3f(
			cam_position_.x + (sky_circle_[a].x * size),
			cam_position_.y - (sky_circle_[a].y * size),
			cam_position_.z + (bottom * size));
		glTexCoord2f(tc_x + tx, tc_y2);
		glVertex3f(
			cam_position_.x + (sky_circle_[a + 1].x * size),
			cam_position_.y - (sky_circle_[a + 1].y * size),
			cam_position_.z + (bottom * size));

		tc_x += tx;
	}

	// Link last point -> first
	// Top
	glColor4f(1.0f, 1.0f, 1.0f, atop);
	glTexCoord2f(tc_x + tx, tc_y1);
	glVertex3f(
		cam_position_.x + (sky_circle_[0].x * size),
		cam_position_.y - (sky_circle_[0].y * size),
		cam_position_.z + (top * size));
	glTexCoord2f(tc_x, tc_y1);
	glVertex3f(
		cam_position_.x + (sky_circle_[31].x * size),
		cam_position_.y - (sky_circle_[31].y * size),
		cam_position_.z + (top * size));

	// Bottom
	glColor4f(1.0f, 1.0f, 1.0f, abottom);
	glTexCoord2f(tc_x, tc_y2);
	glVertex3f(
		cam_position_.x + (sky_circle_[31].x * size),
		cam_position_.y - (sky_circle_[31].y * size),
		cam_position_.z + (bottom * size));
	glTexCoord2f(tc_x + tx, tc_y2);
	glVertex3f(
		cam_position_.x + (sky_circle_[0].x * size),
		cam_position_.y - (sky_circle_[0].y * size),
		cam_position_.z + (bottom * size));

	glEnd();
}

// -----------------------------------------------------------------------------
// Renders the sky
// -----------------------------------------------------------------------------
void MapRenderer3D::renderSky()
{
	gl::setColour(ColRGBA::WHITE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_TEXTURE_2D);

	// Center skybox a bit below the camera view
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -10.0f);

	// Get sky texture
	unsigned sky;
	if (!skytex2_.empty())
		sky = mapeditor::textureManager().texture(skytex2_, false).gl_id;
	else
		sky = mapeditor::textureManager().texture(skytex1_, false).gl_id;
	if (sky)
	{
		// Bind texture
		gl::Texture::bind(sky);

		// Get average colour if needed
		auto& tex_info = gl::Texture::info(sky);
		if (skycol_top_.a == 0)
		{
			int theight    = tex_info.size.y * 0.4;
			skycol_top_    = gl::Texture::averageColour(sky, { 0, 0, tex_info.size.x, theight });
			skycol_bottom_ = gl::Texture::averageColour(
				sky, { 0, tex_info.size.y - theight, tex_info.size.x, tex_info.size.y });
		}

		// Render top cap
		float size = 64.0f;
		glDisable(GL_TEXTURE_2D);
		gl::setColour(skycol_top_);
		glBegin(GL_QUADS);
		glVertex3f(cam_position_.x - (size * 10), cam_position_.y - (size * 10), cam_position_.z + size);
		glVertex3f(cam_position_.x - (size * 10), cam_position_.y + (size * 10), cam_position_.z + size);
		glVertex3f(cam_position_.x + (size * 10), cam_position_.y + (size * 10), cam_position_.z + size);
		glVertex3f(cam_position_.x + (size * 10), cam_position_.y - (size * 10), cam_position_.z + size);
		glEnd();

		// Render bottom cap
		gl::setColour(skycol_bottom_);
		glBegin(GL_QUADS);
		glVertex3f(cam_position_.x - (size * 10), cam_position_.y - (size * 10), cam_position_.z - size);
		glVertex3f(cam_position_.x - (size * 10), cam_position_.y + (size * 10), cam_position_.z - size);
		glVertex3f(cam_position_.x + (size * 10), cam_position_.y + (size * 10), cam_position_.z - size);
		glVertex3f(cam_position_.x + (size * 10), cam_position_.y - (size * 10), cam_position_.z - size);
		glEnd();

		// Render skybox sides
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_TEXTURE_2D);

		// Check for odd sky sizes
		float tx = 0.125f;
		float ty = 2.0f;
		if (tex_info.size.x > 256)
			tx = 0.125f / ((float)tex_info.size.x / 256.0f);
		if (tex_info.size.y > 128)
			ty = 1.0f;

		renderSkySlice(1.0f, 0.5f, 0.0f, 1.0f, size, tx, ty);   // Top
		renderSkySlice(0.5f, -0.5f, 1.0f, 1.0f, size, tx, ty);  // Middle
		renderSkySlice(-0.5f, -1.0f, 1.0f, 0.0f, size, tx, ty); // Bottom
	}

	glPopMatrix();
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);

	// Render all sky quads
	glDisable(GL_TEXTURE_2D);
	for (unsigned a = 0; a < n_quads_; a++)
	{
		// Ignore if not sky
		if ((quads_[a]->flags & SKY) == 0)
			continue;

		// Render quad
		renderQuad(quads_[a]);
		quads_[a] = quads_[n_quads_ - 1];
		n_quads_--;
		a--;
	}

	// Render all sky flats
	flat_last_ = 0;
	for (unsigned a = 0; a < n_flats_; a++)
	{
		if (!flats_[a])
			continue;

		// Ignore if not sky
		if ((flats_[a]->flags & SKY) == 0)
			continue;

		// Render quad
		renderFlat(flats_[a]);
		flats_[a] = flats_[n_flats_ - 1];
		n_flats_--;
		a--;
	}
	glEnable(GL_TEXTURE_2D);
}

// -----------------------------------------------------------------------------
// Updates the vertex texture coordinates of all polygons for sector [index]
// -----------------------------------------------------------------------------
void MapRenderer3D::updateFlatTexCoords(unsigned index, unsigned flat_index) const
{
	using game::UDMFFeature;

	// Check index
	if (index >= map_->nSectors())
		return;

	// Get sector
	auto sector         = map_->sector(index);
	auto control_sector = sector_flats_[index][flat_index].control_sector;

	// Get scaling/offset info
	double ox = 0.;
	double oy = 0.;
	// TODO 3dfloors
	double sx  = sector_flats_[index][flat_index].scale.x;
	double sy  = sector_flats_[index][flat_index].scale.y;
	double rot = 0.;

	// Check for UDMF + panning/scaling/rotation
	if (mapeditor::editContext().mapDesc().format == MapFormat::UDMF)
	{
		if (!(sector_flats_[index][flat_index].flags & CEIL))
		{
			if (game::configuration().featureSupported(UDMFFeature::FlatPanning))
			{
				ox = sector->floatProperty("xpanningfloor");
				oy = sector->floatProperty("ypanningfloor");
			}
			if (game::configuration().featureSupported(UDMFFeature::FlatScaling))
			{
				sx *= (1.0 / sector->floatProperty("xscalefloor"));
				sy *= (1.0 / sector->floatProperty("yscalefloor"));
			}
			if (game::configuration().featureSupported(UDMFFeature::FlatRotation))
				rot = sector->floatProperty("rotationfloor");
		}
		else
		{
			if (game::configuration().featureSupported(UDMFFeature::FlatPanning))
			{
				ox = sector->floatProperty("xpanningceiling");
				oy = sector->floatProperty("ypanningceiling");
			}
			if (game::configuration().featureSupported(UDMFFeature::FlatScaling))
			{
				sx *= (1.0 / sector->floatProperty("xscaleceiling"));
				sy *= (1.0 / sector->floatProperty("yscaleceiling"));
			}
			if (game::configuration().featureSupported(UDMFFeature::FlatRotation))
				rot = sector->floatProperty("rotationceiling");
		}
	}

	// Scaling applies to offsets as well.
	// Note for posterity: worldpanning only applies to textures, not flats
	ox /= sx;
	oy /= sy;

	// Update polygon texture coordinates
	sector->polygon()->setTexture(sector_flats_[index][flat_index].texture);
	sector->polygon()->updateTextureCoords(sx, sy, ox, oy, rot);
}

// -----------------------------------------------------------------------------
// Updates cached rendering data for sector [index]
// -----------------------------------------------------------------------------
void MapRenderer3D::updateSector(unsigned index)
{
	updateSectorFlats(index);
	updateSectorVBOs(index);
}

// -----------------------------------------------------------------------------
// Updates flat structures for sector [index]
// -----------------------------------------------------------------------------
void MapRenderer3D::updateSectorFlats(unsigned index)
{
	// Check index
	if (index >= map_->nSectors())
		return;

	auto* sector = map_->sector(index);
	sector_flats_[index].resize(2 * (1 + sector->extraFloors().size()));

	// Update floor
	bool  mix_tex_flats          = game::configuration().featureSupported(game::Feature::MixTexFlats);
	Flat& floor_flat             = sector_flats_[index][0];
	auto& ftex                   = mapeditor::textureManager().flat(sector->floor().texture, mix_tex_flats);
	floor_flat.sector            = sector;
	floor_flat.control_sector    = sector;
	floor_flat.extra_floor_index = -1;
	floor_flat.texture           = ftex.gl_id;
	floor_flat.scale             = ftex.scale;
	floor_flat.colour            = sector->colourAt(1, true);
	floor_flat.fogcolour         = sector->fogColour();
	floor_flat.light             = sector->lightAt(1);
	floor_flat.flags             = 0;
	floor_flat.plane             = sector->floor().plane;
	floor_flat.base_alpha        = 1.0f;
	if (strutil::equalCI(sector->floor().texture, game::configuration().skyFlat()))
		floor_flat.flags |= SKY;

	// Update ceiling
	Flat& ceiling_flat             = sector_flats_[index][1];
	auto& ctex                     = mapeditor::textureManager().flat(sector->ceiling().texture, mix_tex_flats);
	ceiling_flat.sector            = sector;
	ceiling_flat.control_sector    = sector;
	ceiling_flat.extra_floor_index = -1;
	ceiling_flat.texture           = ctex.gl_id;
	ceiling_flat.scale             = ctex.scale;
	ceiling_flat.colour            = sector->colourAt(2, true);
	ceiling_flat.fogcolour         = sector->fogColour();
	ceiling_flat.light             = sector->lightAt(2);
	ceiling_flat.flags             = CEIL;
	ceiling_flat.plane             = sector->ceiling().plane;
	ceiling_flat.base_alpha        = 1.0f;
	if (strutil::equalCI(sector->ceiling().texture, game::configuration().skyFlat()))
		ceiling_flat.flags |= SKY;

	// Deal with 3D floors
	for (unsigned a = 0; a < sector->extraFloors().size(); a++)
	{
		auto&      extra          = sector->extraFloors()[a];
		MapSector* control_sector = map_->sector(extra.control_sector_index);

		// TODO which of these is really a floor and which is really a ceiling?
		// TODO BOTH sides of the inner flat are drawn sometimes!
		// Bottom plane (defined by the floor of the control sector, but acts
		// like a ceiling)
		Flat& xf_floor             = sector_flats_[index][2 * (a + 1)];
		auto& xftex                = mapeditor::textureManager().flat(control_sector->floor().texture, mix_tex_flats);
		xf_floor.sector            = sector;
		xf_floor.control_sector    = control_sector;
		xf_floor.extra_floor_index = a;
		xf_floor.texture           = xftex.gl_id;
		xf_floor.scale             = xftex.scale;
		// TODO wrong.  maybe?  does it inherit from parent?
		xf_floor.colour = control_sector->colourAt(1, true);
		// TODO 3d floors have no fog color...  right?
		xf_floor.fogcolour = control_sector->fogColour();
		// TODO oughta support screen blends too!!
		// TODO this probably comes from the control sector, unless there's a flag, yadda...
		// TODO more importantly, it propagates downwards to the next floor
		// TODO are the light levels of the inside and outside different?  christ
		// TODO shouldn't this be 1?
		xf_floor.light = sector->lightAt(1, a);
		xf_floor.flags = FLATFLIP;
		if (extra.draw_inside)
			xf_floor.flags |= DRAWBOTH;
		xf_floor.plane      = extra.floor_plane;
		xf_floor.base_alpha = extra.alpha;

		if (extra.additiveTransparency())
		{
			xf_floor.flags |= TRANSADD;
		}

		// Top plane (defined by the ceiling of the control sector, but acts
		// like a floor)
		Flat& xf_ceiling          = sector_flats_[index][2 * (a + 1) + 1];
		auto& xctex               = mapeditor::textureManager().flat(control_sector->ceiling().texture, mix_tex_flats);
		xf_ceiling.sector         = sector;
		xf_ceiling.control_sector = control_sector;
		xf_ceiling.extra_floor_index = a;
		xf_ceiling.texture           = xctex.gl_id;
		xf_ceiling.scale             = xctex.scale;
		// TODO chump hack to use the real sector's light, which is wrong; fix the method to take this into account
		xf_ceiling.colour = sector->colourAt(2, true);
		// TODO again, maybe?
		xf_ceiling.fogcolour = control_sector->fogColour();
		// TODO this probably comes from the control sector, unless there's a flag, yadda...
		xf_ceiling.light = sector->lightAt(2, a);
		xf_ceiling.flags = CEIL | FLATFLIP;
		if (extra.draw_inside)
			xf_ceiling.flags |= DRAWBOTH;
		xf_ceiling.plane      = extra.ceiling_plane;
		xf_ceiling.base_alpha = extra.alpha;

		if (extra.additiveTransparency())
		{
			xf_ceiling.flags |= TRANSADD;
		}
	}

	// Finish up
	for (auto& flat : sector_flats_[index])
		flat.updated_time = app::runTimer();

	if (gl::vboSupport())
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		sector->polygon()->setZ(0);
	}
}

// -----------------------------------------------------------------------------
// Updates VBOs for sector [index]
// -----------------------------------------------------------------------------
void MapRenderer3D::updateSectorVBOs(unsigned index) const
{
	if (!gl::vboSupport())
		return;

	// Check index
	if (index >= map_->nSectors())
		return;
	MapSector* sector = map_->sector(index);
	Polygon2D* poly   = sector->polygon();

	// Update VBOs
	glBindBuffer(GL_ARRAY_BUFFER, vbo_flats_);
	Polygon2D::setupVBOPointers();

	for (unsigned a = 0; a < sector_flats_[index].size(); a++)
	{
		updateFlatTexCoords(index, a);
		poly->setZ(sector_flats_[index][a].plane);
		poly->writeToVBO(sector_flats_[index][a].vbo_offset);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	poly->setZ(0);
}

// -----------------------------------------------------------------------------
// Returns whether the sector at [index] needs to be updated.
// -----------------------------------------------------------------------------
bool MapRenderer3D::isSectorStale(unsigned index) const
{
	MapSector* sector = map_->sector(index);
	if (!sector)
		return false;

	return sector_flats_[index].empty() || sector_flats_[index][0].updated_time < sector->modifiedTime()
		   || sector_flats_[index][0].updated_time < sector->geometryUpdatedTime();
}

// -----------------------------------------------------------------------------
// Renders [flat]
// -----------------------------------------------------------------------------
void MapRenderer3D::renderFlat(const Flat* flat)
{
	// Skip if no sector (for whatever reason)
	if (!flat->sector)
		return;

	// Setup special rendering options
	float alpha = flat->alpha * flat->base_alpha;
	if (flat->flags & SKY && render_3d_sky)
	{
		alpha = 0;
		glDisable(GL_ALPHA_TEST);
	}
	if (flat->flags & TRANSADD)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	else
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Setup colour/light
	setLight(flat->colour, flat->light, alpha);

	// Setup fog colour
	setFog(flat->fogcolour, flat->light);

	// Render flat
	if (gl::vboSupport() && flats_use_vbo)
	{
		// Setup for floor or ceiling
		if (flat->flags & CEIL)
		{
			glCullFace((flat->flags & FLATFLIP) ? GL_FRONT : GL_BACK);
			flat_last_ = 2;
		}
		else
		{
			glCullFace((flat->flags & FLATFLIP) ? GL_BACK : GL_FRONT);
			flat_last_ = 1;
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo_flats_);
		Polygon2D::setupVBOPointers();

		if (flat->flags & DRAWBOTH)
			glDisable(GL_CULL_FACE);

		// Render
		flat->sector->polygon()->renderVBO(flat->vbo_offset);

		if (flat->flags & DRAWBOTH)
			glEnable(GL_CULL_FACE);
	}
	else
	{
		glPushMatrix();

		// Setup for floor or ceiling
		if (flat->flags & CEIL)
		{
			glCullFace((flat->flags & FLATFLIP) ? GL_FRONT : GL_BACK);
			glTranslated(0, 0, flat->sector->ceiling().height);
		}
		else
		{
			glCullFace((flat->flags & FLATFLIP) ? GL_BACK : GL_FRONT);
			glTranslated(0, 0, flat->sector->floor().height);
		}

		if (flat->flags & DRAWBOTH)
			glDisable(GL_CULL_FACE);

		// Render
		flat->sector->polygon()->render();

		if (flat->flags & DRAWBOTH)
			glEnable(GL_CULL_FACE);

		glPopMatrix();
	}

	// Reset settings
	if (flat->flags & SKY && render_3d_sky)
		glEnable(GL_ALPHA_TEST);
}

// -----------------------------------------------------------------------------
// Renders all currently visible flats
// -----------------------------------------------------------------------------
void MapRenderer3D::renderFlats()
{
	// Check for map
	if (!map_)
		return;

	// Init textures
	glEnable(GL_TEXTURE_2D);

	// Render all visible opaque flats, ordered by texture
	unsigned a        = 0;
	unsigned tex_last = 0;
	flat_last_        = 0;
	while (n_flats_ > 0)
	{
		a        = 0;
		tex_last = 0;
		while (a < n_flats_)
		{
			if (flats_[a])
			{
				// Skip different textures on this loop, and save all translucent
				// flats for last
				if ((tex_last && flats_[a]->texture != tex_last) || flats_[a]->base_alpha < 1.0f)
				{
					a++;
					continue;
				}

				// Check texture
				if (!tex_last)
					tex_last = flats_[a]->texture;
				if (tex_last)
					gl::Texture::bind(flats_[a]->texture);

				// Render flat
				renderFlat(flats_[a]);
			}

			flats_[a] = flats_[--n_flats_];
		}
		if (!tex_last)
			break;
	}

	// Render any remaining translucent flats, ordered by depth
	// TODO order by depth?  how??  sector distance, then plane height?  (note
	// that sloped and translucent is forbidden even in gzdoom)
	for (unsigned b = 0; b < n_flats_; b++)
	{
		if (!flats_[b])
			continue;

		if (tex_last != flats_[b]->texture)
		{
			tex_last = flats_[b]->texture;
			gl::Texture::bind(tex_last);
		}
		renderFlat(flats_[b]);
	}
	n_flats_ = 0;

	// Reset gl stuff
	glDisable(GL_TEXTURE_2D);
	if (gl::vboSupport())
	{
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

// -----------------------------------------------------------------------------
// Renders selection overlay for all selected flats
// -----------------------------------------------------------------------------
void MapRenderer3D::renderFlatSelection(const ItemSelection& selection, float alpha) const
{
	if (!render_selection_)
		return;

	// Setup gl stuff
	glLineWidth(2.0f);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_CULL_FACE);

	// Setup colour
	auto& def  = colourconfig::colDef("map_3d_selection");
	auto  col1 = def.colour;
	col1.a *= alpha;
	gl::setColour(col1, def.blendMode());
	auto col2 = col1;
	col2.a *= 0.5;

	// Go through selection
	for (auto& item : selection)
	{
		// TODO this code is awfully similar to the renderHilight code
		// Ignore if not a sector hilight
		if (item.type != mapeditor::ItemType::Ceiling && item.type != mapeditor::ItemType::Floor)
			continue;

		// Get sector
		auto sector = map_->sector(item.real_index >= 0 ? item.real_index : item.index);
		if (!sector)
			return;

		// Get plane
		Plane plane;
		if (item.real_index >= 0)
		{
			for (const auto& extra : sector->extraFloors())
			{
				if (extra.control_sector_index == item.index)
				{
					if (item.type == mapeditor::ItemType::Floor)
						plane = extra.floor_plane;
					else
						plane = extra.ceiling_plane;
				}
			}
		}
		else
		{
			if (item.type == mapeditor::ItemType::Floor)
				plane = sector->floor().plane;
			else
				plane = sector->ceiling().plane;
		}

		// Draw sector outline
		vector<MapLine*> lines;
		sector->putLines(lines);
		gl::setColour(col1);
		glBegin(GL_LINES);
		for (auto& line : lines)
		{
			glVertex3d(line->x1(), line->y1(), plane.heightAt(line->x1(), line->y1()));
			glVertex3d(line->x2(), line->y2(), plane.heightAt(line->x2(), line->y2()));
		}
		glEnd();

		// Render fill
		gl::setColour(col2);
		glDisable(GL_CULL_FACE);
		sector->polygon()->setZ(plane);
		sector->polygon()->render();
		sector->polygon()->setZ(0);
		glEnable(GL_CULL_FACE);
	}

	glCullFace(GL_BACK);
}

// -----------------------------------------------------------------------------
// Sets up coordinates for a quad
// -----------------------------------------------------------------------------
void MapRenderer3D::setupQuad(Quad* quad, Seg2d seg, double top, double bottom) const
{
	// Left
	quad->points[0].x = quad->points[1].x = seg.x1();
	quad->points[0].y = quad->points[1].y = seg.y1();

	// Right
	quad->points[2].x = quad->points[3].x = seg.x2();
	quad->points[2].y = quad->points[3].y = seg.y2();

	// Top/bottom
	quad->points[0].z = quad->points[3].z = top;
	quad->points[1].z = quad->points[2].z = bottom;
}

// -----------------------------------------------------------------------------
// Sets up coordinates for a quad
// -----------------------------------------------------------------------------
void MapRenderer3D::setupQuad(Quad* quad, Seg2d seg, Plane top, Plane bottom) const
{
	// Left
	quad->points[0].x = quad->points[1].x = seg.x1();
	quad->points[0].y = quad->points[1].y = seg.y1();

	// Right
	quad->points[2].x = quad->points[3].x = seg.x2();
	quad->points[2].y = quad->points[3].y = seg.y2();

	// Top/bottom
	quad->points[0].z = top.heightAt(quad->points[0].x, quad->points[0].y);
	quad->points[1].z = bottom.heightAt(quad->points[1].x, quad->points[1].y);
	quad->points[2].z = bottom.heightAt(quad->points[2].x, quad->points[2].y);
	quad->points[3].z = top.heightAt(quad->points[3].x, quad->points[3].y);
}

// -----------------------------------------------------------------------------
// Calculates texture coordinates for a quad
// -----------------------------------------------------------------------------
void MapRenderer3D::setupQuadTexCoords(
	MapRenderer3D::Quad* quad,
	int                  length,
	double               o_left,
	double               o_top,
	double               h_top,
	double               h_bottom,
	bool                 pegbottom,
	double               sx,
	double               sy) const
{
	// Check texture
	if (!quad->texture)
		return;

	// Determine integral height
	int height = math::round(h_top - h_bottom);

	// Initial offsets
	double y1       = o_top;
	double y2       = o_top + height;
	auto&  tex_info = gl::Texture::info(quad->texture);
	if (pegbottom)
	{
		y2 = o_top + tex_info.size.y * sy;
		y1 = y2 - height;
	}

	double x_mult = 1.0 / (tex_info.size.x * sx);
	double y_mult = 1.0 / (tex_info.size.y * sy);

	// Set texture coordinates
	quad->points[0].tx = o_left * x_mult;
	quad->points[0].ty = (y1 * y_mult) + ((h_top - quad->points[0].z) * y_mult);
	quad->points[1].tx = o_left * x_mult;
	quad->points[1].ty = (y2 * y_mult) + ((h_bottom - quad->points[1].z) * y_mult);
	quad->points[2].tx = (o_left + length) * x_mult;
	quad->points[2].ty = (y2 * y_mult) + ((h_bottom - quad->points[2].z) * y_mult);
	quad->points[3].tx = (o_left + length) * x_mult;
	quad->points[3].ty = (y1 * y_mult) + ((h_top - quad->points[3].z) * y_mult);
}

// -----------------------------------------------------------------------------
// Updates cached rendering data for line [index]
// -----------------------------------------------------------------------------
void MapRenderer3D::updateLine(unsigned index)
{
	using game::Feature;
	using game::UDMFFeature;

	// Check index
	if (index > lines_.size())
		return;

	// Clear current line data
	lines_[index].quads.clear();

	// Skip invalid line
	auto line = map_->line(index);
	if (!line->s1())
		return;

	// Process line special
	map_->mapSpecials()->processLineSpecial(line);
	bool line_translucent = map_->mapSpecials()->lineIsTranslucent(line);

	// Get relevant line info
	auto   map_format = mapeditor::editContext().mapDesc().format;
	bool   upeg       = game::configuration().lineBasicFlagSet("dontpegtop", line, map_format);
	bool   lpeg       = game::configuration().lineBasicFlagSet("dontpegbottom", line, map_format);
	double xoff, yoff, sx, sy, lsx, lsy;
	bool   mixed       = game::configuration().featureSupported(Feature::MixTexFlats);
	lines_[index].line = line;
	double alpha       = 1.0;
	if (line->hasProp("alpha"))
		alpha = line->floatProperty("alpha");
	else if (line_translucent) // TranslucentLine special
		alpha = map_->mapSpecials()->translucentLineAlpha(line);

	int line_shading = 0;
	if (render_shade_orthogonal_lines)
	{
		// Increase light level for N/S facing lines
		if (line->x1() == line->x2())
			line_shading = +16;
		// Decrease light level for E/W facing lines
		else if (line->y1() == line->y2())
			line_shading = -16;
	}

	// Get first side info
	int  floor1     = line->frontSector()->floor().height;
	int  ceiling1   = line->frontSector()->ceiling().height;
	auto fp1        = line->frontSector()->floor().plane;
	auto cp1        = line->frontSector()->ceiling().plane;
	auto colour1    = line->frontSector()->colourAt(0, true);
	auto fogcolour1 = line->frontSector()->fogColour();
	auto light1     = line->s1()->light();
	int  xoff1      = line->s1()->texOffsetX();
	int  yoff1      = line->s1()->texOffsetY();

	if (render_shade_orthogonal_lines)
	{
		colour1.r = math::clamp(colour1.r + line_shading, 0, 255);
		colour1.g = math::clamp(colour1.g + line_shading, 0, 255);
		colour1.b = math::clamp(colour1.b + line_shading, 0, 255);
		light1    = math::clamp(light1 + line_shading, 0, 255);
	}

	// --- One-sided line ---
	lsx        = 1;
	lsy        = 1;
	int length = math::round(line->length());
	if (line->s1() && !line->s2())
	{
		Quad quad;

		// Determine offsets
		xoff = xoff1;
		yoff = yoff1;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureOffsets))
		{
			if (line->s1()->hasProp("offsetx_mid"))
				xoff += line->s1()->floatProperty("offsetx_mid");
			if (line->s1()->hasProp("offsety_mid"))
				yoff += line->s1()->floatProperty("offsety_mid");
		}

		// Texture scale
		auto& tex    = mapeditor::textureManager().texture(line->s1()->texMiddle(), mixed);
		quad.texture = tex.gl_id;
		sx           = tex.scale.x;
		sy           = tex.scale.y;
		if (game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			if (line->s1()->hasProp("scalex_mid"))
				lsx = 1.0 / line->s1()->floatProperty("scalex_mid");
			if (line->s1()->hasProp("scaley_mid"))
				lsy = 1.0 / line->s1()->floatProperty("scaley_mid");
		}
		if (!tex.world_panning)
		{
			xoff *= sx;
			yoff *= sy;
		}
		sx *= lsx;
		sy *= lsy;
		xoff *= lsx;
		yoff *= lsy;

		// Create quad
		setupQuad(&quad, line->seg(), cp1, fp1);
		quad.colour    = colour1;
		quad.fogcolour = fogcolour1;
		quad.light     = light1;
		setupQuadTexCoords(&quad, length, xoff, yoff, ceiling1, floor1, lpeg, sx, sy);

		// Add middle quad and finish
		lines_[index].quads.push_back(quad);
		lines_[index].updated_time = app::runTimer();
		return;
	}

	// --- Two-sided line ---

	// TODO middle parts may show 3D floors!

	// Get second side info
	int    floor2      = line->backSector()->floor().height;
	int    ceiling2    = line->backSector()->ceiling().height;
	auto   fp2         = line->backSector()->floor().plane;
	auto   cp2         = line->backSector()->ceiling().plane;
	auto   colour2     = line->backSector()->colourAt(0, true);
	auto   fogcolour2  = line->backSector()->fogColour();
	auto   light2      = line->s2()->light();
	int    xoff2       = line->s2()->texOffsetX();
	int    yoff2       = line->s2()->texOffsetY();
	int    lowceil     = min(ceiling1, ceiling2);
	int    highfloor   = max(floor1, floor2);
	string sky_flat    = game::configuration().skyFlat();
	string hidden_tex  = map_->currentFormat() == MapFormat::Doom64 ? "?" : "-";
	bool   show_midtex = (map_->currentFormat() != MapFormat::Doom64) || (line->flagSet(512));
	// Heights at both endpoints, for both planes, on both sides
	double f1h1 = fp1.heightAt(line->x1(), line->y1());
	double f1h2 = fp1.heightAt(line->x2(), line->y2());
	double f2h1 = fp2.heightAt(line->x1(), line->y1());
	double f2h2 = fp2.heightAt(line->x2(), line->y2());
	double c1h1 = cp1.heightAt(line->x1(), line->y1());
	double c1h2 = cp1.heightAt(line->x2(), line->y2());
	double c2h1 = cp2.heightAt(line->x1(), line->y1());
	double c2h2 = cp2.heightAt(line->x2(), line->y2());

	if (render_shade_orthogonal_lines)
	{
		colour2.r = math::clamp(colour2.r + line_shading, 0, 255);
		colour2.g = math::clamp(colour2.g + line_shading, 0, 255);
		colour2.b = math::clamp(colour2.b + line_shading, 0, 255);
		light2    = math::clamp(light2 + line_shading, 0, 255);
	}

	// Front lower
	lsx = 1;
	lsy = 1;
	if (f2h1 > f1h1 || f2h2 > f1h2)
	{
		Quad quad;

		// Determine offsets
		xoff = xoff1;
		yoff = yoff1;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureOffsets))
		{
			// UDMF extra offsets
			if (line->s1()->hasProp("offsetx_bottom"))
				xoff += line->s1()->floatProperty("offsetx_bottom");
			if (line->s1()->hasProp("offsety_bottom"))
				yoff += line->s1()->floatProperty("offsety_bottom");
		}

		// Texture scale
		auto& tex    = mapeditor::textureManager().texture(line->s1()->texLower(), mixed);
		quad.texture = tex.gl_id;
		sx           = tex.scale.x;
		sy           = tex.scale.y;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			if (line->s1()->hasProp("scalex_bottom"))
				lsx = 1.0 / line->s1()->floatProperty("scalex_bottom");
			if (line->s1()->hasProp("scaley_bottom"))
				lsy = 1.0 / line->s1()->floatProperty("scaley_bottom");
		}
		if (!tex.world_panning)
		{
			xoff *= sx;
			yoff *= sy;
		}
		sx *= lsx;
		sy *= lsy;
		xoff *= lsx;
		yoff *= lsy;

		if (lpeg) // Lower unpegged
			yoff += (ceiling1 - floor2);

		// Create quad
		setupQuad(&quad, line->seg(), fp2, fp1);
		quad.colour    = colour1;
		quad.fogcolour = fogcolour1;
		quad.light     = light1;
		setupQuadTexCoords(&quad, length, xoff, yoff, floor2, floor1, false, sx, sy);
		quad.flags |= LOWER;

		// Add quad
		lines_[index].quads.push_back(quad);
	}

	// Front middle
	lsx          = 1;
	lsy          = 1;
	auto midtex1 = line->stringProperty("side1.texturemiddle");
	if (!midtex1.empty() && midtex1 != hidden_tex && show_midtex)
	{
		Quad quad;

		// Get texture
		auto& tex    = mapeditor::textureManager().texture(line->s1()->texMiddle(), mixed);
		quad.texture = tex.gl_id;

		// Determine offsets
		xoff        = xoff1;
		yoff        = yoff1;
		double ytex = 0;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureOffsets))
		{
			if (line->s1()->hasProp("offsetx_mid"))
				xoff += line->s1()->floatProperty("offsetx_mid");
			if (line->s1()->hasProp("offsety_mid"))
				yoff += line->s1()->floatProperty("offsety_mid");
		}

		// Texture scale
		sx = tex.scale.x;
		sy = tex.scale.y;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			if (line->s1()->hasProp("scalex_mid"))
				lsx = 1.0 / line->s1()->floatProperty("scalex_mid");
			if (line->s1()->hasProp("scaley_mid"))
				lsy = 1.0 / line->s1()->floatProperty("scaley_mid");
		}
		if (!tex.world_panning)
		{
			xoff *= sx;
			yoff *= sy;
		}
		sx *= lsx;
		sy *= lsy;
		xoff *= lsx;
		yoff *= lsy;

		// Setup quad coordinates
		double top, bottom;
		if ((map_->currentFormat() == MapFormat::Doom64)
			|| ((
				map_->currentFormat() == MapFormat::UDMF
				&& game::configuration().featureSupported(UDMFFeature::SideMidtexWrapping)
				&& line->boolProperty("wrapmidtex"))))
		{
			top    = lowceil;
			bottom = highfloor;
			ytex   = yoff;
			if (lpeg)
				ytex -= lowceil - highfloor;
		}
		else if (lpeg)
		{
			auto& tex_info = gl::Texture::info(quad.texture);
			bottom         = highfloor + yoff;
			top            = bottom + (tex_info.size.y * sy);
		}
		else
		{
			auto& tex_info = gl::Texture::info(quad.texture);
			top            = lowceil + yoff;
			bottom         = top - (tex_info.size.y * sy);
		}
		// The "correct" thing here is to allow textures to run into the floor
		// unless clipmidtex is on, but OpenGL is designed not to allow that to
		// happen, and GZDoom doesn't support it either.
		if (bottom < highfloor)
			bottom = highfloor;
		if (top > lowceil)
		{
			ytex = top - lowceil;
			top  = lowceil;
		}

		// Create quad
		setupQuad(&quad, line->seg(), top, bottom);
		quad.colour    = colour1.ampf(1.0f, 1.0f, 1.0f, alpha);
		quad.fogcolour = fogcolour1;
		quad.light     = light1;
		setupQuadTexCoords(&quad, length, xoff, ytex, top, bottom, false, sx, sy);
		quad.flags |= MIDTEX;
		if (line->hasProp("renderstyle") && line->stringProperty("renderstyle") == "add")
			quad.flags |= TRANSADD;
		else if (line_translucent && map_->mapSpecials()->translucentLineAdditive(line)) // TranslucentLine special
			quad.flags |= TRANSADD;

		// Add quad
		lines_[index].quads.push_back(quad);
	}

	// Front upper
	lsx = 1;
	lsy = 1;
	if (c1h1 > c2h1 || c1h2 > c2h2)
	{
		Quad quad;

		// Determine offsets
		xoff = xoff1;
		yoff = yoff1;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureOffsets))
		{
			// UDMF extra offsets
			if (line->s1()->hasProp("offsetx_top"))
				xoff += line->s1()->floatProperty("offsetx_top");
			if (line->s1()->hasProp("offsety_top"))
				yoff += line->s1()->floatProperty("offsety_top");
		}

		// Texture scale
		auto& tex    = mapeditor::textureManager().texture(line->s1()->texUpper(), mixed);
		quad.texture = tex.gl_id;
		sx           = tex.scale.x;
		sy           = tex.scale.y;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			if (line->s1()->hasProp("scalex_top"))
				lsx = 1.0 / line->s1()->floatProperty("scalex_top");
			if (line->s1()->hasProp("scaley_top"))
				lsy = 1.0 / line->s1()->floatProperty("scaley_top");
		}
		if (!tex.world_panning)
		{
			xoff *= sx;
			yoff *= sy;
		}
		sx *= lsx;
		sy *= lsy;
		xoff *= lsx;
		yoff *= lsy;

		// Create quad
		setupQuad(&quad, line->seg(), cp1, cp2);
		quad.colour    = colour1;
		quad.fogcolour = fogcolour1;
		quad.light     = light1;
		setupQuadTexCoords(&quad, length, xoff, yoff, ceiling1, ceiling2, !upeg, sx, sy);
		// Sky hack only applies if both sectors have a sky ceiling
		if (strutil::equalCI(sky_flat, line->frontSector()->ceiling().texture)
			&& strutil::equalCI(sky_flat, line->backSector()->ceiling().texture))
			quad.flags |= SKY;
		quad.flags |= UPPER;

		// Add quad
		lines_[index].quads.push_back(quad);
	}

	// Back lower
	lsx = 1;
	lsy = 1;
	if (f1h1 > f2h1 || f1h2 > f2h2)
	{
		Quad quad;

		// Determine offsets
		xoff = xoff2;
		yoff = yoff2;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureOffsets))
		{
			// UDMF extra offsets
			if (line->s2()->hasProp("offsetx_bottom"))
				xoff += line->s2()->floatProperty("offsetx_bottom");
			if (line->s2()->hasProp("offsety_bottom"))
				yoff += line->s2()->floatProperty("offsety_bottom");
		}

		// Texture scale
		auto& tex    = mapeditor::textureManager().texture(line->s2()->texLower(), mixed);
		quad.texture = tex.gl_id;
		sx           = tex.scale.x;
		sy           = tex.scale.y;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			if (line->s2()->hasProp("scalex_bottom"))
				lsx = 1.0 / line->s2()->floatProperty("scalex_bottom");
			if (line->s2()->hasProp("scaley_bottom"))
				lsy = 1.0 / line->s2()->floatProperty("scaley_bottom");
		}
		if (!tex.world_panning)
		{
			xoff *= sx;
			yoff *= sy;
		}
		sx *= lsx;
		sy *= lsy;
		xoff *= lsx;
		yoff *= lsy;

		if (lpeg) // Lower unpegged
			yoff += (ceiling2 - floor1);

		// Create quad
		setupQuad(&quad, line->seg().flip(), fp1, fp2);
		quad.colour    = colour2;
		quad.fogcolour = fogcolour2;
		quad.light     = light2;
		setupQuadTexCoords(&quad, length, xoff, yoff, floor1, floor2, false, sx, sy);
		if (strutil::equalCI(sky_flat, line->frontSector()->floor().texture))
			quad.flags |= SKY;
		quad.flags |= BACK;
		quad.flags |= LOWER;

		// Add quad
		lines_[index].quads.push_back(quad);
	}

	// Back middle
	lsx          = 1;
	lsy          = 1;
	auto midtex2 = line->stringProperty("side2.texturemiddle");
	if (!midtex2.empty() && midtex2 != hidden_tex && show_midtex)
	{
		Quad quad;

		// Get texture
		auto& tex    = mapeditor::textureManager().texture(midtex2, mixed);
		quad.texture = tex.gl_id;

		// Determine offsets
		xoff        = xoff2;
		yoff        = yoff2;
		double ytex = 0;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureOffsets))
		{
			if (line->s2()->hasProp("offsetx_mid"))
				xoff += line->s2()->floatProperty("offsetx_mid");
			if (line->s2()->hasProp("offsety_mid"))
				yoff += line->s2()->floatProperty("offsety_mid");
		}

		// Texture scale
		sx = tex.scale.x;
		sy = tex.scale.y;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			if (line->s2()->hasProp("scalex_mid"))
				lsx = 1.0 / line->s2()->floatProperty("scalex_mid");
			if (line->s2()->hasProp("scaley_mid"))
				lsy = 1.0 / line->s2()->floatProperty("scaley_mid");
		}
		if (!tex.world_panning)
		{
			xoff *= sx;
			yoff *= sy;
		}
		sx *= lsx;
		sy *= lsy;
		xoff *= lsx;
		yoff *= lsy;

		// Setup quad coordinates
		double top, bottom;
		if ((map_->currentFormat() == MapFormat::Doom64)
			|| (map_->currentFormat() == MapFormat::UDMF
				&& game::configuration().featureSupported(UDMFFeature::SideMidtexWrapping)
				&& line->boolProperty("wrapmidtex")))
		{
			top    = lowceil;
			bottom = highfloor;
			ytex   = yoff;
			if (lpeg)
				ytex -= lowceil - highfloor;
		}
		else if (lpeg)
		{
			auto& tex_info = gl::Texture::info(quad.texture);
			bottom         = highfloor + yoff;
			top            = bottom + (tex_info.size.y * sy);
		}
		else
		{
			auto& tex_info = gl::Texture::info(quad.texture);
			top            = lowceil + yoff;
			bottom         = top - (tex_info.size.y * sy);
		}
		// The "correct" thing here is to allow textures to run into the floor
		// unless clipmidtex is on, but OpenGL is designed not to allow that to
		// happen, and GZDoom doesn't support it either.
		if (bottom < highfloor)
			bottom = highfloor;
		if (top > lowceil)
		{
			ytex = top - lowceil;
			top  = lowceil;
		}

		// Create quad
		setupQuad(&quad, line->seg().flip(), top, bottom);
		quad.colour    = colour2.ampf(1.0f, 1.0f, 1.0f, alpha);
		quad.fogcolour = fogcolour2;
		quad.light     = light2;
		setupQuadTexCoords(&quad, length, xoff, ytex, top, bottom, false, sx, sy);
		quad.flags |= BACK;
		quad.flags |= MIDTEX;
		if (line->hasProp("renderstyle") && line->stringProperty("renderstyle") == "add")
			quad.flags |= TRANSADD;
		else if (line_translucent && map_->mapSpecials()->translucentLineAdditive(line)) // TranslucentLine special
			quad.flags |= TRANSADD;

		// Add quad
		lines_[index].quads.push_back(quad);
	}

	// Back upper
	lsx = 1;
	lsy = 1;
	if (c2h1 > c1h1 || c2h2 > c1h2)
	{
		Quad quad;

		// Determine offsets
		xoff = xoff2;
		yoff = yoff2;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureOffsets))
		{
			// UDMF extra offsets
			if (line->s2()->hasProp("offsetx_top"))
				xoff += line->s2()->floatProperty("offsetx_top");
			if (line->s2()->hasProp("offsety_top"))
				yoff += line->s2()->floatProperty("offsety_top");
		}

		// Texture scale
		auto& tex    = mapeditor::textureManager().texture(line->s2()->texUpper(), mixed);
		quad.texture = tex.gl_id;
		sx           = tex.scale.x;
		sy           = tex.scale.y;
		if (map_->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			if (line->s2()->hasProp("scalex_top"))
				lsx = 1.0 / line->s2()->floatProperty("scalex_top");
			if (line->s2()->hasProp("scaley_top"))
				lsy = 1.0 / line->s2()->floatProperty("scaley_top");
		}
		if (!tex.world_panning)
		{
			xoff *= sx;
			yoff *= sy;
		}
		sx *= lsx;
		sy *= lsy;
		xoff *= lsx;
		yoff *= lsy;

		// Create quad
		setupQuad(&quad, line->seg().flip(), cp2, cp1);
		quad.colour    = colour2;
		quad.fogcolour = fogcolour2;
		quad.light     = light2;
		setupQuadTexCoords(&quad, length, xoff, yoff, ceiling2, ceiling1, !upeg, sx, sy);
		if (strutil::equalCI(sky_flat, line->frontSector()->ceiling().texture))
			quad.flags |= SKY;
		quad.flags |= BACK;
		quad.flags |= UPPER;

		// Add quad
		lines_[index].quads.push_back(quad);
	}

	// Add any middle lines created by 3D floors
	// TODO this code is so, so duplicated, oh dear
	// TODO going to have to deal with vertical overlaps (which already do
	// weird things when sloped floors meet in the middle of a line)
	// TODO relatedly, need to be able to SPLIT a quad even when there's no
	// texture, because 3d floors can change the lighting
	// TODO will one-sided lines ever have to worry about this?
	// TODO need to do back sector too, but with some of the logic reversed
	MapSide* sides[2] = { line->s2(), line->s1() };
	for (unsigned front = 0; front < 2; front++)
	{
		MapSide*   side   = sides[front];
		MapSector* sector = side->sector();

		for (const auto& extra : sector->extraFloors())
		{
			if (extra.ceilingOnly())
				continue; // A floor that's a flat plane can't possibly have any sides

			// Don't draw a texture if the same 3D floor is on both sides
			// TODO a bit clumsy and inefficient
			// TODO unclear what happens if /different/ 3d floors are on both sides, or if the line itself also has a
			// midtex, or if there's an EF_LO affecting the outside of the 3d floor...
			bool shared = false;
			for (auto& extra2 : sides[1 - front]->sector()->extraFloors())
			{
				if (extra2.control_sector_index == extra.control_sector_index)
				{
					shared = true;
					break;
				}
			}
			if (shared)
				continue;

			MapSector* control_sector = map_->sector(extra.control_sector_index);
			MapLine*   control_line   = map_->line(extra.control_line_index);

			xoff = control_line->s1()->texOffsetX() + line->s1()->texOffsetX();
			yoff = control_line->s1()->texOffsetY() + line->s1()->texOffsetY();
			sx = sy = 1;
			if (map_->currentFormat() == MapFormat::UDMF)
				applyZDoomPerSectionOffsets(control_line->s1(), "mid", &xoff, &yoff, &sx, &sy);

			// TODO missing texture check should look for this!
			string texname;
			// Not documented, but in practice, when both flags are set, upper wins
			if (extra.useUpperTexture())
				texname = side->texUpper();
			else if (extra.useLowerTexture())
				texname = side->texLower();
			else
				texname = control_line->s1()->texMiddle();

			Quad quad;
			auto seg = line->seg();
			// If the 3D floor is on the front side of the line, then the
			// outside wall is on the back, so the line segment must be flipped
			if (front)
				seg = seg.flip();
			setupQuad(&quad, seg, control_sector->ceiling().plane, control_sector->floor().plane);
			// TODO all this nonsense
			quad.colour    = colour1.ampf(1.0f, 1.0f, 1.0f, extra.alpha);
			quad.fogcolour = fogcolour1;
			quad.light     = light1;
			quad.texture   = mapeditor::textureManager().texture(texname, mixed).gl_id;

			setupQuadTexCoords(
				&quad,
				length,
				xoff,
				yoff,
				control_sector->ceiling().height,
				control_sector->floor().height,
				false,
				sx,
				sy);
			quad.flags |= MIDTEX;
			if (extra.draw_inside)
			{
				quad.flags |= DRAWBOTH;
			}
			if (extra.additiveTransparency())
			{
				quad.flags |= TRANSADD;
			}
			quad.control_line = extra.control_line_index;
			quad.control_side = control_line->s1()->index();
			// TODO other flags?
			// TODO probably need to remember which extra-floor this came from
			// too, which is slightly more complicated since it might be the
			// sector on either side of the line
			// TODO TRANSADD if that one flag is set!

			lines_[index].quads.push_back(quad);
		}
	}

	// Finished
	lines_[index].updated_time = app::runTimer();
}

// -----------------------------------------------------------------------------
// Renders [quad]
// -----------------------------------------------------------------------------
void MapRenderer3D::renderQuad(const Quad* quad, float alpha)
{
	// Setup special rendering options
	if (quad->colour.a == 255)
	{
		if (quad->flags & SKY && render_3d_sky)
		{
			alpha = 0;
			glDisable(GL_ALPHA_TEST);
		}
		else if (quad->flags & MIDTEX)
			glAlphaFunc(GL_GREATER, 0.9f * alpha);
	}

	// Checking for additive renderstyle
	if (quad->flags & TRANSADD)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	else
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Setup colour/light
	setLight(quad->colour, quad->light, alpha);

	// Setup fog
	setFog(quad->fogcolour, quad->light);

	// Setup DRAWBOTH
	if (quad->flags & DRAWBOTH)
		glDisable(GL_CULL_FACE);

	// Draw quad
	glBegin(GL_QUADS);
	glTexCoord2f(quad->points[0].tx, quad->points[0].ty);
	glVertex3f(quad->points[0].x, quad->points[0].y, quad->points[0].z);
	glTexCoord2f(quad->points[1].tx, quad->points[1].ty);
	glVertex3f(quad->points[1].x, quad->points[1].y, quad->points[1].z);
	glTexCoord2f(quad->points[2].tx, quad->points[2].ty);
	glVertex3f(quad->points[2].x, quad->points[2].y, quad->points[2].z);
	glTexCoord2f(quad->points[3].tx, quad->points[3].ty);
	glVertex3f(quad->points[3].x, quad->points[3].y, quad->points[3].z);
	glEnd();

	// Reset settings
	if (quad->colour.a == 255)
	{
		if (quad->flags & SKY && render_3d_sky)
			glEnable(GL_ALPHA_TEST);
		else if (quad->flags & MIDTEX)
			glAlphaFunc(GL_GREATER, 0.0f);
	}
	if (quad->flags & DRAWBOTH)
		glEnable(GL_CULL_FACE);
}

// -----------------------------------------------------------------------------
// Renders all currently visible wall quads
// -----------------------------------------------------------------------------
void MapRenderer3D::renderWalls()
{
	// Init
	quads_transparent_.clear();
	glEnable(GL_TEXTURE_2D);
	glCullFace(GL_BACK);

	// Render all visible quads, ordered by texture
	unsigned a        = 0;
	unsigned tex_last = 0;
	while (n_quads_ > 0)
	{
		tex_last = 0;
		a        = 0;
		while (a < n_quads_)
		{
			// Check alpha
			if (quads_[a]->colour.a < 255)
			{
				quads_transparent_.push_back(quads_[a]);
				quads_[a] = quads_[--n_quads_];
				continue;
			}

			// Check texture
			if (!tex_last && quads_[a]->texture)
			{
				tex_last = quads_[a]->texture;
				gl::Texture::bind(quads_[a]->texture);
			}
			if (quads_[a]->texture != tex_last)
			{
				a++;
				continue;
			}

			// Render quad
			renderQuad(quads_[a], quads_[a]->alpha);
			quads_[a] = quads_[--n_quads_];
		}
	}

	glDisable(GL_TEXTURE_2D);
}

// -----------------------------------------------------------------------------
// Renders all currently visible transparent wall quads
// -----------------------------------------------------------------------------
void MapRenderer3D::renderTransparentWalls()
{
	// Init
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);
	glDisable(GL_ALPHA_TEST);
	glCullFace(GL_BACK);

	// Render all transparent quads
	for (auto& quad : quads_transparent_)
	{
		// Bind texture
		gl::Texture::bind(quad->texture, false);

		// Render quad
		renderQuad(quad, quad->alpha);
	}

	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);
	glEnable(GL_ALPHA_TEST);
}

// -----------------------------------------------------------------------------
// Renders selection overlay for all selected wall quads
// -----------------------------------------------------------------------------
void MapRenderer3D::renderWallSelection(const ItemSelection& selection, float alpha)
{
	if (!render_selection_)
		return;

	// Setup gl stuff
	glLineWidth(2.0f);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Setup colour
	auto& def  = colourconfig::colDef("map_3d_selection");
	auto  col1 = def.colour;
	col1.a *= alpha;
	gl::setColour(col1);
	auto col2 = col1;
	col2.a *= 0.5;

	// Go through selection
	for (auto item : selection)
	{
		// Ignore if not a wall selection
		if (item.type != mapeditor::ItemType::WallBottom && item.type != mapeditor::ItemType::WallMiddle
			&& item.type != mapeditor::ItemType::WallTop /* && selection[a].type != MapEditor::ItemType::Wall3DFloor*/)
			continue;

		// Get side
		auto side = map_->side(item.real_index >= 0 ? item.real_index : item.index);
		if (!side)
			continue;

		// Get parent line index
		int line = side->parentLine()->index();

		// Get appropriate quad
		Quad* quad = nullptr;
		for (unsigned q = 0; q < lines_[line].quads.size(); q++)
		{
			// Check quad is correct side
			if (map_->line(line)->s1() == side && lines_[line].quads[q].flags & BACK)
				continue;
			if (map_->line(line)->s2() == side && (lines_[line].quads[q].flags & BACK) == 0)
				continue;

			// Check every quad in this line
			if (item.real_index >= 0)
			{
				for (unsigned m = q; m < lines_[line].quads.size(); m++)
				{
					if (item.control_line == lines_[line].quads[m].control_line)
					{
						quad = &lines_[line].quads[m];
						break;
					}
				}
			}
			else if (lines_[line].quads[q].flags & UPPER)
			{
				if (item.type == mapeditor::ItemType::WallTop)
				{
					quad = &lines_[line].quads[q];
					break;
				}
			}
			else if (lines_[line].quads[q].flags & LOWER)
			{
				if (item.type == mapeditor::ItemType::WallBottom)
				{
					quad = &lines_[line].quads[q];
					break;
				}
			}
			else if (item.type == mapeditor::ItemType::WallMiddle)
			{
				quad = &lines_[line].quads[q];
				break;
			} /*
			 else if (selection[a].type == MapEditor::ItemType::Wall3DFloor && selection[a].control_line ==
			 lines[line].quads[a].control_line)
			 {
				 quad = &lines[line].quads[a];
				 break;
			 }*/
		}

		if (!quad)
			continue;

		// Render quad outline
		gl::setColour(col1);
		glBegin(GL_LINE_LOOP);
		for (auto& point : quad->points)
			glVertex3f(point.x, point.y, point.z);
		glEnd();

		// Render quad fill
		gl::setColour(col2);
		glBegin(GL_QUADS);
		for (auto& point : quad->points)
			glVertex3f(point.x, point.y, point.z);
		glEnd();
	}
}

// -----------------------------------------------------------------------------
// Updates cached data for [thing] (at [index])
// -----------------------------------------------------------------------------
void MapRenderer3D::updateThing(unsigned index, const MapThing* thing)
{
	// Check index
	if (index >= things_.size() || !thing)
		return;

	// Setup thing info
	things_[index].type   = &(game::configuration().thingType(thing->type()));
	things_[index].sector = map_->sectors().atPos(thing->position());

	// Get sprite texture
	uint32_t theight      = render_thing_icon_size;
	things_[index].sprite = mapeditor::textureManager()
								.sprite(
									things_[index].type->sprite(),
									things_[index].type->translation(),
									things_[index].type->palette())
								.gl_id;
	if (!things_[index].sprite)
	{
		// Sprite not found, try an icon
		if (use_zeth_icons && things_[index].type->zethIcon() >= 0)
		{
			things_[index].sprite = mapeditor::textureManager()
										.editorImage(
											fmt::format("zethicons/zeth{:02d}", things_[index].type->zethIcon()))
										.gl_id;
			things_[index].flags |= ZETH;
		}
		if (!things_[index].sprite)
			things_[index].sprite = mapeditor::textureManager()
										.editorImage(fmt::format("thing/{}", things_[index].type->icon()))
										.gl_id;
		things_[index].flags |= ICON;
	}
	else
		theight = things_[index].type->scaleY() * gl::Texture::info(things_[index].sprite).size.y;
	if (!things_[index].sprite)
	{
		// Icon not found either, use unknown icon
		things_[index].sprite = mapeditor::textureManager().editorImage("thing/unknown").gl_id;
	}

	// Determine z position
	if (things_[index].type->zHeightAbsolute())
		things_[index].z = thing->zPos();
	else if (things_[index].sector)
	{
		// true = thing's Z relative to floor | false = thing's Z relative to ceiling
		bool gravity = !things_[index].type->hanging();

		if (game::configuration().currentGame() == "srb2" && things_[index].flags & 2)
		{
			// Sonic robo blast 2 things contain a "Flip" flag that inverts the thing's gravity if set
			gravity = !gravity;
		}

		// Get sector floor (or ceiling) height
		int   sheight;
		float zheight = thing->zPos();
		if (!gravity)
		{
			sheight = things_[index].sector->ceiling().plane.heightAt(thing->xPos(), thing->yPos());
			sheight -= theight;
			zheight = -zheight;
		}
		else
		{
			sheight = things_[index].sector->floor().plane.heightAt(thing->xPos(), thing->yPos());
		}

		// Set height
		things_[index].z = sheight;
		if (things_[index].type->shrinkOnZoom())
			things_[index].z -= render_thing_icon_size * 0.5;
		if (things_[index].z < sheight)
			things_[index].z = sheight;
		things_[index].z += zheight;
	}

	// Adjust height by sprite Y offset if needed
	things_[index].z += mapeditor::textureManager().verticalOffset(things_[index].type->sprite());

	things_[index].updated_time = app::runTimer();
}

// -----------------------------------------------------------------------------
// Renders all currently visible things
// -----------------------------------------------------------------------------
void MapRenderer3D::renderThings()
{
	// Init
	glEnable(GL_TEXTURE_2D);
	glCullFace(GL_BACK);
	unsigned tex = 0;

	// Go through things
	double dist, halfwidth, theight;
	double mdist = render_max_thing_dist;
	if (mdist <= 0 || mdist > render_max_dist)
		mdist = render_max_dist;
	ColRGBA  col;
	uint8_t  light;
	float    x1, y1, x2, y2;
	unsigned update = 0;
	Seg2d    strafe(cam_position_.get2d(), (cam_position_ + cam_strafe_).get2d());
	for (unsigned a = 0; a < map_->nThings(); a++)
	{
		auto thing       = map_->thing(a);
		things_[a].flags = things_[a].flags & ~DRAWN;

		// Check side of camera
		if (cam_pitch_ > -0.9 && cam_pitch_ < 0.9)
		{
			if (math::lineSide(thing->position(), strafe) > 0)
				continue;
		}

		// Check thing distance if needed
		dist = math::distance(cam_position_.get2d(), thing->position());
		if (mdist > 0 && dist > mdist)
			continue;

		// Update thing if needed
		if (things_[a].updated_time < thing->modifiedTime()
			|| (things_[a].sector
				&& (things_[a].updated_time < things_[a].sector->modifiedTime()
					|| things_[a].updated_time < things_[a].sector->geometryUpdatedTime())))
		{
			updateThing(a, thing);
			update++;
			if (update > 500)
				break;
		}

		// Skip if not shown
		if (!things_[a].type->decoration() && render_3d_things == 2)
			continue;

		// Get thing sprite
		tex = things_[a].sprite;

		// Bind texture if needed
		gl::Texture::bind(tex, false);

		// Determine coordinates
		auto& tex_info = gl::Texture::info(tex);
		halfwidth      = things_[a].type->scaleX() * tex_info.size.x * 0.5;
		theight        = things_[a].type->scaleY() * tex_info.size.y;
		if (things_[a].flags & ICON)
		{
			halfwidth = render_thing_icon_size * 0.5;
			theight   = render_thing_icon_size;
		}
		x1                = thing->xPos() - cam_strafe_.x * halfwidth;
		y1                = thing->yPos() - cam_strafe_.y * halfwidth;
		x2                = thing->xPos() + cam_strafe_.x * halfwidth;
		y2                = thing->yPos() + cam_strafe_.y * halfwidth;
		things_[a].height = theight;

		// Set colour/brightness
		light = 255;
		// If a thing is defined as fullbright but the sprite is missing,
		// we'll fallback on the icon, which needs to be colored as appropriate.
		if (things_[a].type->fullbright() && !(things_[a].flags & ICON))
			col.set(255, 255, 255, 255);
		else
		{
			// Get light level from sector
			if (things_[a].sector)
				light = things_[a].sector->lightAt();

			// Icon, use thing icon colour (not for Zeth icons, though)
			if (things_[a].flags & ICON)
			{
				if (things_[a].flags & ZETH)
					col.set(255, 255, 255, 255);
				else
					col.set(things_[a].type->colour());
			}

			// Otherwise use sector colour
			else if (things_[a].sector)
				col.set(things_[a].sector->colourAt(0, true));
		}
		setLight(col, light, calcDistFade(dist, mdist));
		auto fogcol = ColRGBA(0, 0, 0, 0);
		if (things_[a].sector)
			fogcol = things_[a].sector->fogColour();

		setFog(fogcol, light);

		// Draw thing
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(x1, y1, things_[a].z + theight);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(x1, y1, things_[a].z);
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(x2, y2, things_[a].z);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(x2, y2, things_[a].z + theight);
		glEnd();

		things_[a].flags |= DRAWN;
	}

	// Draw thing borders if needed
	if (render_3d_things_style >= 1)
	{
		glDisable(GL_TEXTURE_2D);
		glDepthMask(GL_FALSE);
		glAlphaFunc(GL_GREATER, 0.2f);
		glDisable(GL_CULL_FACE);
		glLineWidth(3.5f);

		for (unsigned a = 0; a < map_->nThings(); a++)
		{
			// Skip if hidden
			if (!(things_[a].flags & DRAWN))
				continue;

			auto thing = map_->thing(a);
			col.set(things_[a].type->colour());
			float radius = things_[a].type->radius();
			float bottom = things_[a].z + 0.5f;
			float top    = things_[a].z;
			if (things_[a].type->height() < 0)
				top += things_[a].height;
			else
				top += things_[a].type->height();

			// Fill
			glColor4f(col.fr(), col.fg(), col.fb(), 0.21f);
			uint8_t light2  = 255;
			auto    fogcol2 = ColRGBA(0, 0, 0, 0);
			if (things_[a].sector)
			{
				light2  = things_[a].sector->lightAt();
				fogcol2 = things_[a].sector->fogColour();
			}
			setFog(fogcol2, light2);
			glBegin(GL_QUADS);
			// Bottom
			glVertex3f(thing->xPos() - radius, thing->yPos() - radius, bottom);
			glVertex3f(thing->xPos() + radius, thing->yPos() - radius, bottom);
			glVertex3f(thing->xPos() + radius, thing->yPos() + radius, bottom);
			glVertex3f(thing->xPos() - radius, thing->yPos() + radius, bottom);
			if (render_3d_things_style == 2)
			{
				// Top
				glVertex3f(thing->xPos() + radius, thing->yPos() - radius, top);
				glVertex3f(thing->xPos() - radius, thing->yPos() - radius, top);
				glVertex3f(thing->xPos() - radius, thing->yPos() + radius, top);
				glVertex3f(thing->xPos() + radius, thing->yPos() + radius, top);
				// North
				glVertex3f(thing->xPos() - radius, thing->yPos() - radius, top);
				glVertex3f(thing->xPos() - radius, thing->yPos() - radius, bottom);
				glVertex3f(thing->xPos() + radius, thing->yPos() - radius, bottom);
				glVertex3f(thing->xPos() + radius, thing->yPos() - radius, top);
				// South
				glVertex3f(thing->xPos() + radius, thing->yPos() + radius, top);
				glVertex3f(thing->xPos() + radius, thing->yPos() + radius, bottom);
				glVertex3f(thing->xPos() - radius, thing->yPos() + radius, bottom);
				glVertex3f(thing->xPos() - radius, thing->yPos() + radius, top);
				// East
				glVertex3f(thing->xPos() + radius, thing->yPos() - radius, top);
				glVertex3f(thing->xPos() + radius, thing->yPos() - radius, bottom);
				glVertex3f(thing->xPos() + radius, thing->yPos() + radius, bottom);
				glVertex3f(thing->xPos() + radius, thing->yPos() + radius, top);
				// West
				glVertex3f(thing->xPos() - radius, thing->yPos() + radius, top);
				glVertex3f(thing->xPos() - radius, thing->yPos() + radius, bottom);
				glVertex3f(thing->xPos() - radius, thing->yPos() - radius, bottom);
				glVertex3f(thing->xPos() - radius, thing->yPos() - radius, top);
			}
			glEnd();

			// Outline
			glColor4f(col.fr(), col.fg(), col.fb(), 0.6f);
			// Bottom
			glBegin(GL_LINE_LOOP);
			glVertex3f(thing->xPos() - radius, thing->yPos() - radius, bottom);
			glVertex3f(thing->xPos() + radius, thing->yPos() - radius, bottom);
			glVertex3f(thing->xPos() + radius, thing->yPos() + radius, bottom);
			glVertex3f(thing->xPos() - radius, thing->yPos() + radius, bottom);
			glEnd();
			if (render_3d_things_style == 2)
			{
				// Top
				glBegin(GL_LINE_LOOP);
				glVertex3f(thing->xPos() - radius, thing->yPos() - radius, top);
				glVertex3f(thing->xPos() + radius, thing->yPos() - radius, top);
				glVertex3f(thing->xPos() + radius, thing->yPos() + radius, top);
				glVertex3f(thing->xPos() - radius, thing->yPos() + radius, top);
				glEnd();
				// Corners
				glBegin(GL_LINES);
				glVertex3f(thing->xPos() - radius, thing->yPos() - radius, bottom);
				glVertex3f(thing->xPos() - radius, thing->yPos() - radius, top);
				glVertex3f(thing->xPos() + radius, thing->yPos() - radius, bottom);
				glVertex3f(thing->xPos() + radius, thing->yPos() - radius, top);
				glVertex3f(thing->xPos() + radius, thing->yPos() + radius, bottom);
				glVertex3f(thing->xPos() + radius, thing->yPos() + radius, top);
				glVertex3f(thing->xPos() - radius, thing->yPos() + radius, bottom);
				glVertex3f(thing->xPos() - radius, thing->yPos() + radius, top);
				glEnd();
			}

			// Direction
			glPushMatrix();
			glTranslatef(thing->xPos(), thing->yPos(), bottom);
			glRotated(thing->angle(), 0, 0, 1);
			glBegin(GL_LINES);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(radius, 0.0f, 0.0f);
			glVertex3f(radius, 0.0f, 0.0f);
			glVertex3f(radius - (radius * 0.2f), -radius * 0.2f, 0.0f);
			glVertex3f(radius, 0.0f, 0.0f);
			glVertex3f(radius - (radius * 0.2f), radius * 0.2f, 0.0f);
			glEnd();
			glPopMatrix();
		}

		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
	}
}

// -----------------------------------------------------------------------------
// Renders selection overlay for all selected things
// -----------------------------------------------------------------------------
void MapRenderer3D::renderThingSelection(const ItemSelection& selection, float alpha)
{
	// Do nothing if no things visible
	if (render_3d_things == 0 || !render_selection_)
		return;

	// Setup gl stuff
	glLineWidth(2.0f);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);

	// Setup colour
	auto col1 = colourconfig::colour("map_3d_selection");
	col1.a *= alpha;
	gl::setColour(col1);
	auto col2 = col1;
	col2.a *= 0.5;

	// Go through selection
	double halfwidth, theight, x1, y1, x2, y2;
	for (auto item : selection)
	{
		// Ignore if not a thing selection
		if (item.type != mapeditor::ItemType::Thing)
			continue;

		// Get thing
		auto thing = item.asThing(*map_);
		if (!thing)
			return;

		// Update if required
		if (things_[item.index].type == nullptr)
			updateThing(item.index, thing);

		// Skip if not shown
		if (!things_[item.index].type->decoration() && render_3d_things == 2)
			continue;

		// Determine coordinates
		auto& tex_info = gl::Texture::info(things_[item.index].sprite);
		halfwidth      = tex_info.size.x * 0.5;
		theight        = tex_info.size.y;
		if (things_[item.index].flags & ICON)
		{
			halfwidth = render_thing_icon_size * 0.5;
			theight   = render_thing_icon_size;
		}
		x1 = thing->xPos() - cam_strafe_.x * halfwidth;
		y1 = thing->yPos() - cam_strafe_.y * halfwidth;
		x2 = thing->xPos() + cam_strafe_.x * halfwidth;
		y2 = thing->yPos() + cam_strafe_.y * halfwidth;

		// Render outline
		double z = things_[item.index].z;
		gl::setColour(col1);
		glBegin(GL_LINE_LOOP);
		glVertex3f(x1, y1, z + theight);
		glVertex3f(x1, y1, z);
		glVertex3f(x2, y2, z);
		glVertex3f(x2, y2, z + theight);
		glEnd();

		// Render fill
		gl::setColour(col2);
		glBegin(GL_QUADS);
		glVertex3f(x1, y1, z + theight);
		glVertex3f(x1, y1, z);
		glVertex3f(x2, y2, z);
		glVertex3f(x2, y2, z + theight);
		glEnd();
	}
}

// -----------------------------------------------------------------------------
// (Re)builds the flats Vertex Buffer Object
// -----------------------------------------------------------------------------
void MapRenderer3D::updateFlatsVBO()
{
	if (!flats_use_vbo)
		return;

	// Create VBOs if needed
	if (vbo_flats_ == 0)
		glGenBuffers(1, &vbo_flats_);

	// Get total size needed
	unsigned totalsize = 0;
	for (unsigned a = 0; a < map_->nSectors(); a++)
	{
		// Create the sector flats structure, but don't try to update VBOs yet
		// (since this function is recreating them)
		if (isSectorStale(a))
			updateSectorFlats(a);

		MapSector* sector = map_->sector(a);
		Polygon2D* poly   = sector->polygon();
		totalsize += poly->vboDataSize() * sector_flats_[a].size();
	}

	// Allocate buffer data
	glBindBuffer(GL_ARRAY_BUFFER, vbo_flats_);
	Polygon2D::setupVBOPointers();
	glBufferData(GL_ARRAY_BUFFER, totalsize, nullptr, GL_STATIC_DRAW);

	// Write polygon data to VBO
	unsigned offset = 0;
	for (unsigned a = 0; a < sector_flats_.size(); a++)
	{
		MapSector* sector = sector_flats_[a][0].sector;
		Polygon2D* poly   = sector->polygon();

		// TODO i realize we'll have to do this if any 3d floors are /added/, too
		for (unsigned b = 0; b < sector_flats_[a].size(); b++)
		{
			// Write flat to VBO
			sector_flats_[a][b].vbo_offset = offset;
			updateFlatTexCoords(a, b);
			poly->setZ(sector_flats_[a][b].plane);
			offset = poly->writeToVBO(offset);
		}

		// Reset polygon z
		poly->setZ(0.0f);
	}

	// Clean up
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// -----------------------------------------------------------------------------
// (Re)builds the walls Vertex Buffer Object
// (or would, if it were used for wall rendering)
// -----------------------------------------------------------------------------
void MapRenderer3D::updateWallsVBO() const {}

// -----------------------------------------------------------------------------
// Runs a quick check of all sector bounding boxes against the current view to
// hide any that are outside it
// -----------------------------------------------------------------------------
void MapRenderer3D::quickVisDiscard()
{
	// Create sector distance array if needed
	if (dist_sectors_.size() != map_->nSectors())
		dist_sectors_.resize(map_->nSectors());

	// Go through all sectors
	auto   cam = cam_position_.get2d();
	double min_dist, dist;
	Seg2d  strafe(cam, cam + cam_strafe_.get2d());
	for (unsigned a = 0; a < map_->nSectors(); a++)
	{
		// Get sector bbox
		auto bbox = map_->sector(a)->boundingBox();

		// Init to visible
		dist_sectors_[a] = 0.0f;

		// Check if within bbox
		if (bbox.contains(cam))
			continue;

		// Check side of camera
		if (cam_pitch_ > -0.9 && cam_pitch_ < 0.9)
		{
			if (math::lineSide(bbox.min, strafe) > 0 && math::lineSide(Vec2d(bbox.max.x, bbox.min.y), strafe) > 0
				&& math::lineSide(bbox.max, strafe) > 0 && math::lineSide(Vec2d(bbox.min.x, bbox.max.y), strafe) > 0)
			{
				// Behind camera, invisible
				dist_sectors_[a] = -1.0f;
				continue;
			}
		}

		// Check distance to bbox
		if (render_max_dist > 0)
		{
			min_dist = 9999999;
			dist     = math::distanceToLine(cam, bbox.leftSide());
			if (dist < min_dist)
				min_dist = dist;
			dist = math::distanceToLine(cam, bbox.topSide());
			if (dist < min_dist)
				min_dist = dist;
			dist = math::distanceToLine(cam, bbox.rightSide());
			if (dist < min_dist)
				min_dist = dist;
			dist = math::distanceToLine(cam, bbox.bottomSide());
			if (dist < min_dist)
				min_dist = dist;

			dist_sectors_[a] = dist;
		}
	}

	// Set all lines that are part of invisible sectors to invisible
	for (unsigned a = 0; a < map_->nSides(); a++)
	{
		dist = dist_sectors_[map_->side(a)->sector()->index()];

		lines_[map_->side(a)->parentLine()->index()].visible = !(
			dist < 0 || (render_max_dist > 0 && dist > render_max_dist));
	}
}

// -----------------------------------------------------------------------------
// Calculates and returns the faded alpha value for [distance] from the camera
// -----------------------------------------------------------------------------
float MapRenderer3D::calcDistFade(double distance, double max) const
{
	if (max <= 0)
		return 1.0f;

	float faderange = max * 0.2f;
	if (distance > max - faderange)
		return 1.0f - ((distance - (max - faderange)) / faderange);
	else
		return 1.0f;
}

// -----------------------------------------------------------------------------
// Checks and hides any quads that are not currently in view
// -----------------------------------------------------------------------------
void MapRenderer3D::checkVisibleQuads()
{
	// Create quads array if empty
	if (!quads_)
		quads_ = new Quad*[map_->nLines() * 4];

	// Go through lines
	MapLine* line;
	float    distfade;
	n_quads_         = 0;
	unsigned updates = 0;
	bool     update  = false;
	Seg2d    strafe(cam_position_.get2d(), (cam_position_ + cam_strafe_).get2d());
	for (unsigned a = 0; a < lines_.size(); a++)
	{
		line = map_->line(a);

		// Skip if not visible
		if (!lines_[a].visible)
			continue;

		// Check side of camera
		if (cam_pitch_ > -0.9 && cam_pitch_ < 0.9)
		{
			if (math::lineSide(line->start(), strafe) > 0 && math::lineSide(line->end(), strafe) > 0)
				continue;
		}

		// Check for distance fade
		if (render_max_dist > 0)
			distfade = calcDistFade(math::distanceToLine(cam_position_.get2d(), line->seg()), render_max_dist);
		else
			distfade = 1.0f;

		// Update line if needed
		update = false;
		if (lines_[a].updated_time < line->modifiedTime()) // Check line modified
			update = true;
		if (lines_[a].line != line)
			update = true;
		if (!update && line->s1())
		{
			// Check front side/sector modified
			if (lines_[a].updated_time < line->s1()->modifiedTime()
				|| lines_[a].updated_time < line->frontSector()->modifiedTime()
				|| lines_[a].updated_time < line->frontSector()->geometryUpdatedTime())
				update = true;

			MapSector* sector = line->frontSector();
			for (const auto& extra_floor : sector->extraFloors())
			{
				MapLine* control_line = map_->line(extra_floor.control_line_index);
				if (lines_[a].updated_time < control_line->s1()->modifiedTime()
					|| lines_[a].updated_time < control_line->frontSector()->modifiedTime()
					|| lines_[a].updated_time < control_line->frontSector()->geometryUpdatedTime())
					update = true;
			}
		}
		if (!update && line->s2())
		{
			// Check back side/sector modified
			if (lines_[a].updated_time < line->s2()->modifiedTime()
				|| lines_[a].updated_time < line->backSector()->modifiedTime()
				|| lines_[a].updated_time < line->backSector()->geometryUpdatedTime())
				update = true;

			MapSector* sector = line->backSector();
			for (const auto& extra_floor : sector->extraFloors())
			{
				MapLine* control_line = map_->line(extra_floor.control_line_index);
				if (lines_[a].updated_time < control_line->s1()->modifiedTime()
					|| lines_[a].updated_time < control_line->frontSector()->modifiedTime()
					|| lines_[a].updated_time < control_line->frontSector()->geometryUpdatedTime())
					update = true;
			}
		}
		if (update)
		{
			updateLine(a);
			// updates++;
			// if (updates > 500)
			//	break;
		}

		// Determine quads to be drawn
		for (auto& quad : lines_[a].quads)
		{
			// Check we're on the right side of the quad
			if (!(quad.flags & DRAWBOTH)
				&& math::lineSide(
					   cam_position_.get2d(),
					   Seg2d(quad.points[0].x, quad.points[0].y, quad.points[2].x, quad.points[2].y))
					   < 0)
				continue;

			quads_[n_quads_] = &quad;
			quad.alpha       = distfade;
			n_quads_++;
		}
	}
}

// -----------------------------------------------------------------------------
// Checks and hides any flats that are not currently in view
// -----------------------------------------------------------------------------
void MapRenderer3D::checkVisibleFlats()
{
	// Update flats array
	delete[] flats_;
	n_flats_ = 0;
	for (unsigned a = 0; a < sector_flats_.size(); a++)
	{
		// Skip if invisible
		if (dist_sectors_[a] < 0)
			continue;

		n_flats_ += sector_flats_[a].size();
	}
	flats_ = new Flat*[n_flats_];

	for(unsigned a = 0; a < n_flats_; a++)
		flats_[a] = nullptr;

	// Go through sectors
	MapSector* sector;
	float      alpha;
	unsigned   flat_idx = 0;
	auto       cam      = cam_position_.get2d();
	for (unsigned a = 0; a < map_->nSectors(); a++)
	{
		sector = map_->sector(a);

		// Skip if invisible
		if (dist_sectors_[a] < 0)
			continue;

		// Check distance if needed
		if (render_max_dist > 0)
		{
			if (dist_sectors_[a] > render_max_dist)
				continue;

			// Double-check distance
			dist_sectors_[a] = sector->distanceTo(cam, render_max_dist);
			if (dist_sectors_[a] > render_max_dist && !sector->boundingBox().contains(cam))
			{
				dist_sectors_[a] = -1;
				continue;
			}
		}

		// Update sector info if needed
		if (isSectorStale(a))
			updateSector(a);

		// Set distance fade alpha
		if (render_max_dist > 0)
			alpha = calcDistFade(dist_sectors_[a], render_max_dist);
		else
			alpha = 1.0f;

		// TODO this used to add all the floors, then all the ceilings, which would reduce the number of GL state calls
		// -- can probably fix that

		// Add flats
		for (unsigned b = 0; b < sector_flats_[a].size(); b++)
		{
			Flat& flat         = sector_flats_[a][b];
			flat.alpha         = alpha;
			flats_[flat_idx++] = &flat;

			// For two-sided flats, update which plane is currently visible
			/*if (flat.flags & DRAWBOTH)
			{
				if (cam_position_.z < flat.plane.height_at(cam_position_.x, cam_position_.y))
					flat.flags |= CEIL;
				else
					flat.flags &= ~CEIL;
			}*/
		}
	}
}

// -----------------------------------------------------------------------------
// Finds the closest wall/flat/thing to the camera along the view vector
// -----------------------------------------------------------------------------
mapeditor::Item MapRenderer3D::determineHilight()
{
	// Init
	double          min_dist = 9999999;
	mapeditor::Item current;
	Seg2d           strafe(cam_position_.get2d(), (cam_position_ + cam_strafe_).get2d());

	// Check for required map structures
	if (!map_ || lines_.size() != map_->nLines() || sector_flats_.size() != map_->nSectors()
		|| things_.size() != map_->nThings())
		return current;

	// Check lines
	double height, dist;
	for (unsigned a = 0; a < map_->nLines(); a++)
	{
		// Ignore if not visible
		if (!lines_[a].visible)
			continue;

		auto line = map_->line(a);

		// Find (2d) distance to line
		dist = math::distanceRayLine(
			cam_position_.get2d(), (cam_position_ + cam_dir3d_).get2d(), line->start(), line->end());

		// Ignore if no intersection or something was closer
		if (dist < 0 || dist >= min_dist)
			continue;

		// Find quad intersect if any
		auto intersection = cam_position_ + cam_dir3d_ * dist;
		for (auto& quad : lines_[a].quads)
		{
			// Check side of camera
			if (!(quad.flags & DRAWBOTH)
				&& math::lineSide(
					   cam_position_.get2d(),
					   Seg2d(quad.points[0].x, quad.points[0].y, quad.points[2].x, quad.points[2].y))
					   < 0)
				continue;

			// Check intersection height
			// Need to handle slopes by finding the floor and ceiling height of
			// the quad at the intersection point
			Vec2d  seg_left           = Vec2d(quad.points[1].x, quad.points[1].y);
			Vec2d  seg_right          = Vec2d(quad.points[2].x, quad.points[2].y);
			double dist_along_segment = (intersection.get2d() - seg_left).magnitude()
										/ (seg_right - seg_left).magnitude();
			double top    = quad.points[0].z + (quad.points[3].z - quad.points[0].z) * dist_along_segment;
			double bottom = quad.points[1].z + (quad.points[2].z - quad.points[1].z) * dist_along_segment;
			if (bottom <= intersection.z && intersection.z <= top)
			{
				// Determine selected item from quad flags

				// Side index
				if (quad.flags & BACK)
					current.index = line->s2Index();
				else
					current.index = line->s1Index();

				// Side part
				if (quad.control_side >= 0)
				{
					current.type         = mapeditor::ItemType::WallMiddle;
					current.real_index   = current.index;
					current.control_line = quad.control_line;
					current.index        = quad.control_side;
				}
				else if (quad.flags & UPPER)
					current.type = mapeditor::ItemType::WallTop;
				else if (quad.flags & LOWER)
					current.type = mapeditor::ItemType::WallBottom;
				else
					current.type = mapeditor::ItemType::WallMiddle;

				min_dist = dist;
			}
		}
	}

	// Check sectors
	for (unsigned a = 0; a < map_->nSectors(); a++)
	{
		// Ignore if not visible
		if (dist_sectors_[a] < 0)
			continue;

		// Check distance to sector planes
		for (unsigned b = 0; b < sector_flats_[a].size(); b++)
		{
			const Flat& flat = sector_flats_[a][b];

			dist = math::distanceRayPlane(cam_position_, cam_dir3d_, sector_flats_[a][b].plane);
			if (dist < 0 || dist >= min_dist)
				continue;

			// Check if on the correct side of the plane
			double flat_z = sector_flats_[a][b].plane.heightAt(cam_position_.x, cam_position_.y);
			if (!(flat.flags & DRAWBOTH))
			{
				if (flat.flags & FLATFLIP)
				{
					if (flat.flags & CEIL && cam_position_.z <= flat_z)
						continue;
					if (!(flat.flags & CEIL) && cam_position_.z >= flat_z)
						continue;
				}
				else
				{
					if (flat.flags & CEIL && cam_position_.z >= flat_z)
						continue;
					if (!(flat.flags & CEIL) && cam_position_.z <= flat_z)
						continue;
				}
			}

			// Check if intersection is within sector
			if (!map_->sector(a)->containsPoint((cam_position_ + cam_dir3d_ * dist).get2d()))
				continue;

			if (flat.extra_floor_index < 0)
				current.index = a;
			else
			{
				current.index      = flat.control_sector->index();
				current.real_index = a;
			}

			min_dist = dist;
			if (flat.flags & CEIL)
				current.type = mapeditor::ItemType::Ceiling;
			else
				current.type = mapeditor::ItemType::Floor;
		}
	}

	// Update item distance
	if (min_dist >= 9999999 || min_dist < 0)
		item_dist_ = -1;
	else
		item_dist_ = math::round(min_dist);

	// Check things (if visible)
	if (render_3d_things == 0)
		return current;
	double halfwidth, theight;
	for (unsigned a = 0; a < map_->nThings(); a++)
	{
		// Ignore if no sprite
		if (!things_[a].sprite)
			continue;

		// Ignore if not visible
		auto thing = map_->thing(a);
		if (math::lineSide(thing->position(), strafe) > 0)
			continue;

		// Ignore if not shown
		if (!things_[a].type->decoration() && render_3d_things == 2)
			continue;

		// Find distance to thing sprite
		auto& tex_info = gl::Texture::info(things_[a].sprite);
		halfwidth      = tex_info.size.x * 0.5;
		if (things_[a].flags & ICON)
			halfwidth = render_thing_icon_size * 0.5;
		dist = math::distanceRayLine(
			cam_position_.get2d(),
			(cam_position_ + cam_dir3d_).get2d(),
			thing->position() - cam_strafe_.get2d() * halfwidth,
			thing->position() + cam_strafe_.get2d() * halfwidth);

		// Ignore if no intersection or something was closer
		if (dist < 0 || dist >= min_dist)
			continue;

		// Check intersection height
		theight = tex_info.size.y;
		height  = cam_position_.z + cam_dir3d_.z * dist;
		if (things_[a].flags & ICON)
			theight = render_thing_icon_size;
		if (height >= things_[a].z && height <= things_[a].z + theight)
		{
			current.index = a;
			current.type  = mapeditor::ItemType::Thing;
			min_dist      = dist;
		}
	}

	// Update item distance
	if (min_dist >= 9999999 || min_dist < 0)
		item_dist_ = -1;
	else
		item_dist_ = math::round(min_dist);

	return current;
}

// -----------------------------------------------------------------------------
// Renders the hilight overlay for the currently hilighted object
// -----------------------------------------------------------------------------
void MapRenderer3D::renderHilight(mapeditor::Item hilight, float alpha)
{
	// Do nothing if no item hilighted
	if (hilight.index < 0 || render_3d_hilight == 0 || !render_hilight_)
		return;

	// Setup gl stuff
	glLineWidth(3.0f);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	auto& def         = colourconfig::colDef("map_3d_hilight");
	auto  col_hilight = def.colour;
	col_hilight.a *= alpha;
	gl::setColour(col_hilight);

	// Quad hilight
	if (hilight.type == mapeditor::ItemType::WallBottom || hilight.type == mapeditor::ItemType::WallMiddle
		|| hilight.type == mapeditor::ItemType::WallTop /*|| hilight.type == MapEditor::ItemType::Wall3DFloor*/)
	{
		// Get side
		auto side = map_->side(hilight.real_index >= 0 ? hilight.real_index : hilight.index);
		if (!side)
			return;

		// Get parent line index
		int line = side->parentLine()->index();

		// Get appropriate quad
		Quad* quad = nullptr;
		for (unsigned a = 0; a < lines_[line].quads.size(); a++)
		{
			// Check quad is correct side
			if (map_->line(line)->s1() == side && lines_[line].quads[a].flags & BACK)
				continue;

			// Check quad is correct part
			if (hilight.real_index >= 0)
			{
				for (unsigned m = a; m < lines_[line].quads.size(); m++)
				{
					if (hilight.control_line == lines_[line].quads[a].control_line)
					{
						quad = &lines_[line].quads[a];
						break;
					}
				}
			}
			else if (lines_[line].quads[a].flags & UPPER)
			{
				if (hilight.type == mapeditor::ItemType::WallTop)
				{
					quad = &lines_[line].quads[a];
					break;
				}
			}
			else if (lines_[line].quads[a].flags & LOWER)
			{
				if (hilight.type == mapeditor::ItemType::WallBottom)
				{
					quad = &lines_[line].quads[a];
					break;
				}
			}
			else if (hilight.type == mapeditor::ItemType::WallMiddle)
			{
				quad = &lines_[line].quads[a];
				break;
			}
		}

		if (!quad)
			return;

		// Render outline
		glBegin(GL_LINE_LOOP);
		for (auto& point : quad->points)
			glVertex3f(point.x, point.y, point.z);
		glEnd();

		// Render fill (if needed)
		if (render_3d_hilight > 1)
		{
			glCullFace(GL_BACK);
			col_hilight.a *= 0.3;
			gl::setColour(col_hilight);
			glBegin(GL_QUADS);
			for (auto& point : quad->points)
				glVertex3f(point.x, point.y, point.z);
			glEnd();
		}
	}

	// Sector hilight
	if (hilight.type == mapeditor::ItemType::Floor || hilight.type == mapeditor::ItemType::Ceiling)
	{
		// Get sector
		auto sector = map_->sector(hilight.real_index >= 0 ? hilight.real_index : hilight.index);
		if (!sector)
			return;

		// Get plane
		Plane plane;
		if (hilight.real_index >= 0)
		{
			for (const auto& extra_floor : sector->extraFloors())
			{
				if (extra_floor.control_sector_index == hilight.index)
				{
					auto& extra = extra_floor;
					if (hilight.type == mapeditor::ItemType::Floor)
						plane = extra.floor_plane;
					else
						plane = extra.ceiling_plane;
				}
			}
		}
		else
		{
			if (hilight.type == mapeditor::ItemType::Floor)
				plane = sector->floor().plane;
			else
				plane = sector->ceiling().plane;
		}

		// Render sector outline
		vector<MapLine*> lines;
		sector->putLines(lines);
		glBegin(GL_LINES);
		for (auto& line : lines)
		{
			glVertex3d(line->x1(), line->y1(), plane.heightAt(line->x1(), line->y1()));
			glVertex3d(line->x2(), line->y2(), plane.heightAt(line->x2(), line->y2()));
		}
		glEnd();

		// Render fill if needed
		if (render_3d_hilight > 1)
		{
			col_hilight.a *= 0.3;
			gl::setColour(col_hilight);
			glDisable(GL_CULL_FACE);
			sector->polygon()->setZ(plane);
			sector->polygon()->render();
			sector->polygon()->setZ(0);
			glEnable(GL_CULL_FACE);
		}
	}

	// Thing hilight
	double x1, y1, x2, y2;
	if (hilight.type == mapeditor::ItemType::Thing)
	{
		// Get thing
		auto thing = hilight.asThing(*map_);
		if (!thing)
			return;

		// Determine coordinates
		auto&  tex_info  = gl::Texture::info(things_[hilight.index].sprite);
		double halfwidth = things_[hilight.index].type->scaleX() * tex_info.size.x * 0.5;
		double theight   = things_[hilight.index].type->scaleY() * tex_info.size.y;
		if (things_[hilight.index].flags & ICON)
		{
			halfwidth = render_thing_icon_size * 0.5;
			theight   = render_thing_icon_size;
		}
		x1 = thing->xPos() - cam_strafe_.x * halfwidth;
		y1 = thing->yPos() - cam_strafe_.y * halfwidth;
		x2 = thing->xPos() + cam_strafe_.x * halfwidth;
		y2 = thing->yPos() + cam_strafe_.y * halfwidth;

		// Render outline of sprite
		double z = things_[hilight.index].z;
		glBegin(GL_LINE_LOOP);
		glVertex3f(x1, y1, z + theight);
		glVertex3f(x1, y1, z);
		glVertex3f(x2, y2, z);
		glVertex3f(x2, y2, z + theight);
		glEnd();

		// Render fill if needed
		if (render_3d_hilight > 1)
		{
			glCullFace(GL_BACK);
			col_hilight.a *= 0.3;
			gl::setColour(col_hilight);
			glBegin(GL_QUADS);
			glVertex3f(x1, y1, z + theight);
			glVertex3f(x1, y1, z);
			glVertex3f(x2, y2, z);
			glVertex3f(x2, y2, z + theight);
			glEnd();
		}
	}

	// glEnable(GL_DEPTH_TEST);
	gl::setColour(ColRGBA::WHITE);
}

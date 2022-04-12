#pragma once

#include "MCAnimations.h"
#include "MapRenderer2D.h"
#include "MapRenderer3D.h"
#include "RenderView.h"

namespace slade
{
class MapEditContext;
class MCOverlay;

namespace mapeditor
{
	class Renderer
	{
	public:
		Renderer(MapEditContext& context);

		MapRenderer2D& renderer2D() { return renderer_2d_; }
		MapRenderer3D& renderer3D() { return renderer_3d_; }
		RenderView&    view() { return view_; }

		void forceUpdate();

		// View manipulation
		void   setView(double map_x, double map_y);
		void   setViewSize(int width, int height);
		void   setTopY(double map_y);
		void   pan(double x, double y, bool scale = false);
		void   zoom(double amount, bool toward_cursor = false);
		void   viewFitToMap(bool snap = false);
		void   viewFitToObjects(const vector<MapObject*>& objects);
		double interpolateView(bool smooth, double view_speed, double mult);
		bool   viewIsInterpolated() const;

		// 3d Mode
		void  setCameraThing(MapThing* thing);
		Vec2d cameraPos2D() const;
		Vec2d cameraDir2D() const;

		// Drawing
		void draw();

		// Animation
		bool animationsActive() const { return !animations_.empty() || animations_active_; }
		void updateAnimations(double mult);
		void animateSelectionChange(const mapeditor::Item& item, bool selected = true);
		void animateSelectionChange(const ItemSelection& selection);
		void animateHilightChange(const mapeditor::Item& old_item, MapObject* old_object = nullptr);
		void addAnimation(unique_ptr<MCAnimation> animation);

	private:
		MapEditContext& context_;
		MapRenderer2D   renderer_2d_;
		MapRenderer3D   renderer_3d_;
		RenderView      view_;

		// MCAnimations
		vector<unique_ptr<MCAnimation>> animations_;

		// Animation
		bool   animations_active_    = false;
		double anim_view_speed_      = 0.05;
		float  fade_vertices_        = 1.f;
		float  fade_things_          = 1.f;
		float  fade_flats_           = 1.f;
		float  fade_lines_           = 1.f;
		float  anim_flash_level_     = 0.5f;
		bool   anim_flash_inc_       = true;
		float  anim_info_fade_       = 0.f;
		float  anim_overlay_fade_    = 0.f;
		float  anim_help_fade_       = 0.f;
		bool   cursor_zoom_disabled_ = false;


		// Drawing
		void drawGrid() const;
		void drawEditorMessages() const;
		void drawFeatureHelpText() const;
		void drawSelectionNumbers() const;
		void drawThingQuickAngleLines() const;
		void drawLineLength(Vec2d p1, Vec2d p2, ColRGBA col) const;
		void drawLineDrawLines(bool snap_nearest_vertex) const;
		void drawPasteLines() const;
		void drawObjectEdit();
		void drawAnimations() const;
		void drawMap2d();
		void drawMap3d();

		// Animation
		bool update2dModeCrossfade(double mult);
	};
} // namespace mapeditor
} // namespace slade

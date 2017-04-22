#pragma once

#include "MapRenderer2D.h"
#include "MapRenderer3D.h"
#include "MCAnimations.h"

class MapEditContext;
class MCOverlay;

namespace MapEditor
{
	class RenderView
	{
	public:
		RenderView();

		const fpoint2_t&	offset(bool inter = false) const { return inter ? offset_inter_ : offset_; }
		double				scale(bool inter = false) const { return inter ? scale_inter_ : scale_; }
		const point2_t&		size() const { return size_; }
		const frect_t&		mapBounds() const { return map_bounds_; }

		void	setOffset(double x, double y) { offset_ = { x, y }; updateMapBounds(); }
		void	setSize(int width, int height) { size_ = { width, height }; updateMapBounds(); }

		void	resetInter(bool x, bool y, bool scale);
		void	zoom(double amount);
		void	zoomToward(double amount, const fpoint2_t point);
		void	fitTo(bbox_t bbox);
		bool	interpolate(double mult, const fpoint2_t* towards = nullptr);

		// Map <-> Screen Coordinate Translation
		double		mapX(double screen_x, bool inter = false) const;
		double		mapY(double screen_y, bool inter = false) const;
		fpoint2_t	mapPos(const fpoint2_t& screen_pos, bool inter = false) const;
		int			screenX(double map_x) const;
		int			screenY(double map_y) const;

		void	apply();
		void	setOverlayCoords(bool set) const;

	private:
		fpoint2_t	offset_;
		fpoint2_t	offset_inter_;
		double		scale_;
		double		scale_inter_;
		double		min_scale_;
		double		max_scale_;
		point2_t	size_;
		frect_t		map_bounds_;

		void	updateMapBounds();
	};

	class Renderer
	{
	public:
		struct AnimFader
		{
			float fade;
			float speed;

			bool active()
			{
				return fade > 0.0f && fade < 1.0f;
			}

			void update(float mult)
			{
				fade += speed*mult;
				if (fade > 1.0f)
					fade = 1.0f;
				if (fade < 0.0f)
					fade = 0.0f;
			}
		};

		Renderer(MapEditContext& context);

		MapRenderer2D&	renderer2D() { return renderer_2d_; }
		MapRenderer3D&	renderer3D() { return renderer_3d_; }

		fpoint2_t	mapPos(point2_t screen) const { return view_.mapPos(screen, true); }

		// View manipulation
		void	setView(double map_x, double map_y);
		void	setViewSize(int width, int height) { view_.setSize(width, height); }
		void	setTopY(double map_y);
		void	pan(double x, double y);
		void	zoom(double amount, bool toward_cursor = false);
		void	viewFitToMap(bool snap = false);
		void	viewFitToObjects(const vector<MapObject*>& objects);
		double	interpolateView(bool smooth, double view_speed, double mult);
		bool	viewIsInterpolated() const;

		// 3d Mode
		void		setCameraThing(MapThing* thing);
		fpoint2_t	cameraPos2D();
		fpoint2_t	cameraDir2D();

		// Drawing
		// TODO: Make these private once draw() is implemented
		void	drawGrid() const;
		void	drawEditorMessages() const;
		void	drawFeatureHelpText() const;
		void	drawSelectionNumbers() const;
		void	drawThingQuickAngleLines() const;
		void	drawLineLength(fpoint2_t p1, fpoint2_t p2, rgba_t col) const;
		void	drawLineDrawLines(bool snap_nearest_vertex) const;
		void	drawPasteLines() const;
		void	drawObjectEdit() const;
		//void	drawMap2d();
		//void	drawMap3d();

		// MapCanvas migration
		// TODO: Remove these once MapCanvas split complete
		double	viewScale(bool inter = false) const { return view_.scale(inter); }
		double	viewXOff(bool inter = false) const { return view_.offset(inter).x; }
		double	viewYOff(bool inter = false) const { return view_.offset(inter).y; }
		double	translateX(double x, bool inter = false) const { return view_.mapX(x, inter); }
		double	translateY(double y, bool inter = false) const { return view_.mapY(y, inter); }
		int		screenX(double x) const { return view_.screenX(x); }
		int		screenY(double y) const { return view_.screenY(y); }

		void	draw();

	private:
		MapEditContext&	context_;
		MapRenderer2D	renderer_2d_;
		MapRenderer3D	renderer_3d_;
		RenderView		view_;

		// Full-screen overlay
		MCOverlay*  overlay_current_;
	};
}

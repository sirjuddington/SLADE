#pragma once

#include "MapRenderer2D.h"
#include "MapRenderer3D.h"
#include "MCAnimations.h"
#include "RenderView.h"

class MapEditContext;
class MCOverlay;

namespace MapEditor
{
	class Renderer
	{
	public:
		Renderer(MapEditContext& context);

		MapRenderer2D&	renderer2D() { return renderer_2d_; }
		MapRenderer3D&	renderer3D() { return renderer_3d_; }
		RenderView&		view() { return view_; }

		void	forceUpdate();

		// View manipulation
		void	setView(double map_x, double map_y);
		void	setViewSize(int width, int height);
		void	setTopY(double map_y);
		void	pan(double x, double y, bool scale = false);
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
		void	draw();

		// Animation
		bool	animationsActive() const { return !animations_.empty() || animations_active_; }
		void	updateAnimations(double mult);
		void	animateSelectionChange(const MapEditor::Item& item, bool selected = true);
		void	animateSelectionChange(const ItemSelection &selection);
		void	animateHilightChange(const MapEditor::Item& old_item, MapObject* old_object = nullptr);
		void	addAnimation(std::unique_ptr<MCAnimation> animation);

	private:
		MapEditContext&	context_;
		MapRenderer2D	renderer_2d_;
		MapRenderer3D	renderer_3d_;
		RenderView		view_;

		// MCAnimations
		vector<std::unique_ptr<MCAnimation>>	animations_;

		// Animation
		bool	animations_active_;
		double	anim_view_speed_;
		float	fade_vertices_;
		float	fade_things_;
		float	fade_flats_;
		float	fade_lines_;
		float	anim_flash_level_;
		bool	anim_flash_inc_;
		float	anim_info_fade_;
		float	anim_overlay_fade_;
		float	anim_help_fade_;


		// Drawing
		void	drawGrid() const;
		void	drawEditorMessages() const;
		void	drawFeatureHelpText() const;
		void	drawSelectionNumbers() const;
		void	drawThingQuickAngleLines() const;
		void	drawLineLength(fpoint2_t p1, fpoint2_t p2, rgba_t col) const;
		void	drawLineDrawLines(bool snap_nearest_vertex) const;
		void	drawPasteLines() const;
		void	drawObjectEdit();
		void	drawAnimations() const;
		void	drawMap2d();
		void	drawMap3d();

		// Animation
		bool	update2dModeCrossfade(double mult);
	};
}

#pragma once

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

		void	apply() const;
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
}

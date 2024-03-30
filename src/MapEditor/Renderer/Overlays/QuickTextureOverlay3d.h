#pragma once

#include "MCOverlay.h"

namespace slade
{
class ItemSelection;
namespace mapeditor
{
	class MapEditContext;
}

class QuickTextureOverlay3d : public MCOverlay
{
public:
	QuickTextureOverlay3d(mapeditor::MapEditContext* editor);
	~QuickTextureOverlay3d() override = default;

	void setTexture(string_view name);
	void applyTexture() const;

	void update(long frametime) override;

	void   draw(gl::draw2d::Context& dc, float fade = 1.0f) override;
	void   drawTexture(gl::draw2d::Context& dc, unsigned index, double x, double bottom, double size, float fade);
	double determineSize(double x, int width) const;

	void close(bool cancel = false) override;

	void doSearch();
	void keyDown(string_view key) override;

	static bool ok(const ItemSelection& sel);

private:
	struct QTTex
	{
		unsigned texture;
		string   name;
		QTTex(string_view name) : texture{ 0 }, name{ name } {}
	};

	vector<QTTex>              textures_;
	unsigned                   current_index_ = 0;
	string                     search_;
	double                     anim_offset_ = 0.;
	mapeditor::MapEditContext* editor_      = nullptr;
	bool                       sel_flats_   = true;
	bool                       sel_walls_   = true;
};
} // namespace slade

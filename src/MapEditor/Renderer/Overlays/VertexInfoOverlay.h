#pragma once

class MapVertex;
class VertexInfoOverlay
{
public:
	VertexInfoOverlay();
	~VertexInfoOverlay();

	void update(MapVertex* vertex);
	void draw(int bottom, int right, float alpha = 1.0f);

private:
	string info_;
	bool   pos_frac_;
};

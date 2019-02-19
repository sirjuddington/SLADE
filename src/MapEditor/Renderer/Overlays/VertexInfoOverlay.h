#pragma once

class MapVertex;
class VertexInfoOverlay
{
public:
	VertexInfoOverlay()  = default;
	~VertexInfoOverlay() = default;

	void update(MapVertex* vertex);
	void draw(int bottom, int right, float alpha = 1.0f) const;

private:
	std::string info_;
	bool        pos_frac_ = false;
};

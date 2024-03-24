#pragma once

namespace slade
{
class VertexInfoOverlay
{
public:
	VertexInfoOverlay()  = default;
	~VertexInfoOverlay() = default;

	void update(MapVertex* vertex);
	void draw(int bottom, int right, float alpha = 1.0f) const;

private:
	vector<string> info_;
};
} // namespace slade

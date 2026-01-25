#pragma once

namespace slade::map
{
struct LineTranslucency;

class RenderSpecials
{
public:
	explicit RenderSpecials(SLADEMap& map);
	~RenderSpecials() = default;

	optional<LineTranslucency> lineTranslucency(const MapLine& line);

	void processLineSpecial(const MapLine& line);
	void clearSpecials();

	void lineUpdated(const MapLine& line);

private:
	SLADEMap* map_;

	struct LineTransparencySpecial
	{
		const MapLine* line;
		const MapLine* target;
		float          alpha;
		bool           additive;
	};
	vector<LineTransparencySpecial> line_transparency_specials_;
	bool                            line_transparency_specials_sorted_ = false;

	void addTranslucentLine(const MapLine& line, bool zdoom, u8 alpha = 255, bool additive = false);
	void removeTranslucentLine(const MapLine& line);
};
} // namespace slade::map

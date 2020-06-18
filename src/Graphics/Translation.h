#pragma once
#include "Utility/Colour.h"

namespace slade
{
class Translation;

class TransRange
{
	friend class Translation;

public:
	enum class Type
	{
		Palette = 1,
		Colour,
		Desat,
		Blend,
		Tint,
		Special
	};

	struct IndexRange
	{
		uint8_t start = 0;
		uint8_t end   = 0;

		IndexRange(int start, int end) : start{ (uint8_t)start }, end{ (uint8_t)end } {}

		string asText() const { return fmt::format("{}:{}", start, end); }
	};

	TransRange(Type type, IndexRange range) : type_{ type }, range_{ range } {}
	virtual ~TransRange() = default;

	Type              type() const { return type_; }
	const IndexRange& range() const { return range_; }
	uint8_t           start() const { return range_.start; }
	uint8_t           end() const { return range_.end; }

	void setRange(const IndexRange& range) { range_ = range; }
	void setStart(uint8_t val) { range_.start = val; }
	void setEnd(uint8_t val) { range_.end = val; }

	virtual string asText() { return ""; }

protected:
	Type       type_;
	IndexRange range_;
};

class TransRangePalette : public TransRange
{
	friend class Translation;

public:
	TransRangePalette(IndexRange range, IndexRange dest_range) :
		TransRange{ Type::Palette, range },
		dest_range_{ dest_range }
	{
	}
	TransRangePalette(const TransRangePalette& copy) :
		TransRange{ Type::Palette, copy.range_ },
		dest_range_{ copy.dest_range_ }
	{
	}

	uint8_t dStart() const { return dest_range_.start; }
	uint8_t dEnd() const { return dest_range_.end; }

	void setDStart(uint8_t val) { dest_range_.start = val; }
	void setDEnd(uint8_t val) { dest_range_.end = val; }

	string asText() override
	{
		return fmt::format("{}:{}={}:{}", range_.start, range_.end, dest_range_.start, dest_range_.end);
	}

private:
	IndexRange dest_range_;
};

class TransRangeColour : public TransRange
{
	friend class Translation;

public:
	TransRangeColour(
		IndexRange     range,
		const ColRGBA& col_start = ColRGBA::BLACK,
		const ColRGBA& col_end   = ColRGBA::WHITE) :
		TransRange{ Type::Colour, range },
		col_start_{ col_start },
		col_end_{ col_end }
	{
	}
	TransRangeColour(const TransRangeColour& copy) :
		TransRange{ Type::Colour, copy.range_ },
		col_start_{ copy.col_start_ },
		col_end_{ copy.col_end_ }
	{
	}

	const ColRGBA& startColour() const { return col_start_; }
	const ColRGBA& endColour() const { return col_end_; }

	void setStartColour(const ColRGBA& col) { col_start_.set(col); }
	void setEndColour(const ColRGBA& col) { col_end_.set(col); }

	string asText() override
	{
		return fmt::format(
			"{}:{}=[{},{},{}]:[{},{},{}]",
			range_.start,
			range_.end,
			col_start_.r,
			col_start_.g,
			col_start_.b,
			col_end_.r,
			col_end_.g,
			col_end_.b);
	}

private:
	ColRGBA col_start_, col_end_;
};

class TransRangeDesat : public TransRange
{
	friend class Translation;

public:
	struct RGB
	{
		float r, g, b;
	};

	TransRangeDesat(IndexRange range, const RGB& start = { 0, 0, 0 }, const RGB& end = { 2, 2, 2 }) :
		TransRange{ Type::Desat, range },
		rgb_start_{ start },
		rgb_end_{ end }
	{
	}
	TransRangeDesat(const TransRangeDesat& copy) :
		TransRange{ Type::Desat, copy.range_ },
		rgb_start_{ copy.rgb_start_ },
		rgb_end_{ copy.rgb_end_ }
	{
	}

	const RGB& rgbStart() const { return rgb_start_; }
	const RGB& rgbEnd() const { return rgb_end_; }

	void setRGBStart(float r, float g, float b) { rgb_start_ = { r, g, b }; }
	void setRGBEnd(float r, float g, float b) { rgb_end_ = { r, g, b }; }

	string asText() override
	{
		return fmt::format(
			"{}:{}=%[{:1.2f},{:1.2f},{:1.2f}]:[{:1.2f},{:1.2f},{:1.2f}]",
			range_.start,
			range_.end,
			rgb_start_.r,
			rgb_start_.g,
			rgb_start_.b,
			rgb_end_.r,
			rgb_end_.g,
			rgb_end_.b);
	}

private:
	RGB rgb_start_;
	RGB rgb_end_;
};

class TransRangeBlend : public TransRange
{
	friend class Translation;

public:
	TransRangeBlend(IndexRange range, const ColRGBA& colour = ColRGBA::RED) :
		TransRange{ Type::Blend, range },
		colour_{ colour }
	{
	}
	TransRangeBlend(const TransRangeBlend& copy) : TransRange{ Type::Blend, copy.range_ }, colour_{ copy.colour_ } {}

	const ColRGBA& colour() const { return colour_; }
	void           setColour(const ColRGBA& c) { colour_ = c; }

	string asText() override
	{
		return fmt::format("{}:{}=#[{},{},{}]", range_.start, range_.end, colour_.r, colour_.g, colour_.b);
	}

private:
	ColRGBA colour_;
};

class TransRangeTint : public TransRange
{
	friend class Translation;

public:
	TransRangeTint(IndexRange range, const ColRGBA& colour = ColRGBA::RED, uint8_t amount = 50) :
		TransRange{ Type::Tint, range },
		colour_{ colour },
		amount_{ amount }
	{
	}
	TransRangeTint(const TransRangeTint& copy) :
		TransRange{ Type::Tint, copy.range_ },
		colour_{ copy.colour_ },
		amount_{ copy.amount_ }
	{
	}

	ColRGBA colour() const { return colour_; }
	uint8_t amount() const { return amount_; }
	void    setColour(const ColRGBA& c) { colour_ = c; }
	void    setAmount(uint8_t a) { amount_ = a; }

	string asText() override
	{
		return fmt::format("{}:{}=@{}[{},{},{}]", range_.start, range_.end, amount_, colour_.r, colour_.g, colour_.b);
	}

private:
	ColRGBA colour_;
	uint8_t amount_;
};

class TransRangeSpecial : public TransRange
{
	friend class Translation;

public:
	TransRangeSpecial(IndexRange range, string_view special = "") :
		TransRange{ Type::Special, range },
		special_{ special }
	{
	}
	TransRangeSpecial(const TransRangeSpecial& copy) :
		TransRange{ Type::Special, copy.range_ },
		special_{ copy.special_ }
	{
	}

	const string& special() const { return special_; }
	void          setSpecial(string_view sp) { special_ = sp; }

	string asText() override { return fmt::format("{}:{}=${}", range_.start, range_.end, special_); }

private:
	string special_;
};

class Palette;
class Translation
{
public:
	Translation()  = default;
	~Translation() = default;

	const vector<unique_ptr<TransRange>>& ranges() const { return translations_; }

	void        parse(string_view def);
	TransRange* parseRange(string_view range);
	void        read(const uint8_t* data);
	string      asText();
	void        clear();
	void        copy(const Translation& copy);
	bool        isEmpty() const { return built_in_name_.empty() && translations_.empty(); }

	unsigned      nRanges() const { return translations_.size(); }
	TransRange*   range(unsigned index);
	const string& builtInName() const { return built_in_name_; }
	uint8_t       desaturationAmount() const { return desat_amount_; }

	void setBuiltInName(string_view name) { built_in_name_ = name; }
	void setDesaturationAmount(uint8_t amount) { desat_amount_ = amount; }

	ColRGBA translate(ColRGBA col, Palette* pal = nullptr);

	TransRange* addRange(TransRange::Type type, int pos = -1, int range_start = 0, int range_end = 0);
	void        removeRange(int pos);
	void        swapRanges(int pos1, int pos2);

	static ColRGBA specialBlend(ColRGBA col, uint8_t type, Palette* pal = nullptr);
	static string  getPredefined(string_view def);

private:
	vector<unique_ptr<TransRange>> translations_;
	string                         built_in_name_;
	uint8_t                        desat_amount_ = 0;
};
} // namespace slade

#pragma once

class Translation;

class TransRange
{
	friend class Translation;

public:
	enum Type
	{
		Palette = 1,
		Colour,
		Desat,
		Blend,
		Tint,
		Special
	};

	TransRange(Type type)
	{
		type_    = type;
		o_start_ = 0;
		o_end_   = 0;
	}

	virtual ~TransRange() {}

	uint8_t type() { return type_; }
	uint8_t oStart() { return o_start_; }
	uint8_t oEnd() { return o_end_; }

	void setOStart(uint8_t val) { o_start_ = val; }
	void setOEnd(uint8_t val) { o_end_ = val; }

	virtual string asText() { return ""; }

protected:
	Type    type_;
	uint8_t o_start_;
	uint8_t o_end_;
};

class TransRangePalette : public TransRange
{
	friend class Translation;

public:
	TransRangePalette() : TransRange(Palette) { d_start_ = d_end_ = 0; }
	TransRangePalette(TransRangePalette* copy) : TransRange(Palette)
	{
		o_start_ = copy->o_start_;
		o_end_   = copy->o_end_;
		d_start_ = copy->d_start_;
		d_end_   = copy->d_end_;
	}

	uint8_t dStart() { return d_start_; }
	uint8_t dEnd() { return d_end_; }

	void setDStart(uint8_t val) { d_start_ = val; }
	void setDEnd(uint8_t val) { d_end_ = val; }

	string asText() { return S_FMT("%d:%d=%d:%d", o_start_, o_end_, d_start_, d_end_); }

private:
	uint8_t d_start_;
	uint8_t d_end_;
};

class TransRangeColour : public TransRange
{
	friend class Translation;
public:
	TransRangeColour() : TransRange(Colour)
	{
		d_start_ = COL_BLACK;
		d_end_   = COL_WHITE;
	}
	TransRangeColour(TransRangeColour* copy) : TransRange(Colour)
	{
		o_start_ = copy->o_start_;
		o_end_   = copy->o_end_;
		d_start_.set(copy->d_start_);
		d_end_.set(copy->d_end_);
	}

	rgba_t dStart() { return d_start_; }
	rgba_t dEnd() { return d_end_; }

	void setDStart(rgba_t col) { d_start_.set(col); }
	void setDEnd(rgba_t col) { d_end_.set(col); }

	string asText()
	{
		return S_FMT(
			"%d:%d=[%d,%d,%d]:[%d,%d,%d]",
			o_start_,
			o_end_,
			d_start_.r,
			d_start_.g,
			d_start_.b,
			d_end_.r,
			d_end_.g,
			d_end_.b);
	}

private:
	rgba_t d_start_, d_end_;
};

class TransRangeDesat : public TransRange
{
	friend class Translation;
public:
	TransRangeDesat() : TransRange(Desat)
	{
		d_sr_ = d_sg_ = d_sb_ = 0;
		d_er_ = d_eg_ = d_eb_ = 2;
	}
	TransRangeDesat(TransRangeDesat* copy) : TransRange(Desat)
	{
		o_start_ = copy->o_start_;
		o_end_   = copy->o_end_;
		d_sr_     = copy->d_sr_;
		d_sg_     = copy->d_sg_;
		d_sb_     = copy->d_sb_;
		d_er_     = copy->d_er_;
		d_eg_     = copy->d_eg_;
		d_eb_     = copy->d_eb_;
	}

	float dSr() { return d_sr_; }
	float dSg() { return d_sg_; }
	float dSb() { return d_sb_; }
	float dEr() { return d_er_; }
	float dEg() { return d_eg_; }
	float dEb() { return d_eb_; }

	void setDStart(float r, float g, float b)
	{
		d_sr_ = r;
		d_sg_ = g;
		d_sb_ = b;
	}
	void setDEnd(float r, float g, float b)
	{
		d_er_ = r;
		d_eg_ = g;
		d_eb_ = b;
	}

	string asText()
	{
		return S_FMT(
			"%d:%d=%%[%1.2f,%1.2f,%1.2f]:[%1.2f,%1.2f,%1.2f]", o_start_, o_end_, d_sr_, d_sg_, d_sb_, d_er_, d_eg_, d_eb_);
	}

private:
	float d_sr_, d_sg_, d_sb_;
	float d_er_, d_eg_, d_eb_;
};

class TransRangeBlend : public TransRange
{
	friend class Translation;
public:
	TransRangeBlend() : TransRange(Blend) { col_ = COL_RED; }
	TransRangeBlend(TransRangeBlend* copy) : TransRange(Blend)
	{
		o_start_ = copy->o_start_;
		o_end_   = copy->o_end_;
		col_      = copy->col_;
	}

	rgba_t colour() { return col_; }
	void   setColour(rgba_t c) { col_ = c; }

	string asText() { return S_FMT("%d:%d=#[%d,%d,%d]", o_start_, o_end_, col_.r, col_.g, col_.b); }

private:
	rgba_t col_;
};

class TransRangeTint : public TransRange
{
	friend class Translation;
public:
	TransRangeTint() : TransRange(Tint)
	{
		col_    = COL_RED;
		amount_ = 50;
	}
	TransRangeTint(TransRangeTint* copy) : TransRange(Tint)
	{
		o_start_ = copy->o_start_;
		o_end_   = copy->o_end_;
		col_      = copy->col_;
		amount_   = copy->amount_;
	}

	rgba_t  colour() { return col_; }
	uint8_t amount() { return amount_; }
	void    setColour(rgba_t c) { col_ = c; }
	void    setAmount(uint8_t a) { amount_ = a; }

	string asText() { return S_FMT("%d:%d=@%d[%d,%d,%d]", o_start_, o_end_, amount_, col_.r, col_.g, col_.b); }

private:
	rgba_t  col_;
	uint8_t amount_;
};

class TransRangeSpecial : public TransRange
{
	friend class Translation;
public:
	TransRangeSpecial() : TransRange(Special) { special_ = ""; }
	TransRangeSpecial(TransRangeSpecial* copy) : TransRange(Special)
	{
		o_start_ = copy->o_start_;
		o_end_   = copy->o_end_;
		special_  = copy->special_;
	}

	string special() { return special_; }
	void   setSpecial(string sp) { special_ = sp; }

	string asText() { return S_FMT("%d:%d=$%s", o_start_, o_end_, special_); }

private:
	string special_;
};

class Palette;
class Translation
{
public:
	Translation();
	~Translation();

	void   parse(string def);
	void   parseRange(string range);
	void   read(const uint8_t* data);
	string asText();
	void   clear();
	void   copy(Translation& copy);
	bool   isEmpty() { return built_in_name_.IsEmpty() && translations_.size() == 0; }

	unsigned    nRanges() { return translations_.size(); }
	TransRange* range(unsigned index);
	string      builtInName() { return built_in_name_; }
	void        setDesaturationAmount(uint8_t amount) { desat_amount_ = amount; }

	rgba_t translate(rgba_t col, Palette* pal = nullptr);
	rgba_t specialBlend(rgba_t col, uint8_t type, Palette* pal = nullptr);

	void addRange(int type, int pos);
	void removeRange(int pos);
	void swapRanges(int pos1, int pos2);

	static string getPredefined(string def);

private:
	vector<TransRange*> translations_;
	string              built_in_name_;
	uint8_t             desat_amount_;
};


#ifndef __TRANSLATION_H__
#define __TRANSLATION_H__

#define	TRANS_PALETTE	1
#define TRANS_COLOUR	2
#define TRANS_DESAT		3
#define TRANS_BLEND		4
#define TRANS_TINT		5
#define TRANS_SPECIAL	6

class Translation;

class TransRange
{
	friend class Translation;
protected:
	uint8_t	type;
	uint8_t	o_start;
	uint8_t	o_end;

public:
	TransRange(uint8_t type)
	{
		this->type = type;
		this->o_start = 0;
		this->o_end = 0;
	}

	virtual ~TransRange() {}

	uint8_t getType() { return type; }
	uint8_t	oStart() { return o_start; }
	uint8_t oEnd() { return o_end; }

	void	setOStart(uint8_t val) { o_start = val; }
	void	setOEnd(uint8_t val) { o_end = val; }

	virtual string	asText() { return ""; }
};

class TransRangePalette : public TransRange
{
	friend class Translation;
private:
	uint8_t	d_start;
	uint8_t	d_end;

public:
	TransRangePalette() : TransRange(TRANS_PALETTE) { d_start = d_end = 0; }
	TransRangePalette(TransRangePalette* copy) : TransRange(TRANS_PALETTE)
	{
		o_start = copy->o_start;
		o_end = copy->o_end;
		d_start = copy->d_start;
		d_end = copy->d_end;
	}

	uint8_t	dStart() { return d_start; }
	uint8_t	dEnd() { return d_end; }

	void	setDStart(uint8_t val) { d_start = val; }
	void	setDEnd(uint8_t val) { d_end = val; }

	string	asText() { return S_FMT("%d:%d=%d:%d", o_start, o_end, d_start, d_end); }
};

class TransRangeColour : public TransRange
{
	friend class Translation;
private:
	rgba_t d_start, d_end;

public:
	TransRangeColour() : TransRange(TRANS_COLOUR) { d_start = COL_BLACK; d_end = COL_WHITE; }
	TransRangeColour(TransRangeColour* copy) : TransRange(TRANS_COLOUR)
	{
		o_start = copy->o_start;
		o_end = copy->o_end;
		d_start.set(copy->d_start);
		d_end.set(copy->d_end);
	}

	rgba_t	dStart() { return d_start; }
	rgba_t	dEnd() { return d_end; }

	void	setDStart(rgba_t col) { d_start.set(col); }
	void	setDEnd(rgba_t col) { d_end.set(col); }

	string asText()
	{
		return S_FMT("%d:%d=[%d,%d,%d]:[%d,%d,%d]",
		             o_start, o_end,
		             d_start.r, d_start.g, d_start.b,
		             d_end.r, d_end.g, d_end.b);
	}
};

class TransRangeDesat : public TransRange
{
	friend class Translation;
private:
	float d_sr, d_sg, d_sb;
	float d_er, d_eg, d_eb;

public:
	TransRangeDesat() : TransRange(TRANS_DESAT) { d_sr = d_sg = d_sb = 0; d_er = d_eg = d_eb = 2; }
	TransRangeDesat(TransRangeDesat* copy) : TransRange(TRANS_DESAT)
	{
		o_start = copy->o_start;
		o_end = copy->o_end;
		d_sr = copy->d_sr;
		d_sg = copy->d_sg;
		d_sb = copy->d_sb;
		d_er = copy->d_er;
		d_eg = copy->d_eg;
		d_eb = copy->d_eb;
	}

	float dSr() { return d_sr; }
	float dSg() { return d_sg; }
	float dSb() { return d_sb; }
	float dEr() { return d_er; }
	float dEg() { return d_eg; }
	float dEb() { return d_eb; }

	void	setDStart(float r, float g, float b) { d_sr = r; d_sg = g; d_sb = b; }
	void	setDEnd(float r, float g, float b) { d_er = r; d_eg = g; d_eb = b; }

	string asText()
	{
		return S_FMT("%d:%d=%%[%1.2f,%1.2f,%1.2f]:[%1.2f,%1.2f,%1.2f]",
		             o_start, o_end, d_sr, d_sg, d_sb, d_er, d_eg, d_eb);
	}
};

class TransRangeBlend : public TransRange
{
	friend class Translation;
private:
	rgba_t col;

public:
	TransRangeBlend() : TransRange(TRANS_BLEND) { col = COL_RED; }
	TransRangeBlend(TransRangeBlend* copy) : TransRange(TRANS_BLEND)
	{
		o_start = copy->o_start;
		o_end = copy->o_end;
		col = copy->col;
	}

	rgba_t	getColour() { return col; }
	void	setColour(rgba_t c) { col = c; }

	string asText()
	{
		return S_FMT("%d:%d=#[%d,%d,%d]",
			o_start, o_end, col.r, col.g, col.b);
	}
};

class TransRangeTint : public TransRange
{
	friend class Translation;
private:
	rgba_t	col;
	uint8_t	amount;

public:
	TransRangeTint() : TransRange(TRANS_TINT) { col = COL_RED; amount = 50; }
	TransRangeTint(TransRangeTint* copy) : TransRange(TRANS_TINT)
	{
		o_start = copy->o_start;
		o_end = copy->o_end;
		col = copy->col;
		amount = copy->amount;
	}

	rgba_t	getColour() { return col; }
	uint8_t	getAmount() { return amount; }
	void	setColour(rgba_t c) { col = c; }
	void	setAmount(uint8_t a) { amount = a; }

	string asText()
	{
		return S_FMT("%d:%d=@%d[%d,%d,%d]",
			o_start, o_end, amount, col.r, col.g, col.b);
	}
};

class TransRangeSpecial : public TransRange
{
	friend class Translation;
private:
	string special;

public:
	TransRangeSpecial() : TransRange(TRANS_SPECIAL) { special = ""; }
	TransRangeSpecial(TransRangeSpecial* copy) : TransRange(TRANS_SPECIAL)
	{
		o_start = copy->o_start;
		o_end = copy->o_end;
		special = copy->special;
	}

	string	getSpecial() { return special; }
	void	setSpecial(string sp) { special = sp; }

	string asText()
	{
		return S_FMT("%d:%d=$%s", o_start, o_end, special);
	}
};

class Palette8bit;
class Translation
{
private:
	vector<TransRange*>	translations;
	string				built_in_name;
	uint8_t				desat_amount;

public:
	Translation();
	~Translation();

	void	parse(string def);
	void	read(const uint8_t * data);
	string	asText();
	void	clear();
	void	copy(Translation& copy);
	bool	isEmpty() { return built_in_name.IsEmpty() && translations.size() == 0; }

	unsigned	nRanges() { return translations.size(); }
	TransRange*	getRange(unsigned index);
	string		builtInName() { return built_in_name; }
	void		setDesaturationAmount(uint8_t amount) { desat_amount = amount; }

	rgba_t	translate(rgba_t col, Palette8bit* pal = NULL);
	rgba_t	specialBlend(rgba_t col, uint8_t type, Palette8bit* pal = NULL);

	void	addRange(int type, int pos);
	void	removeRange(int pos);
	void	swapRanges(int pos1, int pos2);
};

#endif//__TRANSLATION_H__

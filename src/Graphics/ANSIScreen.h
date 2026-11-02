#pragma once

namespace slade::gfx
{
class ANSIScreen
{
public:
	static constexpr unsigned NUMROWS  = 25;
	static constexpr unsigned NUMCOLS  = 80;
	static constexpr unsigned SIZE     = NUMROWS * NUMCOLS;
	static constexpr unsigned DATASIZE = SIZE * 2;

	using Selection = std::array<bool, SIZE>;

	struct Signals
	{
		sigslot::signal<unsigned>                char_changed;
		sigslot::signal<const vector<unsigned>&> chars_changed;
		sigslot::signal<>                        selection_changed;
	};

	ANSIScreen()  = default;
	~ANSIScreen() = default;

	const MemChunk&  data() const { return data_; }
	const Selection& selection() const { return selection_; }
	Signals&         signals() { return signals_; }

	bool open(const MemChunk& mc);

	u8 colourAt(unsigned index) const;
	u8 colourAt(u8 x, u8 y) const;

	u8 foregroundAt(unsigned index) const;
	u8 foregroundAt(u8 x, u8 y) const;

	u8 backgroundAt(unsigned index) const;
	u8 backgroundAt(u8 x, u8 y) const;

	u8 characterAt(unsigned index) const;
	u8 characterAt(u8 x, u8 y) const;

	void setForeground(unsigned index, u8 fg);
	void setForeground(u8 x, u8 y, u8 fg);
	void setSelectionForeground(u8 fg);

	void setBackground(unsigned index, u8 bg);
	void setBackground(u8 x, u8 y, u8 bg);
	void setSelectionBackground(u8 bg);

	void setCharacter(unsigned index, u8 ch);
	void setCharacter(u8 x, u8 y, u8 ch);
	void setSelectionCharacter(u8 ch);

	bool     isSelected(unsigned index) const;
	bool     isSelected(u8 x, u8 y) const;
	bool     hasSelection() const;
	unsigned selectionCount() const;
	unsigned firstSelectedIndex() const;

	void select(unsigned index, bool set = true);
	void select(u8 x, u8 y, bool set = true);
	void select(const vector<unsigned>& indices, bool set = true);

	void toggleSelection(unsigned index);
	void toggleSelection(u8 x, u8 y);
	void toggleSelection(const vector<unsigned>& indices);

	void moveSelection(i8 x_offset, i8 y_offset);

	void clearSelection() { selection_.fill(false); }
	void selectAll() { selection_.fill(true); }

private:
	MemChunk  data_;
	Selection selection_;
	Signals   signals_;
};
} // namespace slade::gfx

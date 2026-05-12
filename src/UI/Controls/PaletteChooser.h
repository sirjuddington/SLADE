#pragma once

namespace slade
{
class PaletteChooser : public wxChoice
{
public:
	PaletteChooser(wxWindow* parent, int id);
	~PaletteChooser() override;

	void     setGlobalFromArchive(Archive* archive, int lump = 0);
	Palette* selectedPalette(const ArchiveEntry* entry = nullptr) const;
	bool     globalSelected() const;
	void     selectPalette(const string& name);
	void     onPaletteChanged(wxCommandEvent& e);
	void     addPalette(const string& name);

	// Signals
	struct Signals
	{
		sigslot::signal<> palette_changed;
	};
	Signals& signals() { return signals_; }

private:
	unique_ptr<Palette> pal_global_;
	Signals             signals_;
};
} // namespace slade

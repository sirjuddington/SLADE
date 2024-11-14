
#include "Main.h"
#include "RadioButtonPanel.h"
#include "UI/Layout.h"

using namespace slade;
using namespace ui;


RadioButtonPanel::RadioButtonPanel(
	wxWindow*               parent,
	const vector<wxString>& choices,
	const wxString&         label,
	int                     selected,
	int                     orientation) :
	wxPanel(parent)
{
	auto lh    = LayoutHelper(this);
	auto sizer = new wxBoxSizer(orientation);

	// Check if we need to add a label
	if (!label.empty())
	{
		auto text       = new wxStaticText(this, wxID_ANY, label);
		auto main_sizer = new wxBoxSizer(wxVERTICAL);
		main_sizer->Add(text, lh.sfWithSmallBorder(0, wxBOTTOM));
		main_sizer->Add(sizer, lh.sfWithXLargeBorder(1, wxLEFT).Expand());
		SetSizer(main_sizer);
	}
	else
		SetSizer(sizer);

	// Create radio buttons
	for (size_t i = 0; i < choices.size(); i++)
	{
		if (i > 0)
			sizer->AddSpacer(lh.pad());

		auto rb = new wxRadioButton(this, wxID_ANY, choices[i]);
		sizer->Add(rb, wxSizerFlags());
		radio_buttons_.push_back(rb);

		if (i == selected)
			rb->SetValue(true);
	}
}

int RadioButtonPanel::getSelection() const
{
	for (size_t i = 0; i < radio_buttons_.size(); i++)
		if (radio_buttons_[i]->GetValue())
			return i;

	return -1;
}

void RadioButtonPanel::setSelection(int index) const
{
	if (index >= 0 && index < static_cast<int>(radio_buttons_.size()))
		radio_buttons_[index]->SetValue(true);
}


// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ZoomControl.cpp
// Description: A simple control for zooming, with +/- buttons to zoom in/out
//              and a combobox to select or enter a zoom level (percent).
//              Can also be linked to a GfxCanvas or CTextureCanvas
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ZoomControl.h"
#include "General/UI.h"
#include "UI/Canvas/CTextureCanvas.h"
#include "UI/Canvas/GfxCanvas.h"
#include "UI/SToolBar/SToolBarButton.h"
#include <array>

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, zoom_gfx, 100, CVar::Save)
CVAR(Int, zoom_ctex, 100, CVar::Save)
namespace slade::ui
{
std::array<int, 8>  zoom_percents      = { 25, 50, 75, 100, 150, 200, 400, 800 };
std::array<int, 23> zoom_percents_fine = { 25,  50,  75,  100,  125,  150,  200,  250,  300,  400,  500, 600,
										   700, 800, 900, 1000, 1250, 1500, 2000, 2500, 3000, 4000, 5000 };
} // namespace slade::ui


// -----------------------------------------------------------------------------
//
// ZoomControl Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ZoomControl class constructor
// -----------------------------------------------------------------------------
ZoomControl::ZoomControl(wxWindow* parent) : wxPanel(parent, -1)
{
	setup();
}

// -----------------------------------------------------------------------------
// ZoomControl class constructor (linking GfxCanvas)
// -----------------------------------------------------------------------------
ZoomControl::ZoomControl(wxWindow* parent, GfxCanvas* linked_canvas) :
	wxPanel(parent, -1), linked_gfx_canvas_{ linked_canvas }, zoom_(zoom_gfx)
{
	linked_canvas->linkZoomControl(this);
	linked_canvas->setScale(zoomScale());
	setup();
}

// -----------------------------------------------------------------------------
// ZoomControl class constructor (linking CTextureCanvas)
// -----------------------------------------------------------------------------
ZoomControl::ZoomControl(wxWindow* parent, CTextureCanvas* linked_canvas) :
	wxPanel(parent, -1), linked_texture_canvas_{ linked_canvas }, zoom_(zoom_ctex)
{
	linked_canvas->linkZoomControl(this);
	linked_canvas->setScale(zoomScale());
	setup();
}

// -----------------------------------------------------------------------------
// Sets the zoom level to [percent]%
// -----------------------------------------------------------------------------
void ZoomControl::setZoomPercent(int percent)
{
	zoom_ = percent;
	cb_zoom_->SetValue(fmt::format("{}%", zoom_));
	updateZoomButtons();

	// Zoom gfx/texture canvas and update
	if (linked_gfx_canvas_)
	{
		linked_gfx_canvas_->setScale(zoomScale());
		linked_gfx_canvas_->Refresh();
		zoom_gfx = zoom_;
	}
	if (linked_texture_canvas_)
	{
		linked_texture_canvas_->setScale(zoomScale());
		linked_texture_canvas_->redraw(false);
		zoom_ctex = zoom_;
	}
}

// -----------------------------------------------------------------------------
// Sets the zoom level to a scale value (1.0 = 100%, 0.5 = 50%, etc.)
// -----------------------------------------------------------------------------
void ZoomControl::setZoomScale(double scale)
{
	setZoomPercent(static_cast<int>(scale * 100.));
}

// -----------------------------------------------------------------------------
// Zooms out to the next smaller zoom preset
// -----------------------------------------------------------------------------
void ZoomControl::zoomOut(bool fine)
{
	if (fine)
	{
		for (int i = zoom_percents_fine.size() - 1; i >= 0; --i)
			if (zoom_percents_fine[i] < zoom_)
			{
				setZoomPercent(zoom_percents_fine[i]);
				return;
			}
	}
	else
	{
		for (int i = zoom_percents.size() - 1; i >= 0; --i)
			if (zoom_percents[i] < zoom_)
			{
				setZoomPercent(zoom_percents[i]);
				return;
			}
	}
}

// -----------------------------------------------------------------------------
// Zooms in to the next larger zoom preset
// -----------------------------------------------------------------------------
void ZoomControl::zoomIn(bool fine)
{
	if (fine)
	{
		for (const auto& pct : zoom_percents_fine)
			if (pct > zoom_)
			{
				setZoomPercent(pct);
				return;
			}
	}
	else
	{
		for (const auto& pct : zoom_percents)
			if (pct > zoom_)
			{
				setZoomPercent(pct);
				return;
			}
	}
}

// -----------------------------------------------------------------------------
// Sets up the control
// -----------------------------------------------------------------------------
void ZoomControl::setup()
{
	// Dropdown values
	wxArrayString values;
	for (const auto& pct : zoom_percents)
		values.Add(fmt::format("{}%", pct));

	// Combobox size
#ifdef WIN32
	wxSize cbsize(ui::scalePx(64), -1);
#else
	auto cbsize = wxDefaultSize;
#endif

	// Create controls
	cb_zoom_ = new wxComboBox(
		this, -1, fmt::format("{}%", zoom_), wxDefaultPosition, cbsize, values, wxTE_PROCESS_ENTER);
	btn_zoom_out_ = new SToolBarButton(this, "zoom_out", "Zoom Out", "zoom_out", "Zoom Out", false, 16);
	btn_zoom_in_  = new SToolBarButton(this, "zoom_in", "Zoom In", "zoom_in", "Zoom In", false, 16);

#ifdef __WXGTK__
	// wxWidgets doesn't leave space for the dropdown arrow in gtk3 for whatever reason
	cbsize = cb_zoom_->GetBestSize();
	cbsize.x += ui::scalePx(20);
	cb_zoom_->SetInitialSize(cbsize);
#endif

	// Layout
	auto* hbox = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(hbox);
	hbox->Add(new wxStaticText(this, -1, "Zoom:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, ui::px(ui::Size::PadMinimum));
	hbox->Add(btn_zoom_out_, 0, wxALIGN_CENTER_VERTICAL);
	hbox->Add(cb_zoom_, 1, wxEXPAND);
	hbox->Add(btn_zoom_in_, 0, wxALIGN_CENTER_VERTICAL);

	// --- Events ---

	// Zoom level selected in dropdown
	cb_zoom_->Bind(
		wxEVT_COMBOBOX,
		[this](wxCommandEvent& e)
		{
			auto val = cb_zoom_->GetValue();
			val.RemoveLast(1); // Remove %
			long val_percent;
			val.ToLong(&val_percent);
			setZoomPercent(val_percent);
		});

	// Zoom level text entered
	cb_zoom_->Bind(
		wxEVT_TEXT_ENTER,
		[this](wxCommandEvent& e)
		{
			auto val = e.GetString();
			if (val.EndsWith("%"))
				val.RemoveLast(1); // Remove % if entered
			long val_percent;
			if (val.ToLong(&val_percent))
				setZoomPercent(val_percent);
			else
				setZoomPercent(zoom_);
		});

	// Zoom in/out button clicked
	Bind(
		wxEVT_STOOLBAR_BUTTON_CLICKED,
		[this](wxCommandEvent& e)
		{
			if (e.GetString() == "zoom_in")
				zoomIn();
			else if (e.GetString() == "zoom_out")
				zoomOut();
		});
}

// -----------------------------------------------------------------------------
// Updates the zoom in/out buttons depending on the current zoom level
// -----------------------------------------------------------------------------
void ZoomControl::updateZoomButtons() const
{
	btn_zoom_out_->Enable(zoom_ > zoom_percents[0]);
	btn_zoom_in_->Enable(zoom_ < zoom_percents[zoom_percents.size() - 1]);
}

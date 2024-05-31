
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SZoomSlider.cpp
// Description: A simple slider control for zooming, shows the selected zoom
//              amount as a % and can be linked to a GfxGLCanvas
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
#include "SZoomSlider.h"
#include "UI/Canvas/GL/CTextureGLCanvas.h"
#include "UI/Canvas/GfxCanvasBase.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// SZoomSlider Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SZoomSlider class constructor (linking GfxGLCanvas)
// -----------------------------------------------------------------------------
SZoomSlider::SZoomSlider(wxWindow* parent, GfxCanvasBase* linked_canvas) :
	wxPanel{ parent },
	linked_gfx_canvas_{ linked_canvas }
{
	setup();
}

// -----------------------------------------------------------------------------
// SZoomSlider class constructor (linking CTextureGLCanvas)
// -----------------------------------------------------------------------------
SZoomSlider::SZoomSlider(wxWindow* parent, CTextureGLCanvas* linked_canvas) :
	wxPanel{ parent },
	linked_texture_canvas_{ linked_canvas }
{
	setup();
}

// -----------------------------------------------------------------------------
// Initializes the control
// -----------------------------------------------------------------------------
void SZoomSlider::setup()
{
	// Create controls
	slider_zoom_ = new wxSlider(this, -1, 100, 20, 800, wxDefaultPosition, FromDIP(wxSize(150, -1)));
	slider_zoom_->SetLineSize(10);
	slider_zoom_->SetPageSize(100);
	label_zoom_amount_ = new wxStaticText(this, -1, "100%");

	// Layout
	SetSizer(new wxBoxSizer(wxHORIZONTAL));
	GetSizer()->Add(wxutil::createLabelHBox(this, "Zoom:", slider_zoom_), wxutil::sfWithBorder(1, wxRIGHT).Expand());
	GetSizer()->Add(label_zoom_amount_, wxSizerFlags().CenterVertical());

	// Slider change event
	slider_zoom_->Bind(
		wxEVT_SLIDER,
		[&](wxCommandEvent&)
		{
			// Update zoom label
			label_zoom_amount_->SetLabel(wxString::Format("%d%%", zoomPercent()));

			// Zoom gfx/texture canvas and update
			if (linked_gfx_canvas_)
			{
				linked_gfx_canvas_->setScale(zoomFactor());
				linked_gfx_canvas_->window()->Refresh();
			}
			if (linked_texture_canvas_)
			{
				linked_texture_canvas_->setScale(zoomFactor());
				linked_texture_canvas_->redraw(false);
			}
		});
}

// -----------------------------------------------------------------------------
// Returns the current zoom level as a percentage
// -----------------------------------------------------------------------------
int SZoomSlider::zoomPercent() const
{
	int zoom_percent = slider_zoom_->GetValue();

	// Lock to 10% increments
	int remainder = zoom_percent % 10;
	zoom_percent -= remainder;

	return zoom_percent;
}

// -----------------------------------------------------------------------------
// Sets the zoom level to [percent]
// -----------------------------------------------------------------------------
void SZoomSlider::setZoom(int percent) const
{
	// Lock to 10% increments
	int remainder = percent % 10;
	percent -= remainder;

	slider_zoom_->SetValue(percent);
}

// -----------------------------------------------------------------------------
// Sets the zoom level to [factor]
// -----------------------------------------------------------------------------
void SZoomSlider::setZoom(double factor) const
{
	setZoom(static_cast<int>(factor * 100));
}

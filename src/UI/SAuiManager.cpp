
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SAuiManager.cpp
// Description: A specialization of wxAuiManager that adds some extra
//              functionality and tweaks for SLADE
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
#include "SAuiManager.h"
#include "SAuiTabArt.h"
#include "STopWindow.h"
#include "UI.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// SAuiManager Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SAuiManager class constructor
// -----------------------------------------------------------------------------
SAuiManager::SAuiManager(STopWindow* parent) : wxAuiManager(parent), parent_(parent)
{
	SetArtProvider(new SAuiDockArt(parent));
	SetFlags(wxAUI_MGR_DEFAULT | wxAUI_MGR_ALLOW_ACTIVE_PANE | wxAUI_MGR_TRANSPARENT_DRAG);

	// Need to do this otherwise docked panes don't remember their size when
	// closed and reopened
	Bind(
		wxEVT_AUI_PANE_CLOSE,
		[&](wxAuiManagerEvent& e)
		{
			auto pane = e.GetPane();
			if (pane && pane->IsOk() && pane->IsDocked())
				pane->BestSize(pane->rect.GetSize());
			e.Skip();
		});
}

// -----------------------------------------------------------------------------
// Shows or hides the pane [id].
// If [focus] is true, the pane will be focused (when shown)
// -----------------------------------------------------------------------------
void SAuiManager::showPane(string_view id, bool show, bool focus)
{
	showPane(GetPane(wxString::FromUTF8(id)), show, focus);
}

// -----------------------------------------------------------------------------
// Toggles the visibility of the pane [id].
// If [focus_on_show] is true, the pane will be focused when shown.
// Returns true if the pane is now visible, false if it is hidden
// -----------------------------------------------------------------------------
bool SAuiManager::togglePane(string_view id, bool focus_on_show)
{
	auto& p_inf = GetPane(wxString::FromUTF8(id));
	showPane(p_inf, !p_inf.IsShown(), focus_on_show);
	return p_inf.IsShown();
}

// -----------------------------------------------------------------------------
// Loads the saved layout for the parent window
// -----------------------------------------------------------------------------
void SAuiManager::loadLayout()
{
	auto layout = getWindowLayout(parent_->id().c_str());

	for (const auto& component : layout)
		if (!component.first.empty() && !component.second.empty())
		{
			auto& pane = GetPane(wxString::FromUTF8(component.first));
			LoadPaneInfo(wxString::FromUTF8(component.second), pane);

			// Set the dock size to the saved best size, if it was set
			const auto size = pane.dock_direction == wxAUI_DOCK_LEFT || pane.dock_direction == wxAUI_DOCK_RIGHT
								  ? pane.best_size.x
								  : pane.best_size.y;
			pane.dock_size  = size > 0 ? size : 0;
		}

	Update();
}

// -----------------------------------------------------------------------------
// Saves the current layout of the parent window
// -----------------------------------------------------------------------------
void SAuiManager::saveLayout()
{
	vector<StringPair> layout;

	for (auto pane : GetAllPanes())
		if (pane.IsOk() && !pane.name.empty() && !pane.name.StartsWith(wxS("_")))
			layout.emplace_back(pane.name.utf8_string(), SavePaneInfo(pane).utf8_string());

	setWindowLayout(parent_->id().c_str(), layout);
}

// -----------------------------------------------------------------------------
// Shows or hides the pane [p_inf].
// If [focus] is true, the pane will be focused (when shown)
// -----------------------------------------------------------------------------
void SAuiManager::showPane(wxAuiPaneInfo& p_inf, bool show, bool focus)
{
	// Ensure the pane's size is remembered if it's being closed
	if (p_inf.IsOk() && p_inf.IsDocked() && p_inf.IsShown())
		p_inf.BestSize(p_inf.rect.GetSize());

	p_inf.Show(show);

	// Focus the pane if requested (and it's being shown)
	if (focus && show && p_inf.window)
		p_inf.window->SetFocus();

	Update();
}

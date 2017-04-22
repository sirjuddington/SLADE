
#include "Main.h"
#include "Input.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "MapEditor/UI/ObjectEditPanel.h"
#include "General/UI.h"
#include "General/KeyBind.h"
#include "General/Clipboard.h"
#include "MapEditor/Renderer/Overlays/MCOverlay.h"

using namespace MapEditor;

EXTERN_CVAR(Bool, selection_clear_click)
EXTERN_CVAR(Int, flat_drawtype)
EXTERN_CVAR(Bool, map_show_selection_numbers)
EXTERN_CVAR(Float, render_3d_brightness)
EXTERN_CVAR(Bool, camera_3d_gravity)
EXTERN_CVAR(Int, render_3d_things)
EXTERN_CVAR(Int, render_3d_things_style)
EXTERN_CVAR(Int, render_3d_hilight)
EXTERN_CVAR(Bool, info_overlay_3d)

Input::Input(MapEditContext& context) :
	context_{context},
	panning_{false},
	mouse_state_{MouseState::Normal},
	mouse_pos_{0, 0},
	mouse_down_pos_{-1, -1},
	mouse_drag_{DragType::None},
	shift_down_{false},
	ctrl_down_{false},
	alt_down_{false}
{
}

void Input::mouseMove(int new_x, int new_y)
{
	mouse_pos_ = { new_x, new_y };
	mouse_pos_map_ = context_.renderer().mapPos(mouse_pos_);
}

void Input::mouseDown()
{
	mouse_down_pos_ = mouse_pos_;
	mouse_down_pos_map_ = mouse_pos_map_;
}

void Input::mouseUp()
{
	mouse_down_pos_ = { -1, -1 };
	mouse_down_pos_map_ = { -1, -1 };
}

void Input::mouseWheel(bool up, double amount)
{
	mouse_wheel_speed_ = amount;

	if (up)
	{
		KeyBind::keyPressed(keypress_t("mwheelup", alt_down_, ctrl_down_, shift_down_));

		// Send to overlay if active
		if (context_.overlayActive())
			context_.currentOverlay()->keyDown("mwheelup");

		KeyBind::keyReleased("mwheelup");
	}
	else
	{
		KeyBind::keyPressed(keypress_t("mwheeldown", alt_down_, ctrl_down_, shift_down_));

		// Send to overlay if active
		if (context_.overlayActive())
			context_.currentOverlay()->keyDown("mwheeldown");

		KeyBind::keyReleased("mwheeldown");
	}
}

void Input::updateKeyModifiersWx(int modifiers)
{
	shift_down_ = (modifiers & wxMOD_SHIFT) > 0;
	ctrl_down_ = (modifiers & wxMOD_CONTROL) > 0;
	alt_down_ = (modifiers & wxMOD_ALT) > 0;
}

void Input::onKeyBindPress(string name)
{
	// Check if an overlay is active
	if (context_.overlayActive())
	{
		// Accept edit
		if (name == "map_edit_accept")
		{
			context_.closeCurrentOverlay();
			context_.renderer().renderer3D().enableHilight(true);
			context_.renderer().renderer3D().enableSelection(true);
		}

		// Cancel edit
		else if (name == "map_edit_cancel")
		{
			context_.closeCurrentOverlay(true);
			context_.renderer().renderer3D().enableHilight(true);
			context_.renderer().renderer3D().enableSelection(true);
		}

		// Nothing else
		return;
	}

	// Toggle 3d mode
	if (name == "map_toggle_3d")
	{
		if (context_.editMode() == Mode::Visual)
			context_.setPrevEditMode();
		else
			context_.setEditMode(Mode::Visual);
	}

	// Send to edit context first
	if (mouse_state_ == MouseState::Normal)
	{
		if (context_.handleKeyBind(name, mouse_pos_map_))
			return;
	}

	// Handle keybinds depending on mode
	if (context_.editMode() == Mode::Visual)
		handleKeyBind3d(name);
	else
	{
		handleKeyBind2dView(name);
		handleKeyBind2d(name);
	}
}

void Input::onKeyBindRelease(string name)
{
	if (name == "me2d_pan_view" && context_.input().panning())
	{
		panning_ = false;
		if (mouse_state_ == MouseState::Normal)
			context_.selection().updateHilight(mouse_pos_map_, context_.renderer().viewScale());
		context_.setCursor(UI::MouseCursor::Normal);
	}

	else if (name == "me2d_thing_quick_angle" && mouse_state_ == MouseState::ThingAngle)
	{
		mouse_state_ = MouseState::Normal;
		context_.endUndoRecord(true);
		context_.selection().updateHilight(mouse_pos_map_, context_.renderer().viewScale());
	}
}

void Input::handleKeyBind2dView(const string& name)
{
	// Pan left
	if (name == "me2d_left")
		context_.renderer().pan(-128 / context_.renderer().viewScale(), 0);

	// Pan right
	else if (name == "me2d_right")
		context_.renderer().pan(128 / context_.renderer().viewScale(), 0);

	// Pan up
	else if (name == "me2d_up")
		context_.renderer().pan(0, 128 / context_.renderer().viewScale());

	// Pan down
	else if (name == "me2d_down")
		context_.renderer().pan(0, -128 / context_.renderer().viewScale());

	// Zoom out
	else if (name == "me2d_zoom_out")
		context_.renderer().zoom(0.8);

	// Zoom in
	else if (name == "me2d_zoom_in")
		context_.renderer().zoom(1.25);

	// Zoom out (follow mouse)
	if (name == "me2d_zoom_out_m")
		context_.renderer().zoom(1.0 - (0.2 * mouse_wheel_speed_), true);

	// Zoom in (follow mouse)
	else if (name == "me2d_zoom_in_m")
		context_.renderer().zoom(1.0 + (0.25 * mouse_wheel_speed_), true);

	// Zoom in (show object)
	// TODO: This
	//else if (name == "me2d_show_object")
	//	viewShowObject();

	// Zoom out (full map)
	else if (name == "me2d_show_all")
		context_.renderer().viewFitToMap();

	// Pan view
	else if (name == "me2d_pan_view")
	{
		mouse_down_pos_ = mouse_pos_;
		panning_ = true;
		if (mouse_state_ == MouseState::Normal)
			context_.selection().clearHilight();
		context_.setCursor(UI::MouseCursor::Move);
	}

	// Increment grid
	else if (name == "me2d_grid_inc")
		context_.incrementGrid();

	// Decrement grid
	else if (name == "me2d_grid_dec")
		context_.decrementGrid();
}

void Input::handleKeyBind2d(const string& name)
{
	// --- Line Drawing ---
	if (mouse_state_ == MouseState::LineDraw)
	{
		// Accept line draw
		if (name == "map_edit_accept")
		{
			mouse_state_ = MouseState::Normal;
			context_.lineDraw().end();
			MapEditor::window()->showShapeDrawPanel(false);
		}

		// Cancel line draw
		else if (name == "map_edit_cancel")
		{
			mouse_state_ = MouseState::Normal;
			context_.lineDraw().end(false);
			MapEditor::window()->showShapeDrawPanel(false);
		}
	}

	// --- Paste ---
	else if (mouse_state_ == MouseState::Paste)
	{
		// Accept paste
		if (name == "map_edit_accept")
		{
			mouse_state_ = MouseState::Normal;
			context_.paste(context_.input().mousePosMap());
		}

		// Cancel paste
		else if (name == "map_edit_cancel")
			mouse_state_ = MouseState::Normal;
	}

	// --- Tag edit ---
	else if (mouse_state_ == MouseState::TagSectors)
	{
		// Accept
		if (name == "map_edit_accept")
		{
			mouse_state_ = MouseState::Normal;
			context_.endTagEdit(true);
		}

		// Cancel
		else if (name == "map_edit_cancel")
		{
			mouse_state_ = MouseState::Normal;
			context_.endTagEdit(false);
		}
	}
	else if (mouse_state_ == MouseState::TagThings)
	{
		// Accept
		if (name == "map_edit_accept")
		{
			mouse_state_ = MouseState::Normal;
			context_.endTagEdit(true);
		}

		// Cancel
		else if (name == "map_edit_cancel")
		{
			mouse_state_ = MouseState::Normal;
			context_.endTagEdit(false);
		}
	}

	// --- Moving ---
	else if (mouse_state_ == MouseState::Move)
	{
		// Move toggle
		if (name == "me2d_move")
		{
			context_.endMove();
			mouse_state_ = MouseState::Normal;
			context_.renderer().renderer2D().forceUpdate();
		}

		// Accept move
		else if (name == "map_edit_accept")
		{
			context_.endMove();
			mouse_state_ = MouseState::Normal;
			context_.renderer().renderer2D().forceUpdate();
		}

		// Cancel move
		else if (name == "map_edit_cancel")
		{
			context_.endMove(false);
			mouse_state_ = MouseState::Normal;
			context_.renderer().renderer2D().forceUpdate();
		}
	}

	// --- Object Edit ---
	else if (mouse_state_ == MouseState::ObjectEdit)
	{
		// Accept edit
		if (name == "map_edit_accept")
		{
			context_.objectEdit().end(true);
			mouse_state_ = MouseState::Normal;
			context_.renderer().renderer2D().forceUpdate();
			context_.setCursor(UI::MouseCursor::Normal);
		}

		// Cancel edit
		else if (name == "map_edit_cancel" || name == "me2d_begin_object_edit")
		{
			context_.objectEdit().end(false);
			mouse_state_ = MouseState::Normal;
			context_.renderer().renderer2D().forceUpdate();
			context_.setCursor(UI::MouseCursor::Normal);
		}
	}

	// --- Normal mouse state ---
	else if (mouse_state_ == MouseState::Normal)
	{
		// --- All edit modes ---

		// Vertices mode
		if (name == "me2d_mode_vertices")
			context_.setEditMode(Mode::Vertices);

		// Lines mode
		else if (name == "me2d_mode_lines")
			context_.setEditMode(Mode::Lines);

		// Sectors mode
		else if (name == "me2d_mode_sectors")
			context_.setEditMode(Mode::Sectors);

		// Things mode
		else if (name == "me2d_mode_things")
			context_.setEditMode(Mode::Things);

		// 3d mode
		else if (name == "me2d_mode_3d")
			context_.setEditMode(Mode::Visual);

		// Cycle flat type
		if (name == "me2d_flat_type")
		{
			flat_drawtype = flat_drawtype + 1;
			if (flat_drawtype > 2)
				flat_drawtype = 0;

			// Editor message and toolbar update
			switch (flat_drawtype)
			{
			case 0:
				context_.addEditorMessage("Flats: None");
				SAction::fromId("mapw_flat_none")->setChecked();
				break;
			case 1:
				context_.addEditorMessage("Flats: Untextured");
				SAction::fromId("mapw_flat_untextured")->setChecked();
				break;
			case 2:
				context_.addEditorMessage("Flats: Textured");
				SAction::fromId("mapw_flat_textured")->setChecked();
				break;
			default: break;
			};

			MapEditor::window()->refreshToolBar();
		}

		// Move items (toggle)
		else if (name == "me2d_move")
		{
			if (context_.beginMove(context_.input().mousePosMap()))
			{
				mouse_state_ = MouseState::Move;
				context_.renderer().renderer2D().forceUpdate();
			}
		}

		// Edit items
		else if (name == "me2d_begin_object_edit")
			context_.objectEdit().begin();

		// Split line
		else if (name == "me2d_split_line")
			context_.splitLine(context_.input().mousePosMap().x, context_.input().mousePosMap().y, 16 / context_.renderer().viewScale());

		// Begin line drawing
		else if (name == "me2d_begin_linedraw")
			context_.lineDraw().begin();

		// Begin shape drawing
		else if (name == "me2d_begin_shapedraw")
			context_.lineDraw().begin(true);

		// Create object
		else if (name == "me2d_create_object")
		{
			// If in lines mode, begin line drawing
			if (context_.editMode() == Mode::Lines)
			{
				context_.lineDraw().setState(LineDraw::State::Line);
				mouse_state_ = MouseState::LineDraw;
			}
			else
				context_.createObject(context_.input().mousePosMap().x, context_.input().mousePosMap().y);
		}

		// Delete object
		else if (name == "me2d_delete_object")
			context_.deleteObject();

		// Copy properties
		else if (name == "me2d_copy_properties")
			context_.copyProperties();

		// Paste properties
		else if (name == "me2d_paste_properties")
			context_.pasteProperties();

		// Paste object(s)
		else if (name == "paste")
		{
			// Check if any data is copied
			ClipboardItem* item = nullptr;
			for (unsigned a = 0; a < theClipboard->nItems(); a++)
			{
				if (theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_ARCH ||
					theClipboard->getItem(a)->getType() == CLIPBOARD_MAP_THINGS)
				{
					item = theClipboard->getItem(a);
					break;
				}
			}

			// Begin paste if appropriate data exists
			if (item)
				mouse_state_ = MouseState::Paste;
		}

		// Toggle selection numbers
		else if (name == "me2d_toggle_selection_numbers")
		{
			map_show_selection_numbers = !map_show_selection_numbers;

			if (map_show_selection_numbers)
				context_.addEditorMessage("Selection numbers enabled");
			else
				context_.addEditorMessage("Selection numbers disabled");
		}

		// Mirror
		else if (name == "me2d_mirror_x")
			context_.mirror(true);
		else if (name == "me2d_mirror_y")
			context_.mirror(false);

		// Object Properties
		else if (name == "me2d_object_properties")
			context_.editObjectProperties();


		// --- Lines edit mode ---
		if (context_.editMode() == Mode::Lines)
		{
			// Change line texture
			if (name == "me2d_line_change_texture")
				context_.openLineTextureOverlay();

			// Flip line
			else if (name == "me2d_line_flip")
				context_.flipLines();

			// Flip line (no sides)
			else if (name == "me2d_line_flip_nosides")
				context_.flipLines(false);

			// Edit line tags
			else if (name == "me2d_line_tag_edit")
			{
				if (context_.beginTagEdit() > 0)
				{
					mouse_state_ = MouseState::TagSectors;

					// Setup help text
					string key_accept = KeyBind::getBind("map_edit_accept").keysAsString();
					string key_cancel = KeyBind::getBind("map_edit_cancel").keysAsString();
					context_.setFeatureHelp({
						"Tag Edit",
						S_FMT("%s = Accept", key_accept),
						S_FMT("%s = Cancel", key_cancel),
						"Left Click = Toggle tagged sector"
					});
				}
			}
		}


		// --- Things edit mode ---
		else if (context_.editMode() == Mode::Things)
		{
			// Change type
			if (name == "me2d_thing_change_type")
				context_.changeThingType();

			// Quick angle
			else if (name == "me2d_thing_quick_angle")
			{
				if (mouse_state_ == MouseState::Normal)
				{
					if (context_.selection().hasHilightOrSelection())
						context_.beginUndoRecord("Thing Direction Change", true, false, false);

					mouse_state_ = MouseState::ThingAngle;
				}
			}
		}


		// --- Sectors edit mode ---
		else if (context_.editMode() == Mode::Sectors)
		{
			// Change sector texture
			if (name == "me2d_sector_change_texture")
				context_.changeSectorTexture();
		}
	}
}

void Input::handleKeyBind3d(const string& name)
{
	// Escape from 3D mode
	if (name == "map_edit_cancel")
		context_.setPrevEditMode();

	// Toggle fog
	else if (name == "me3d_toggle_fog")
	{
		bool fog = context_.renderer().renderer3D().fogEnabled();
		context_.renderer().renderer3D().enableFog(!fog);
		if (fog)
			context_.addEditorMessage("Fog disabled");
		else
			context_.addEditorMessage("Fog enabled");
	}

	// Toggle fullbright
	else if (name == "me3d_toggle_fullbright")
	{
		bool fb = context_.renderer().renderer3D().fullbrightEnabled();
		context_.renderer().renderer3D().enableFullbright(!fb);
		if (fb)
			context_.addEditorMessage("Fullbright disabled");
		else
			context_.addEditorMessage("Fullbright enabled");
	}

	// Adjust brightness
	else if (name == "me3d_adjust_brightness")
	{
		render_3d_brightness = render_3d_brightness + 0.1;
		if (render_3d_brightness > 2.0)
		{
			render_3d_brightness = 1.0;
		}
		context_.addEditorMessage(S_FMT("Brightness set to %1.1f", (double)render_3d_brightness));
	}

	// Toggle gravity
	else if (name == "me3d_toggle_gravity")
	{
		camera_3d_gravity = !camera_3d_gravity;
		if (!camera_3d_gravity)
			context_.addEditorMessage("Gravity disabled");
		else
			context_.addEditorMessage("Gravity enabled");
	}

//		// Release mouse cursor
//	else if (name == "me3d_release_mouse")
//		lockMouse(false);

	// Toggle things
	else if (name == "me3d_toggle_things")
	{
		// Change thing display type
		render_3d_things = render_3d_things + 1;
		if (render_3d_things > 2)
			render_3d_things = 0;

		// Editor message
		if (render_3d_things == 0)
			context_.addEditorMessage("Things disabled");
		else if (render_3d_things == 1)
			context_.addEditorMessage("Things enabled: All");
		else
			context_.addEditorMessage("Things enabled: Decorations only");
	}

	// Change thing render style
	else if (name == "me3d_thing_style")
	{
		// Change thing display style
		render_3d_things_style = render_3d_things_style + 1;
		if (render_3d_things_style > 2)
			render_3d_things_style = 0;

		// Editor message
		if (render_3d_things_style == 0)
			context_.addEditorMessage("Thing render style: Sprites only");
		else if (render_3d_things_style == 1)
			context_.addEditorMessage("Thing render style: Sprites + Ground boxes");
		else
			context_.addEditorMessage("Thing render style: Sprites + Full boxes");
	}

	// Toggle hilight
	else if (name == "me3d_toggle_hilight")
	{
		// Change hilight type
		render_3d_hilight = render_3d_hilight + 1;
		if (render_3d_hilight > 2)
			render_3d_hilight = 0;

		// Editor message
		if (render_3d_hilight == 0)
			context_.addEditorMessage("Hilight disabled");
		else if (render_3d_hilight == 1)
			context_.addEditorMessage("Hilight enabled: Outline");
		else if (render_3d_hilight == 2)
			context_.addEditorMessage("Hilight enabled: Solid");
	}

	// Toggle info overlay
	else if (name == "me3d_toggle_info")
		info_overlay_3d = !info_overlay_3d;

	// Quick texture
	else if (name == "me3d_quick_texture")
		context_.openQuickTextureOverlay();

	// Send to map editor
	else
		context_.handleKeyBind(name, mouse_pos_map_);
}

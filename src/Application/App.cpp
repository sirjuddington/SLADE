/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    App.cpp
 * Description: The App namespace, with various general application
 *              related functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "App.h"
#include "General/SAction.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace App
{
	wxStopWatch	timer;
	int			temp_fail_count = 0;

	// Directory paths
	string	dir_data = "";
	string	dir_user = "";
	string	dir_app = "";
	string	dir_res = "";
#ifdef WIN32
	string	dir_separator = "\\";
#else
	string	dir_separator = "/";
#endif
}

CVAR(Int, temp_location, 0, CVAR_SAVE)
CVAR(String, temp_location_custom, "", CVAR_SAVE)


/*******************************************************************
 * APP NAMESPACE FUNCTIONS
 *******************************************************************/

namespace App
{
	/* initDirectories
	 * Checks for and creates necessary application directories. Returns
	 * true if all directories existed and were created successfully if
	 * needed, false otherwise
	 *******************************************************************/
	bool initDirectories()
	{
		// If we're passed in a INSTALL_PREFIX (from CMAKE_INSTALL_PREFIX),
		// use this for the installation prefix
#if defined(__WXGTK__) && defined(INSTALL_PREFIX)
		wxStandardPaths::Get().SetInstallPrefix(INSTALL_PREFIX);
#endif//defined(__UNIX__) && defined(INSTALL_PREFIX)

		// Setup app dir
		dir_app = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();

		// Check for portable install
		if (wxFileExists(path("portable", Dir::Executable)))
		{
			// Setup portable user/data dirs
			dir_data = dir_app;
			dir_res = dir_app;
			dir_user = dir_app + dir_separator + "config";
		}
		else
		{
			// Setup standard user/data dirs
			dir_user = wxStandardPaths::Get().GetUserDataDir();
			dir_data = wxStandardPaths::Get().GetDataDir();
			dir_res = wxStandardPaths::Get().GetResourcesDir();
		}

		// Create user dir if necessary
		if (!wxDirExists(dir_user))
		{
			if (!wxMkdir(dir_user))
			{
				wxMessageBox(S_FMT("Unable to create user directory \"%s\"", dir_user), "Error", wxICON_ERROR);
				return false;
			}
		}

		// Check data dir
		if (!wxDirExists(dir_data))
			dir_data = dir_app;	// Use app dir if data dir doesn't exist

		// Check res dir
		if (!wxDirExists(dir_res))
			dir_res = dir_app;	// Use app dir if res dir doesn't exist

		return true;
	}

	/* SLADEWxApp::initActions
	* Sets up all menu/toolbar actions
	*******************************************************************/
	void initActions()
	{
		SAction::initWxId(26000);

		// MainWindow
		new SAction("main_exit", "E&xit", "exit", "Quit SLADE", "", SAction::Type::Normal, wxID_EXIT);
		new SAction("main_undo", "Undo", "undo", "Undo", "Ctrl+Z");
		new SAction("main_redo", "Redo", "redo", "Redo", "Ctrl+Y");
		new SAction("main_setbra", "Set &Base Resource Archive", "archive", "Set the Base Resource Archive, to act as the program 'IWAD'");
		new SAction("main_preferences", "&Preferences...", "settings", "Setup SLADE options and preferences", "", SAction::Type::Normal, wxID_PREFERENCES);
		new SAction("main_showam", "&Archive Manager", "archive", "Toggle the Archive Manager window", "Ctrl+1");
		new SAction("main_showconsole", "&Console", "console", "Toggle the Console window", "Ctrl+2");
		new SAction("main_showundohistory", "&Undo History", "undo", "Toggle the Undo History window", "Ctrl+3");
		new SAction("main_onlinedocs", "Online &Documentation", "wiki", "View SLADE documentation online");
		new SAction("main_about", "&About", "logo", "Informaton about SLADE", "", SAction::Type::Normal, wxID_ABOUT);
		new SAction("main_updatecheck", "Check for Updates...", "", "Check online for updates");

		// ArchiveManagerPanel
		new SAction("aman_newwad", "New Wad Archive", "newarchive", "Create a new Doom Wad Archive", "Ctrl+Shift+W");
		new SAction("aman_newzip", "New Zip Archive", "newzip", "Create a new Zip Archive", "Ctrl+Shift+Z");
		new SAction("aman_newmap", "New Map", "mapeditor", "Create a new standalone map", "Ctrl+Shift+M");
		new SAction("aman_open", "&Open", "open", "Open an existing Archive", "Ctrl+O");
		new SAction("aman_opendir", "Open &Directory", "opendir", "Open a directory as an Archive");
		new SAction("aman_save", "&Save", "save", "Save the currently open Archive", "Ctrl+S");
		new SAction("aman_saveas", "Save &As", "saveas", "Save the currently open Archive to a new file", "Ctrl+Shift+S");
		new SAction("aman_saveall", "Save All", "saveall", "Save all open Archives");
		new SAction("aman_close", "&Close", "close", "Close the currently open Archive", "Ctrl+W");
		new SAction("aman_closeall", "Close All", "closeall", "Close all open Archives");
		new SAction("aman_recent_open", "Open", "open", "Open the selected Archive(s)");
		new SAction("aman_recent_remove", "Remove", "close", "Remove the selected Archive(s) from the recent list");
		new SAction("aman_bookmark_go", "Go To", "open", "Go to the selected bookmark");
		new SAction("aman_bookmark_remove", "Remove", "close", "Remove the selected bookmark(s) from the list");
		new SAction("aman_save_a", "&Save", "save", "Save the selected Archive", "Ctrl+S");
		new SAction("aman_saveas_a", "Save &As", "saveas", "Save the selected Archive to a new file", "Ctrl+Shift+S");
		new SAction("aman_close_a", "&Close", "close", "Close the selected Archive", "Ctrl+W");
		new SAction("aman_recent", "<insert recent file name>", "", "", "", SAction::Type::Normal, -1, -1, 20);

		// ArchivePanel
		new SAction("arch_newentry", "New Entry", "newentry", "Create a new empty entry");
		new SAction("arch_newpalette", "New PLAYPAL", "palette", "Create a new palette entry");
		new SAction("arch_newanimated", "New ANIMATED", "animation", "Create a new Boom ANIMATED entry");
		new SAction("arch_newswitches", "New SWITCHES", "switch", "Create a new Boom SWITCHES entry");
		new SAction("arch_newdir", "New Directory", "newfolder", "Create a new empty directory");
		new SAction("arch_importfiles", "&Import Files", "importfiles", "Import multiple files into the archive", "kb:el_import_files");
		new SAction("arch_buildarchive", "&Build Archive", "buildarchive", "Build archive from the current directory", "kb:el_build_archive");
		new SAction("arch_texeditor", "&Texture Editor", "texeditor", "Open the texture editor for the current archive");
		new SAction("arch_mapeditor", "&Map Editor", "mapeditor", "Open the map editor");
		new SAction("arch_clean_patches", "Remove Unused &Patches", "", "Remove any unused patches, and their associated entries");
		new SAction("arch_clean_textures", "Remove Unused &Textures", "", "Remove any unused textures");
		new SAction("arch_clean_flats", "Remove Unused &Flats", "", "Remove any unused flats");
		new SAction("arch_check_duplicates", "Check Duplicate Entry Names", "", "Checks the archive for any entries sharing the same name");
		new SAction("arch_check_duplicates2", "Check Duplicate Entry Content", "", "Checks the archive for any entries sharing the same data");
		new SAction("arch_clean_iwaddupes", "Remove Entries Duplicated from IWAD", "", "Remove entries that are exact duplicates of entries from the base resource archive");
		new SAction("arch_replace_maps", "Replace in Maps", "", "Tool to find and replace thing types, specials and textures in all maps");
		new SAction("arch_entry_rename", "Rename", "rename", "Rename the selected entries", "kb:el_rename");
		new SAction("arch_entry_rename_each", "Rename Each", "renameeach", "Rename separately all the selected entries");
		new SAction("arch_entry_delete", "Delete", "delete", "Delete the selected entries");
		new SAction("arch_entry_revert", "Revert", "revert", "Revert any modifications made to the selected entries since the last save");
		new SAction("arch_entry_cut", "Cut", "cut", "Cut the selected entries");
		new SAction("arch_entry_copy", "Copy", "copy", "Copy the selected entries");
		new SAction("arch_entry_paste", "Paste", "paste", "Paste the selected entries");
		new SAction("arch_entry_moveup", "Move Up", "up", "Move the selected entries up", "kb:el_move_up");
		new SAction("arch_entry_movedown", "Move Down", "down", "Move the selected entries down", "kb:el_move_down");
		new SAction("arch_entry_sort", "Sort", "down", "Sort the entries in the list");
		new SAction("arch_entry_import", "Import", "import", "Import a file to the selected entry", "kb:el_import");
		new SAction("arch_entry_export", "Export", "export", "Export the selected entries to files", "kb:el_export");
		new SAction("arch_entry_bookmark", "Bookmark", "bookmark", "Bookmark the current entry");
		new SAction("arch_entry_opentab", "In New Tab", "", "Open selected entries in separate tabs");
		new SAction("arch_entry_crc32", "Compute CRC-32 Checksum", "text", "Compute the CRC-32 checksums of the selected entries");
		new SAction("arch_entry_openext", "", "", "", "", SAction::Type::Normal, -1, -1, 20);
		new SAction("arch_entry_setup_external", "Setup External Editors", "settings", "Open the preferences dialog to set up external editors");
		new SAction("arch_bas_convertb", "Convert to SWANTBLS", "", "Convert any selected SWITCHES and ANIMATED entries to a single SWANTBLS entry");
		new SAction("arch_bas_convertz", "Convert to ANIMDEFS", "", "Convert any selected SWITCHES and ANIMATED entries to a single ANIMDEFS entry");
		new SAction("arch_swan_convert", "Compile to SWITCHES and ANIMATED", "", "Convert SWANTBLS entries into SWITCHES and ANIMATED entries");
		new SAction("arch_texturex_convertzd", "Convert to TEXTURES", "", "Convert any selected TEXTUREx entries to ZDoom TEXTURES format");
		new SAction("arch_texturex_finderrors", "Find Texture Errors", "", "Log to the console any error detected in the TEXTUREx entries");
		new SAction("arch_view_text", "View as Text", "text", "Open the selected entry in the text editor, regardless of type");
		new SAction("arch_view_hex", "View as Hex", "data", "Open the selected entry in the hex editor, regardless of type");
		new SAction("arch_gfx_convert", "Convert to...", "convert", "Open the Gfx Conversion Dialog for any selected gfx entries");
		new SAction("arch_gfx_translate", "Colour Remap...", "remap", "Remap a range of colours in the selected gfx entries to another range (paletted gfx only)");
		new SAction("arch_gfx_colourise", "Colourise", "colourise", "Colourise the selected gfx entries");
		new SAction("arch_gfx_tint", "Tint", "tint", "Tint the selected gfx entries by a colour/amount");
		new SAction("arch_gfx_offsets", "Modify Gfx Offsets", "offset", "Mass-modify the offsets for any selected gfx entries");
		new SAction("arch_gfx_addptable", "Add to Patch Table", "pnames", "Add selected gfx entries to PNAMES");
		new SAction("arch_gfx_addtexturex", "Add to TEXTUREx", "texturex", "Create textures from selected gfx entries and add them to TEXTUREx");
		new SAction("arch_gfx_exportpng", "Export as PNG", "export", "Export selected gfx entries to PNG format files");
		new SAction("arch_gfx_pngopt", "Optimize PNG", "pngopt", "Optimize PNG entries");
		new SAction("arch_audio_convertwd", "Convert WAV to Doom Sound", "convert", "Convert any selected WAV format entries to Doom Sound format");
		new SAction("arch_audio_convertdw", "Convert Doom Sound to WAV", "convert", "Convert any selected Doom Sound format entries to WAV format");
		new SAction("arch_audio_convertmus", "Convert MUS to MIDI", "convert", "Convert any selected MUS format entries to MIDI format");
		new SAction("arch_scripts_compileacs", "Compile ACS", "compile", "Compile any selected text entries to ACS bytecode");
		new SAction("arch_scripts_compilehacs", "Compile ACS (Hexen bytecode)", "compile2", "Compile any selected text entries to Hexen-compatible ACS bytecode");
		new SAction("arch_map_opendb2", "Open Map in Doom Builder 2", "", "Open the selected map in Doom Builder 2");
		new SAction("arch_run", "Run Archive", "run", "Run the current archive", "Ctrl+Shift+R");

		// GfxEntryPanel
		new SAction("pgfx_mirror", "Mirror", "mirror", "Mirror the graphic horizontally");
		new SAction("pgfx_flip", "Flip", "flip", "Flip the graphic vertically");
		new SAction("pgfx_rotate", "Rotate", "rotate", "Rotate the graphic");
		new SAction("pgfx_translate", "Colour Remap", "remap", "Remap a range of colours in the graphic to another range (paletted gfx only)");
		new SAction("pgfx_colourise", "Colourise", "colourise", "Colourise the graphic");
		new SAction("pgfx_tint", "Tint", "tint", "Tint the graphic by a colour/amount");
		new SAction("pgfx_alph", "alPh Chunk", "", "Add/Remove alPh chunk to/from the PNG", "", SAction::Type::Check);
		new SAction("pgfx_trns", "tRNS Chunk", "", "Add/Remove tRNS chunk to/from the PNG", "", SAction::Type::Check);
		new SAction("pgfx_extract", "Extract All", "", "Extract all images in this entry to separate PNGs");
		new SAction("pgfx_crop", "Crop", "settings", "Crop the graphic");
		new SAction("pgfx_convert", "Convert to...", "convert", "Open the Gfx Conversion Dialog for the entry");
		new SAction("pgfx_pngopt", "Optimize PNG", "pngopt", "Optimize PNG entry");

		// ArchiveEntryList
		new SAction("aelt_sizecol", "Size", "", "Show the size column", "", SAction::Type::Check);
		new SAction("aelt_typecol", "Type", "", "Show the type column", "", SAction::Type::Check);
		new SAction("aelt_indexcol", "Index", "", "Show the index column", "", SAction::Type::Check);
		new SAction("aelt_hrules", "Horizontal Rules", "", "Show horizontal rules between entries", "", SAction::Type::Check);
		new SAction("aelt_vrules", "Vertical Rules", "", "Show vertical rules between columns", "", SAction::Type::Check);
		new SAction("aelt_bgcolour", "Colour by Type", "", "Colour item background by entry type", "", SAction::Type::Check);
		new SAction("aelt_bgalt", "Alternating Row Colour", "", "Show alternating row colours", "", SAction::Type::Check);

		// TextureEditorPanel
		new SAction("txed_new", "New Texture", "tex_new", "Create a new, empty texture", "kb:txed_tex_new");
		new SAction("txed_delete", "Delete Texture", "tex_delete", "Deletes the selected texture(s) from the list", "kb:txed_tex_delete");
		new SAction("txed_new_patch", "New Texture from Patch", "tex_newpatch", "Create a new texture from an existing patch", "kb:txed_tex_new_patch");
		new SAction("txed_new_file", "New Texture from File", "tex_newfile", "Create a new texture from an image file", "kb:txed_tex_new_file");
		new SAction("txed_rename", "Rename Texture", "tex_rename", "Rename the selected texture(s)");
		new SAction("txed_rename_each", "Rename Each", "tex_renameeach", "Rename separately all the selected textures");
		new SAction("txed_export", "Export Texture", "tex_export", "Create standalone images from the selected texture(s)");
		new SAction("txed_extract", "Extract Texture", "tex_extract", "Export the selected texture(s) as PNG files");
		new SAction("txed_offsets", "Modify Offsets", "tex_offset", "Mass modify offsets in the selected texture(s)");
		new SAction("txed_up", "Move Up", "up", "Move the selected texture(s) up in the list", "kb:txed_tex_up");
		new SAction("txed_down", "Move Down", "down", "Move the selected texture(s) down in the list", "kb:txed_tex_down");
		new SAction("txed_sort", "Sort", "down", "Sort the textures in the list");
		new SAction("txed_copy", "Copy", "copy", "Copy the selected texture(s)", "Ctrl+C");
		new SAction("txed_cut", "Cut", "cut", "Cut the selected texture(s)", "Ctrl+X");
		new SAction("txed_paste", "Paste", "paste", "Paste the previously copied texture(s)", "Ctrl+V");
		new SAction("txed_patch_add", "Add Patch", "patch_add", "Add a patch to the texture", "kb:txed_patch_add");
		new SAction("txed_patch_remove", "Remove Selected Patch(es)", "patch_remove", "Remove selected patch(es) from the texture", "kb:txed_patch_delete");
		new SAction("txed_patch_replace", "Replace Selected Patch(es)", "patch_replace", "Replace selected patch(es) with a different patch", "kb:txed_patch_replace");
		new SAction("txed_patch_back", "Send Selected Patch(es) Back", "patch_back", "Send selected patch(es) toward the back", "kb:txed_patch_back");
		new SAction("txed_patch_forward", "Bring Selected Patch(es) Forward", "patch_forward", "Bring selected patch(es) toward the front", "kb:txed_patch_forward");
		new SAction("txed_patch_duplicate", "Duplicate Selected Patch(es)", "patch_duplicate", "Duplicate the selected patch(es)", "kb:txed_patch_duplicate");

		// AnimatedEntryPanel
		new SAction("anim_new", "New Animation", "animation_new", "Create a new, dummy animation");
		new SAction("anim_delete", "Delete Animation", "animation_delete", "Deletes the selected animation(s) from the list");
		new SAction("anim_up", "Move Up", "up", "Move the selected animation(s) up in the list");
		new SAction("anim_down", "Move Down", "down", "Move the selected animation(s) down in the list");

		// SwitchesEntryPanel
		new SAction("swch_new", "New Switch", "switch_new", "Create a new, dummy switch");
		new SAction("swch_delete", "Delete Switch", "switch_delete", "Deletes the selected switch(es) from the list");
		new SAction("swch_up", "Move Up", "up", "Move the selected switch(es) up in the list");
		new SAction("swch_down", "Move Down", "down", "Move the selected switch(es) down in the list");

		// PaletteEntryPanel
		new SAction("ppal_addcustom", "Add to Custom Palettes", "plus", "Add the current palette to the custom palettes list");
		new SAction("ppal_test", "Test Palette", "palette_test", "Temporarily add the current palette to the palette chooser");
		new SAction("ppal_exportas", "Export As...", "export", "Export the current palette to a file");
		new SAction("ppal_importfrom", "Import From...", "import", "Import data from a file in the current palette");
		new SAction("ppal_colourise", "Colourise", "palette_colourise", "Colourise the palette");
		new SAction("ppal_tint", "Tint", "palette_tint", "Tint the palette");
		new SAction("ppal_tweak", "Tweak", "palette_tweak", "Tweak the palette");
		new SAction("ppal_invert", "Invert", "palette_invert", "Invert the palette");
		new SAction("ppal_gradient", "Gradient", "palette_gradient", "Add a gradient to the palette");
		new SAction("ppal_moveup", "Pull Ahead", "palette_pull", "Move this palette one rank towards the first");
		new SAction("ppal_movedown", "Push Back", "palette_push", "Move this palette one rank towards the last");
		new SAction("ppal_duplicate", "Duplicate", "palette_duplicate", "Create a copy of this palette at the end");
		new SAction("ppal_remove", "Remove", "palette_delete", "Erase this palette");
		new SAction("ppal_removeothers", "Remove Others", "palette_deleteothers", "Keep only this palette and erase all others");
		new SAction("ppal_report", "Write Report", "text", "Write an info report on this palette");
		new SAction("ppal_generate", "Generate Palettes", "palette", "Generate full range of palettes from the first");
		new SAction("ppal_colormap", "Generate Colormaps", "colormap", "Generate colormap lump from the first palette");

		// MapEntryPanel
		new SAction("pmap_open_text", "Edit Level Script", "text", "Open the map header as text (to edit fragglescript, etc.)");

		// DataEntryPanel
		new SAction("data_add_row", "Add Row", "plus", "Add a new row (after the currently selected row");
		new SAction("data_delete_row", "Delete Row(s)", "close", "Delete the currently selected row(s)");
		new SAction("data_cut_row", "Cut Row(s)", "cut", "Cut the currently selected row(s)", "Ctrl+X");
		new SAction("data_copy_row", "Copy Row(s)", "copy", "Copy the currently selected row(s)", "Ctrl+C");
		new SAction("data_paste_row", "Paste Row(s)", "paste", "Paste at the currently selected row", "Ctrl+V");
		new SAction("data_change_value", "Change Value...", "rename", "Change the value of the selected cell(s)");

		// TextEntryPanel
		new SAction("ptxt_wrap", "Word Wrapping", "", "Toggle word wrapping", "", SAction::Type::Check, -1, -1, 1, "txed_word_wrap");
		new SAction("ptxt_find_replace", "Find+Replace...", "", "Find and (optionally) replace text", "kb:ted_findreplace");
		new SAction("ptxt_fold_foldall", "Fold All", "minus", "Fold all possible code", "kb:ted_fold_foldall");
		new SAction("ptxt_fold_unfoldall", "Unfold All", "plus", "Unfold all folded code", "kb:ted_fold_unfoldall");
		new SAction("ptxt_jump_to_line", "Jump To Line...", "up", "Jump to a specific line number", "kb:ted_jumptoline");

		// Map Editor Window
		new SAction("mapw_save", "&Save Map Changes", "save", "Save any changes to the current map", "Ctrl+S");
		new SAction("mapw_saveas", "Save Map &As...", "saveas", "Save the map to a new wad archive", "Ctrl+Shift+S");
		new SAction("mapw_rename", "&Rename Map", "rename", "Rename the current map");
		new SAction("mapw_convert", "Con&vert Map...", "convert", "Convert the current map to a different format");
		new SAction("mapw_backup", "Restore Backup...", "undo", "Restore a previous backup of the current map");
		new SAction("mapw_undo", "Undo", "undo", "Undo", "Ctrl+Z");
		new SAction("mapw_redo", "Redo", "redo", "Redo", "Ctrl+Y");
		new SAction("mapw_setbra", "Set &Base Resource Archive", "archive", "Set the Base Resource Archive, to act as the program 'IWAD'");
		new SAction("mapw_preferences", "&Preferences...", "settings", "Setup SLADE options and preferences");
		int group_mode = SAction::newGroup();
		new SAction("mapw_mode_vertices", "Vertices Mode", "verts", "Change to vertices editing mode", "kb:me2d_mode_vertices", SAction::Type::Radio, -1, group_mode);
		new SAction("mapw_mode_lines", "Lines Mode", "lines", "Change to lines editing mode", "kb:me2d_mode_lines", SAction::Type::Radio, -1, group_mode);
		new SAction("mapw_mode_sectors", "Sectors Mode", "sectors", "Change to sectors editing mode", "kb:me2d_mode_sectors", SAction::Type::Radio, -1, group_mode);
		new SAction("mapw_mode_things", "Things Mode", "things", "Change to things editing mode", "kb:me2d_mode_things", SAction::Type::Radio, -1, group_mode);
		new SAction("mapw_mode_3d", "3d Mode", "3d", "Change to 3d editing mode", "kb:map_toggle_3d", SAction::Type::Radio, -1, group_mode);
		int group_flat_type = SAction::newGroup();
		new SAction("mapw_flat_none", "Wireframe", "flat_w", "Don't show flats (wireframe)", "", SAction::Type::Radio, -1, group_flat_type);
		new SAction("mapw_flat_untextured", "Untextured", "flat_u", "Show untextured flats", "", SAction::Type::Radio, -1, group_flat_type);
		new SAction("mapw_flat_textured", "Textured", "flat_t", "Show textured flats", "", SAction::Type::Radio, -1, group_flat_type);
		int group_sector_mode = SAction::newGroup();
		new SAction("mapw_sectormode_normal", "Normal (Both)", "sector_both", "Edit sector floors and ceilings", "", SAction::Type::Radio, -1, group_sector_mode);
		new SAction("mapw_sectormode_floor", "Floors", "sector_floor", "Edit sector floors", "", SAction::Type::Radio, -1, group_sector_mode);
		new SAction("mapw_sectormode_ceiling", "Ceilings", "sector_ceiling", "Edit sector ceilings", "", SAction::Type::Radio, -1, group_sector_mode);
		new SAction("mapw_showproperties", "&Item Properties", "properties", "Toggle the Item Properties window", "Ctrl+1");
		new SAction("mapw_showconsole", "&Console", "console", "Toggle the Console window", "Ctrl+2");
		new SAction("mapw_showundohistory", "&Undo History", "undo", "Toggle the Undo History window", "Ctrl+3");
		new SAction("mapw_showchecks", "Map Checks", "tick", "Toggle the Map Checks window", "Ctrl+4");
		new SAction("mapw_showscripteditor", "Script &Editor", "text", "Toggle the Script Editor window", "Ctrl+5");
		new SAction("mapw_run_map", "Run Map", "run", "Run the current map", "Ctrl+Shift+R");
		new SAction("mapw_draw_lines", "Draw Lines", "linedraw", "Begin line drawing", "kb:me2d_begin_linedraw");
		new SAction("mapw_draw_shape", "Draw Shape", "shapedraw", "Begin shape drawing", "kb:me2d_begin_shapedraw");
		new SAction("mapw_edit_objects", "Edit Object(s)", "objectedit", "Edit currently selected object(s)", "kb:me2d_begin_object_edit");
		new SAction("mapw_vertex_create", "Create Vertex Here", "", "Create a new vertex at the cursor position");
		new SAction("mapw_line_changetexture", "Change Texture", "", "Change the currently selected or hilighted line texture(s)", "kb:me2d_line_change_texture");
		new SAction("mapw_line_changespecial", "Change Special", "", "Change the currently selected or hilighted line special");
		new SAction("mapw_line_tagedit", "Edit Tagged", "", "Select sectors/things to tag to this line's special", "kb:me2d_line_tag_edit");
		new SAction("mapw_line_correctsectors", "Correct Sectors", "tick", "Correct line sector references");
		new SAction("mapw_line_flip", "Flip Line", "", "Flip the currently selected of hilighted line(s)", "kb:me2d_line_flip");
		new SAction("mapw_thing_changetype", "Change Type", "", "Change the currently selected or hilighted thing type(s)", "kb:me2d_thing_change_type");
		new SAction("mapw_thing_create", "Create Thing Here", "", "Create a new thing at the cursor position");
		new SAction("mapw_sector_create", "Create Sector Here", "", "Create a sector at the cursor position");
		new SAction("mapw_sector_changetexture", "Change Texture", "", "Change the currently selected or hilighted sector texture(s)", "kb:me2d_sector_change_texture");
		new SAction("mapw_sector_changespecial", "Change Special", "", "Change the currently selected or hilighted sector special(s)");
		new SAction("mapw_sector_join", "Merge Sectors", "", "Join the currently selected sectors together, removing unneeded lines", "kb:me2d_sector_join");
		new SAction("mapw_sector_join_keep", "Join Sectors", "", "Join the currently selected sectors together, keeping all lines", "kb:me2d_sector_join_keep");
		new SAction("mapw_item_properties", "Properties", "properties", "Edit the currently selected item's properties");
		new SAction("mapw_camera_set", "Move 3d Camera Here", "", "Set the current position of the 3d mode camera to the cursor position");
		new SAction("mapw_clear_selection", "Clear Selection", "", "Clear the current selection, if any", "kb:me2d_clear_selection");
		new SAction("mapw_show_fullmap", "Show Full Map", "", "Zooms out so that the full map is visible", "kb:me2d_show_all");
		new SAction("mapw_show_item", "Show Item...", "", "Zoom and scroll to show a map item");
		new SAction("mapw_mirror_y", "Mirror Vertically", "flip", "Mirror the selected objects vertically", "kb:me2d_mirror_y");
		new SAction("mapw_mirror_x", "Mirror Horizontally", "mirror", "Mirror the selected objects horizontally", "kb:me2d_mirror_x");
		new SAction("mapw_run_map_here", "Run Map from Here", "run", "Run the current map, starting at the current cursor position");

		// Script editor
		new SAction("mapw_script_save", "Save", "save", "Save changes to scripts");
		new SAction("mapw_script_compile", "Compile", "compile", "Compile scripts");
		new SAction("mapw_script_jumpto", "Jump To...", "up", "Jump to a specific script/function");
		new SAction("mapw_script_togglelanguage", "Show Language List", "properties", "Show/Hide the language list", "", SAction::Type::Check, -1, -1, 1, "script_show_language_list");
	}
}

bool App::init()
{
	// Init application directories
	if (!initDirectories())
		return false;

	// Init SActions
	initActions();

	return true;
}

long App::runTimer()
{
	return timer.Time();
}

/* App::path
 * Prepends an application-related path to a filename,
 * App::Dir::Data: SLADE application data directory (for SLADE.pk3)
 * App::Dir::User: User configuration and resources directory
 * App::Dir::Executable: Directory of the SLADE executable
 * App::Dir::Temp: Temporary files directory
 *******************************************************************/
string App::path(string filename, Dir dir)
{
	if (dir == Dir::Data)
		return dir_data + dir_separator + filename;
	if (dir == Dir::User)
		return dir_user + dir_separator + filename;
	if (dir == Dir::Executable)
		return dir_app + dir_separator + filename;
	if (dir == Dir::Resources)
		return dir_res + dir_separator + filename;
	if (dir == Dir::Temp)
	{
		// Get temp path
		string dir_temp;
		if (temp_location == 0)
			dir_temp = wxStandardPaths::Get().GetTempDir().Append(dir_separator).Append("SLADE3");
		else if (temp_location == 1)
			dir_temp = dir_app + dir_separator + "temp";
		else
			dir_temp = temp_location_custom;

		// Create folder if necessary
		if (!wxDirExists(dir_temp) && temp_fail_count < 2)
		{
			if (!wxMkdir(dir_temp))
			{
				LOG_MESSAGE(1, "Unable to create temp directory \"%s\"", dir_temp);
				temp_fail_count++;
				return path(filename, dir);
			}
		}

		return dir_temp + dir_separator + filename;
	}

	return filename;
}

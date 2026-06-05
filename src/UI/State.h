#pragma once

namespace slade::ui
{
// UI State setting names
inline constexpr string_view ENTRYLIST_INDEX_VISIBLE       = "EntryListIndexVisible";
inline constexpr string_view ENTRYLIST_INDEX_WIDTH         = "EntryListIndexWidth";
inline constexpr string_view ENTRYLIST_SIZE_VISIBLE        = "EntryListSizeVisible";
inline constexpr string_view ENTRYLIST_SIZE_WIDTH          = "EntryListSizeWidth";
inline constexpr string_view ENTRYLIST_TYPE_VISIBLE        = "EntryListTypeVisible";
inline constexpr string_view ENTRYLIST_TYPE_WIDTH          = "EntryListTypeWidth";
inline constexpr string_view ENTRYLIST_NAME_WIDTH          = "EntryListNameWidth";
inline constexpr string_view ENTRYLIST_NAME_WIDTH_LIST     = "EntryListNameWidthList";
inline constexpr string_view ENTRYLIST_NAME_WIDTH_TREE     = "EntryListNameWidthTree";
inline constexpr string_view ENTRYLIST_VIEW_TYPE           = "EntryListViewType";
inline constexpr string_view ENTRYLIST_SORT_COLUMN         = "EntryListSortColumn";
inline constexpr string_view ENTRYLIST_SORT_DESCENDING     = "EntryListSortDescending";
inline constexpr string_view ARCHIVEPANEL_SPLIT_POS        = "ArchivePanelSplitPos";
inline constexpr string_view ARCHIVEPANEL_SPLIT_POS_LIST   = "ArchivePanelSplitPosList";
inline constexpr string_view ARCHIVEPANEL_SPLIT_POS_TREE   = "ArchivePanelSplitPosTree";
inline constexpr string_view ARCHIVE_LAST_CREATED_FORMAT   = "ArchiveLastCreatedFormat";
inline constexpr string_view COLOURISEDIALOG_LAST_COLOUR   = "ColouriseDialogLastColour";
inline constexpr string_view TINTDIALOG_LAST_COLOUR        = "TintDialogLastColour";
inline constexpr string_view TINTDIALOG_LAST_AMOUNT        = "TintDialogLastAmount";
inline constexpr string_view ZOOM_GFXCANVAS                = "ZoomGfxCanvas";
inline constexpr string_view ZOOM_CTEXTURECANVAS           = "ZoomCTextureCanvas";
inline constexpr string_view BROWSERWINDOW_MAXIMIZED       = "BrowserWindowMaximized";
inline constexpr string_view MAINWINDOW_MAXIMIZED          = "MainWindowMaximized";
inline constexpr string_view MAPEDITORWINDOW_MAXIMIZED     = "MapEditorWindowMaximized";
inline constexpr string_view SCRIPTMANAGERWINDOW_MAXIMIZED = "ScriptManagerWindowMaximized";
inline constexpr string_view SETUP_WIZARD_RUN              = "SetupWizardRun";
inline constexpr string_view RUNDIALOG_LAST_EXE            = "RunDialogLastExe";
inline constexpr string_view RUNDIALOG_LAST_CONFIG         = "RunDialogLastConfig";
inline constexpr string_view RUNDIALOG_LAST_EXTRA          = "RunDialogLastExtra";
inline constexpr string_view RUNDIALOG_START_3D            = "RunDialogStart3d";
inline constexpr string_view MAPDIALOG_LAST_GAME           = "MapDialogLastGame";
inline constexpr string_view MAPDIALOG_LAST_PORT           = "MapDialogLastPort";
inline constexpr string_view FILEDIALOG_LAST_DIR           = "FileDialogLastDir";

void initStateProps();
bool hasSavedState(string_view name, const Archive* archive = nullptr);

bool   getStateBool(string_view name, const Archive* archive = nullptr);
int    getStateInt(string_view name, const Archive* archive = nullptr);
double getStateFloat(string_view name, const Archive* archive = nullptr);
string getStateString(string_view name, const Archive* archive = nullptr);

void saveStateBool(string_view name, bool value, const Archive* archive = nullptr, bool save_global = false);
void saveStateInt(string_view name, int value, const Archive* archive = nullptr, bool save_global = false);
void saveStateFloat(string_view name, double value, const Archive* archive = nullptr, bool save_global = false);
void saveStateString(string_view name, string_view value, const Archive* archive = nullptr, bool save_global = false);
void toggleStateBool(string_view name, const Archive* archive = nullptr);
} // namespace slade::ui

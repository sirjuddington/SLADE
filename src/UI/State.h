#pragma once

namespace slade::ui
{
// UI State setting names
static string ENTRYLIST_INDEX_VISIBLE       = "EntryListIndexVisible";
static string ENTRYLIST_INDEX_WIDTH         = "EntryListIndexWidth";
static string ENTRYLIST_SIZE_VISIBLE        = "EntryListSizeVisible";
static string ENTRYLIST_SIZE_WIDTH          = "EntryListSizeWidth";
static string ENTRYLIST_TYPE_VISIBLE        = "EntryListTypeVisible";
static string ENTRYLIST_TYPE_WIDTH          = "EntryListTypeWidth";
static string ENTRYLIST_NAME_WIDTH          = "EntryListNameWidth";
static string ENTRYLIST_NAME_WIDTH_LIST     = "EntryListNameWidthList";
static string ENTRYLIST_NAME_WIDTH_TREE     = "EntryListNameWidthTree";
static string ENTRYLIST_VIEW_TYPE           = "EntryListViewType";
static string ENTRYLIST_SORT_COLUMN         = "EntryListSortColumn";
static string ENTRYLIST_SORT_DESCENDING     = "EntryListSortDescending";
static string ARCHIVEPANEL_SPLIT_POS        = "ArchivePanelSplitPos";
static string ARCHIVEPANEL_SPLIT_POS_LIST   = "ArchivePanelSplitPosList";
static string ARCHIVEPANEL_SPLIT_POS_TREE   = "ArchivePanelSplitPosTree";
static string ARCHIVE_LAST_CREATED_FORMAT   = "ArchiveLastCreatedFormat";
static string COLOURISEDIALOG_LAST_COLOUR   = "ColouriseDialogLastColour";
static string TINTDIALOG_LAST_COLOUR        = "TintDialogLastColour";
static string TINTDIALOG_LAST_AMOUNT        = "TintDialogLastAmount";
static string ZOOM_GFXCANVAS                = "ZoomGfxCanvas";
static string ZOOM_CTEXTURECANVAS           = "ZoomCTextureCanvas";
static string BROWSERWINDOW_MAXIMIZED       = "BrowserWindowMaximized";
static string MAINWINDOW_MAXIMIZED          = "MainWindowMaximized";
static string MAPEDITORWINDOW_MAXIMIZED     = "MapEditorWindowMaximized";
static string SCRIPTMANAGERWINDOW_MAXIMIZED = "ScriptManagerWindowMaximized";
static string SETUP_WIZARD_RUN              = "SetupWizardRun";
static string RUNDIALOG_LAST_EXE            = "RunDialogLastExe";
static string RUNDIALOG_LAST_CONFIG         = "RunDialogLastConfig";
static string RUNDIALOG_LAST_EXTRA          = "RunDialogLastExtra";
static string RUNDIALOG_START_3D            = "RunDialogStart3d";

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

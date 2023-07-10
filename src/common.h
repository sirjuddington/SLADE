#ifndef COMMON_H
#define COMMON_H

// wxWidgets -------------------------------------------------------------------

#include <wx/aboutdlg.h>
#include <wx/app.h>
#include <wx/arrstr.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>
#include <wx/aui/auibook.h>
#include <wx/aui/dockart.h>
#include <wx/bitmap.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/choicdlg.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/clrpicker.h>
#include <wx/colordlg.h>
#include <wx/colour.h>
#include <wx/combobox.h>
#include <wx/dataview.h>
#include <wx/datetime.h>
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dialog.h>
#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/display.h>
#include <wx/dnd.h>
#include <wx/event.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/fontpicker.h>
#include <wx/gbsizer.h>
#include <wx/generic/dirctrlg.h>
#include <wx/generic/textdlgg.h>
#include <wx/graphics.h>
#include <wx/grid.h>
#include <wx/hashmap.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/ipc.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/minifram.h>
#include <wx/msgdlg.h>
#include <wx/numdlg.h>
#include <wx/panel.h>
#include <wx/popupwin.h>
#include <wx/process.h>
#include <wx/propgrid/property.h>
#include <wx/propgrid/advprops.h>
#include <wx/propgrid/propgrid.h>
#include <wx/protocol/http.h>
#include <wx/ptr_scpd.h>
#include <wx/radiobut.h>
#include <wx/regex.h>
#include <wx/renderer.h>
#include <wx/scrolbar.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/snglinst.h>
#include <wx/spinctrl.h>
#include <wx/sstream.h>
#include <wx/stackwalk.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/stc/stc.h>
#include <wx/stdpaths.h>
#include <wx/stopwatch.h>
#include <wx/string.h>
#include <wx/sysopt.h>
#include <wx/textctrl.h>
#include <wx/textfile.h>
#include <wx/thread.h>
#include <wx/toolbar.h>
#include <wx/treebook.h>
#include <wx/treelist.h>
#include <wx/utils.h>
#include <wx/valnum.h>
#include <wx/wfstream.h>
#include <wx/wrapsizer.h>
#include <wx/xrc/xmlres.h>
#include <wx/zipstrm.h>

#ifndef USE_WEBVIEW_STARTPAGE
#include <wx/html/htmlwin.h>
#endif

#ifdef USE_WEBVIEW_STARTPAGE
#include <wx/webview.h>
#endif

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#endif


// Other Libraries -------------------------------------------------------------

// Fluidsynth
#ifndef NO_FLUIDSYNTH
#include <fluidsynth.h>
#endif

// SFML
#include <SFML/System.hpp>

// Freeimage
#define FREEIMAGE_LIB
#include <FreeImage.h>
#ifndef _WIN32
#undef _WINDOWS_ // Undefine _WINDOWS_ that has been defined by FreeImage
#endif

// fmt
#include <fmt/core.h>

// Sigslot
#include "thirdparty/sigslot/signal.hpp"

// glm
#include <glm/glm.hpp>


// C++ -------------------------------------------------------------------------

#include <map>
#include <vector>
#include <functional>
#include <algorithm>
#include <set>
#include <cmath>
#include <memory>
#include <optional>
#include <unordered_map>

#endif // COMMON_H

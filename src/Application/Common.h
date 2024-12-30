
#ifndef __COMMON_H__
#define __COMMON_H__

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
#include <wx/display.h>
#include <wx/event.h>
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
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/ipc.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/minifram.h>
#include <wx/msgdlg.h>
#include <wx/numdlg.h>
#include <wx/panel.h>
#include <wx/popupwin.h>
#include <wx/process.h>
#include <wx/propgrid/advprops.h>
#include <wx/propgrid/property.h>
#include <wx/propgrid/propgrid.h>
#include <wx/protocol/http.h>
#include <wx/radiobut.h>
#include <wx/regex.h>
#include <wx/renderer.h>
#include <wx/scrolbar.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/snglinst.h>
#include <wx/spinctrl.h>
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
#include <wx/zipstrm.h>

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#endif


// Other Libraries -------------------------------------------------------------

// fmt
#include <fmt/format.h>

// Sigslot
#include "thirdparty/sigslot/signal.hpp"

// GLM
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_CTOR_INIT
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>


// C++ -------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <variant>
#include <vector>

#endif // __COMMON_H__

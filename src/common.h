#ifndef COMMON_H
#define COMMON_H

// wx widgets
#include <wx/string.h>
#include <wx/log.h>
#include <wx/file.h>
#include <wx/app.h>
#include <wx/stopwatch.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/process.h>
#include <wx/button.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/propgrid/propgrid.h>
#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/gbsizer.h>
#include <wx/msgdlg.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/clrpicker.h>
#include <wx/listctrl.h>
#include <wx/treebook.h>
#include <wx/slider.h>
#include <wx/dataview.h>
#include <wx/radiobut.h>
#include <wx/clipbrd.h>
#include <wx/protocol/http.h>
#include <wx/sstream.h>
#include <wx/aui/auibar.h>
#include <wx/toolbar.h>
#include <wx/event.h>
#include <wx/menu.h>
#include <wx/aui/auibook.h>
#include <wx/utils.h>
#include <wx/choicdlg.h>
#include <wx/msgdlg.h>
#include <wx/dnd.h>
#include <wx/aboutdlg.h>
#include <wx/textfile.h>
#include <wx/colour.h>
#include <wx/regex.h>
#include <wx/dcbuffer.h>
#include <wx/image.h>
#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include <wx/renderer.h>
#include <wx/aui/aui.h>
#include <wx/textctrl.h>
#include <wx/settings.h>
#include <wx/numdlg.h>
#include <wx/imaglist.h>
#include <wx/graphics.h>
#include <wx/wrapsizer.h>
#include <wx/display.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/ptr_scpd.h>
#include <wx/dataview.h>
#include <wx/scrolbar.h>
#include <wx/filefn.h>
#include <wx/datetime.h>
#include <wx/treelist.h>
#include <wx/valnum.h>
#include <wx/combobox.h>
#include <wx/dcmemory.h>
#include <wx/ffile.h>
#include <wx/ipc.h>
#include <wx/snglinst.h>
#include <wx/stackwalk.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>
#include <wx/clrpicker.h>
#include <wx/fontpicker.h>
#include <wx/thread.h>
#include <wx/bitmap.h>
#include <wx/popupwin.h>
#include <wx/minifram.h>
#include <wx/aui/dockart.h>
#include <wx/arrstr.h>
#include <wx/hashmap.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/stc/stc.h>
#include <wx/generic/dirctrlg.h>
#include <wx/checklst.h>
#include <wx/grid.h>
#include <wx/generic/textdlgg.h>
#include <wx/colordlg.h>
#include <wx/grid.h>
#include <wx/radiobut.h>
#include <wx/mediactrl.h>
#include <wx/statline.h>

#ifndef USE_WEBVIEW_STARTPAGE
#include <wx/html/htmlwin.h>
#endif

#ifdef USE_WEBVIEW_STARTPAGE
#include <wx/webview.h>
#endif

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#endif

// other libraries
#ifndef NO_FLUIDSYNTH
#include <fluidsynth.h>
#endif

// SFML
#include <SFML/System.hpp>

#ifdef USE_SFML_RENDERWINDOW
#include <SFML/Graphics.hpp>
#include <wx/control.h>
#endif

// Freeimage
#include <FreeImage.h>

// C++
#include <map>
#include <vector>
#include <algorithm>
#include <set>
#include <cmath>
#include <memory>

// C
#include <locale.h>

#endif // COMMON_H


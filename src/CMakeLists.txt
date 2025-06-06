# wxWidgets version minimum 3.0
if (NOT WX_VERSION)
	SET(WX_VERSION 3.0)
endif (NOT WX_VERSION)

# Sources
file(GLOB_RECURSE SLADE_SOURCES CONFIGURE_DEPENDS
	Application/*.cpp
	Archive/*.cpp
	Audio/*.cpp
	Game/*.cpp
	General/*.cpp
	Graphics/*.cpp
	MainEditor/*.cpp
	MapEditor/*.cpp
	OpenGL/*.cpp
	SLADEMap/*.cpp
	TextEditor/*.cpp
	UI/*.cpp
	Utility/*.cpp
	ZReaders/*.cpp
)

# Lua/Scripting sources if not excluded
if (NOT NO_LUA)
	file(GLOB_RECURSE SLADE_SCRIPTING_SOURCES CONFIGURE_DEPENDS Scripting/*.cpp)
	set(SLADE_SOURCES ${SLADE_SOURCES} ${SLADE_SCRIPTING_SOURCES})
else ()
	ADD_DEFINITIONS(-DNO_LUA)
endif ()

# Disable implicit wxString encoding
# (only when building wx 3.3 currently, as it causes build errors in wx 3.2)
if (BUILD_WX)
	ADD_DEFINITIONS(-DwxNO_IMPLICIT_WXSTRING_ENCODING)
endif ()

# Headers
file(GLOB_RECURSE SLADE_HEADERS CONFIGURE_DEPENDS
	*.h
	*.hpp
)

# Setup depending on platform
if (WIN32 AND MSVC)
	include("win_msvc") # Windows+MSVC
else ()
	include("unix") # Linux or MacOS
endif ()

## Build-time requirements

* C++17 compiler (e.g. g++ 8.x or Visual Studio 2019)
* bzip2 library
* cURL library
* FreeImage
* Fluidsynth library
* fmt 6.x
* ftgl, an OpenGL font managing library
* GLEW
* GTK 2.x/3.x (POSIX only)
* Lua
* mpg123 library
* OpenGL
* SFML
* wxWidgets 3.x
* zlib

### cmake

The cmake build offers additional switches:

* `-DNO_COTIRE=ON`: disable the use of precompiled headers
* `-DNO_FLUIDSYNTH=ON`: disable building with Fluidsynth
* `-DNO_LUA=ON`: disable building with Lua, disables scripting engine feature
* `-DNO_WEBVIEW=ON`: use if your wxWidgets build has no wxWebview or if not desired
* `-DWX_GTK3=OFF`: use if your wxWidgets build is using the wxGTK2 backend (there is no autodetection at this point)

### Visual Studio

SLADE can be built on Windows using [Visual Studio](https://visualstudio.microsoft.com/) 2019+ (a free 'community' edition is available which works fine) and [vcpkg](https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=vs-2019) for handling the required external libraries.

Note that you will most likely want to use the `x64-windows-static` triplet when installing vcpkg libraries for the above-mentioned components, e.g.

```
.\vcpkg install <libraries> --triplet x64-windows-static
```

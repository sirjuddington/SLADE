### Mandatory build-time requirements

* C++17 compiler (e.g. g++ 8.x)
* bzip2 library
* cURL library
* FreeImage
* fmt 6.x
* ftgl, an OpenGL font managing library
* GLEW
* GTK 2.x/3.x
* mpg123 library
* OpenGL
* SFML
* wxWidgets 3.x
* zlib

### Optional build-time requirements

* Fluidsynth (deactivate with `cmake -DNO_FLUIDSYNTH=ON`)

### Additional configure switches for cmake

* `-DNO_COTIRE=ON`: disable the use of precompiled headers
* `-DNO_WEBVIEW=ON`: use if your wxWidgets build has no wxWebview or if not desired
* `-DWX_GTK3=OFF`: use if your wxWidgets build is using the wxGTK2 backend (there is no autodetection at this point)

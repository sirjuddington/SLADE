## Linux

### Mandatory build-time requirements

* A compiler supporting C++20 or later
* bzip2
* glm
* mpg123 >= 1.28.1
* OpenGL
* SFML
* wxWidgets >= 3.3.2 (it may build with earlier versions, but that is not recommended due
  to known bugs)
* zlib
* libwebp
* libarchive
* libxmp
* fluidsynth

## Windows

SLADE can be built on Windows using [Visual Studio](https://visualstudio.microsoft.com/) 2022+ (a free 'community' edition is available which works fine) and [vcpkg](https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=vs-2019) for handling the required external libraries.

### Required vcpkg libraries

SLADE provides a vcpkg.json file for 'manifest mode' which should automatically download and install the required libraries via vcpkg when configuring the cmake project

## Linux

### Mandatory build-time requirements

* A compiler supporting C++20 or later
* bzip2
* glm
* mpg123 >= 1.28.1
* OpenGL
* SFML
* wxWidgets >= 3.3.2
* zlib
* libwebp
* libarchive
* libxmp
* fluidsynth

## Windows

SLADE can be built on Windows using [Visual Studio](https://visualstudio.microsoft.com/) 2026+ (a free 'community' edition is available which works fine) and [vcpkg](https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=vs-2019) for handling the required external libraries.

Note: Slade now requires VS2026 due to the use of the Segment Heap and SegmentHeap.cmake.

### Required vcpkg libraries

SLADE provides a vcpkg.json file for 'manifest mode' which should automatically download and install the required libraries via vcpkg when configuring the cmake project

### Compiling

1.  Install Visual Studio 2026.
	- Make sure to install `cmake` and `git`.
2.  Open up an "x64 Native Tools Command Prompt for VS" (a batch file installed as part of VS).
3.  Ensure that `git` is in your path.
	- If installed as part of VS, it can be found in "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd\git.exe").
4.  Change to your SLADE source directory.
5.  Run `cmake --preset win-x64-release -S . -B build` at the command prompt.
6.  Assuming (5) succeeds, run `cmake --build build` at the command prompt.

SLADE should now be built into the `dist` subdirectory.

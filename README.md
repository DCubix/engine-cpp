## EngineCPP
Game engine written in C++

### Building

#### Linux

Building on Linux is pretty straight-forward.

- First, get the SDL2 dependency, the rest of the dependencies are included
as Git submodules so don't worry about them.

##### Arch:
```
$ sudo pacman -S sdl2
```

##### Ubuntu:
```
$ sudo apt-get libsdl2-dev
```

##### Fedora:
```
$ sudo yum install SDL2-devel
```

- Then just run CMake, generate Unix Make Files and just make it.
```bash
$ mkdir build
$ cd build
$ cmake -G "Unix MakeFiles" -DCMAKE_BUILD_TYPE=Debug(or Release) ..
$ ...
$ make -j2
```

#### Mac

TODO! (Need Help)

#### Windows

On Windows, you have to install CMake-GUI to help you out with setting the SDL2
library.

- You have to get it from here: https://www.libsdl.org/download-2.0.php
In `Development Libraries`, pick one for your desired compiler. Extract it somewhere you
think it's easy to find.

- Then open CMake-GUI, set the `Source code` path to the repository root (where the CMakeLists.txt resides)
and set the `build` path to the build folder (create it!).

- Click `Configure` and pick the desired compiler (Visual Studio XX if you picked the VS libraries or
MSYS/MinGW Makefiles if you picked the MinGW libraries) and then click `Finish`.

- An error should appear telling you SDL2 was not found, then set the fields in red
that read `SDL2_LIBRARIES` and `SDL2_INCLUDE_DIRS` to the respective `lib` and `include` folders
you extracted earlier.

- Now click `Configure` again and then `Generate`.

- Build the project with MinGW, MSYS or Visual Studio.
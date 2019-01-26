[![Build status](https://ci.appveyor.com/api/projects/status/ouns27vystoasiwx?svg=true)](https://ci.appveyor.com/project/Milek7/maszyna-8kwj6/branch/master)
# [MaSzyna Train Simulator](https://eu07.pl/?setlang=en)
MaSzyna is a free, polish train simulator. MaSzyna executable source code is licensed under [MPL 2.0 license](https://www.mozilla.org/en-US/MPL/2.0/), and comes with huge pack of free assets, on [custom license](https://eu07.pl/theme/Maszyna/dokumentacja/inne/readme_pliki/en-licence.html).

# Getting Started
## Prerequisites
List of requirements for compiling executable. For usage/runtime requirements [see here](#minimum-requirements).

*In brackets there are oldest tested versions.*

### **1. Software:**
- [CMake](https://cmake.org/) (3.0)
- [make](https://www.gnu.org/software/make/)
- Compiler supporting C++14*. Tested under:
  - Visual Studio (2017)
  - GCC (8.2.1)

  **Note:** MinGW is not supported now. There were some issues with it.
  
  **\* technically, in source code we have some features from C++17, but even older compilers should handle that. You were warned.**

### **2. Dynamic libraries:**

- [GLFW 3.x](https://www.glfw.org/) (3.2.1)

  **Note:** There's some issue in our build system. If link error occurs, use `-DGLFW3_LIBRARIES='<path>'` in CMake.
- [GLM](https://glm.g-truc.net/) (0.9.9.0)
- serialport (0.1.1)
- [sndfile](https://github.com/erikd/libsndfile) (1.0.28)
- [LuajIT](http://luajit.org/) (2.0.5)
- [GLEW](http://glew.sourceforge.net/) (2.1.0)
- [freeglut/GLUT](http://freeglut.sourceforge.net/) (freeglut 3.0.0)
- [PNG](http://www.libpng.org/pub/png/libpng.html) (1.6.34)
- [OpenAL](https://www.openal.org/) (1.18.2)
- pthread
- [Python 2.7](https://www.python.org)

### **3. OpenGL 3.0.**

## Compiling
MaSzyna should work and compile natively under **Linux** and **Windows**. Other platforms are not tested. Full list of requirements is [here](#minimum-requirements) for runtime, and [here](#2-dynamic-libraries) for building.

**Note:** Currently our dev team is too small to fully support compiling process. We do whatever we can, to fix and improve it, but still there may be some issues.

**If you have any problems, feel free to send [issue](https://github.com/eu07/maszyna/issues), or write to us on [our dev chat](https://milek7.pl:8065/eu07/channels/dev).**

**Known issues:**
 - GLFW may have some problems with linking.
 - there may be problems with order of linking x11, and other libs.

Commands will be written in [`Bash`](https://www.gnu.org/software/bash/). No-Linux users must do in corresponding technology.

0. Clone source code.
  You may download source code [as ZIP archive](https://github.com/eu07/maszyna/archive/master.zip), or clone it by [`Git`](https://git-scm.com/). We won't provide tutorial to second one, the only note worth mention is that, repository doesn't contain submodules, so `--recursive` is not needed. From now, it is assumed that your working directory is inside directory with unpacked source code.
1. Make directory, where build files will be stored, then enter inside it.

        $ mkdir build
        $ cd ./build

2. Generate makefile by CMake. Call:
        
        $ cmake ../ [flags]

     where flags may be:

     | Flag                        | Meaning           |
     |-----------------------------|-------------------|
     | -DCMAKE_BUILD_TYPE=Debug    | For debug build   |
     | -DCMAKE_BUILD_TYPE=Release  | For release build |
     | -DGLFW3_LIBRARIES='\<Path>' | To set path to GLFW. If other linker errors are present, it is possible to set other library paths as well, to do that corresponding variable must be fund in CMake files, then passed after `-D`. |

     Other CMake flags may be passed as well.

3. After generation makefile, run make.
        
        make

4. If everything went well, compilation process should be finished.

### Installing
  As we don't have any install script, as `make install`, executable must be copied to install directory "by hand".

  Executable will be in `./bin` directory, named as: `eu07_yymmdd`, or `eu07_yymmdd_d`, where:
  - `yy` is year,
  - `mm` is month,
  - `dd` is day
  - `_d` is debug flag.

  If you currently have MaSzyna assets, just copy executable to install directory.
  Else you must download and unpack assets:

    http://stuff.eu07.pl/MaSzyna-release-latest.zip

  Override every file.

  **Note:** Linux users may have issue, that `Rainsted` (third party starter), may not detect executable. In that case it is possible, to simulate Windows executable, by creation shell script, like this:
  
  **./eu07.exe** (which is technically shell script)

    #!/bin/sh
    ./eu07 $1 $2 $3 $4 $5

  If it still won't work you may try to increase (by comments) file size up to 1MB.
  
### Trailer
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/BqmuKr4MYm8/0.jpg)](https://www.youtube.com/watch?v=BqmuKr4MYm8)
### [Screenshots](https://eu07.pl/galeria/)

![](https://eu07.pl/data/uploads/dodatki2017/e6act_4.png)
![](https://eu07.pl/data/uploads/gallery/2018/cien_2018.png)
[More...](https://eu07.pl/forum/index.php/topic,14630.0.html)
### Minimum Requirements
- OpenGL 1.5+
- 15 GB free disk space
- sound card, support for OpenAL

### [License](LICENSE)

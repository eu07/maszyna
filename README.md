

[![Build status](https://ci.appveyor.com/api/projects/status/ouns27vystoasiwx?svg=true)](https://ci.appveyor.com/project/Milek7/maszyna-8kwj6/branch/master)

# MaSzyna Train Simulator

_A free, **Polish** train simulator._

[Website](https://eu07.pl/?setlang=en) · [Report an issue](https://github.com/eu07/maszyna/issues) · [Dev chat](https://milek7.pl:8065/eu07/channels/dev)

MaSzyna executable source code is licensed under the [MPL 2.0](https://www.mozilla.org/en-US/MPL/2.0/) and comes with a large pack of free assets under a [custom license](https://eu07.pl/theme/Maszyna/dokumentacja/inne/readme_pliki/en-licence.html).


## Table of Contents

-   [Getting Started](#getting-started)
    
    -   [Prerequisites](#prerequisites)
        
    -   [CMake flags](#cmake-flags)
        
    -   [Windows](#windows)
        
    -   [Linux](#linux)
        
-   [Installing](#installing)
    
-   [Known Issues](#known-issues)
    
-   [Support](#support)
    
-   [License](#license)
    


## Getting Started

MaSzyna compiles and runs natively on **Linux** and **Windows**. Other platforms are not tested.

> **Heads-up**  
> Our dev team is small; we keep improving the build process, but issues may still occur. If you get stuck, please ask on the [dev chat](https://milek7.pl:8065/eu07/channels/dev).

### Prerequisites

> For runtime requirements see [Minimum Requirements](#minimum-requirements).

**1) Software** _(oldest tested in parentheses)_

-   [CMake](https://cmake.org/) _(3.0)_
    
-   [make](https://www.gnu.org/software/make/)
    
-   A C++ compiler with C++14 support (we use some C++17 features, but older compilers may still work):
    
    -   **Windows:** Visual Studio 2022
        
    -   **Linux:** GCC _(12.3.1)_
        

> **Note:** MinGW is currently **not supported**.

**2) Libraries** _(oldest tested in parentheses)_

-   [GLFW 3.x](https://www.glfw.org/) _(3.2.1)_  
    If a link error occurs, pass `-DGLFW3_LIBRARIES="<path>"` to CMake.
    
-   [GLM](https://glm.g-truc.net/) _(0.9.9.0)_
    
-   [libserialport](https://sigrok.org/wiki/Libserialport) _(0.1.1)_
    
-   [libsndfile](https://github.com/erikd/libsndfile) _(1.0.28)_
    
-   [LuaJIT](http://luajit.org/) _(2.0.5)_
    
-   [GLEW](http://glew.sourceforge.net/) _(2.1.0)_
    
-   [libpng](http://www.libpng.org/pub/png/libpng.html) _(1.6.34)_
    
-   [OpenAL](https://www.openal.org/) _(1.18.2)_
    
-   **pthread**
    
-   [Python 2.7](https://www.python.org/)
    
-   [Asio](https://think-async.com/Asio) _(1.12)_
    

**3) Graphics API**

-   **OpenGL 3.0** or newer
- **DirectX 12** via NVRHI (optional, Windows only, for Better Renderer)


### CMake flags

| Flag | Meaning |
|------|---------|
| `-DCMAKE_BUILD_TYPE=Debug` | Debug build |
| `-DCMAKE_BUILD_TYPE=Release` | Release build |
| `-DCMAKE_BUILD_TYPE=RelWithDebInfo` | Release build with debug symbols |
| `-DWITH_BETTER_RENDERER=ON/OFF` | Enable/disable NVRHI-based renderer |

> **Linux note:** `WITH_BETTER_RENDERER` uses DirectX 12 through NVRHI and is **not supported on Linux**.




## Windows

```powershell
# Clone repository with submodules
git clone --recursive https://github.com/eu07/maszyna.git

cd maszyna

# Init vcpkg
call setup.bat

# Create directory for CMake build files
mkdir build
cd build

# Generate CMake project (replace %CONFIG% with Debug/Release/RelWithDebInfo)
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%CONFIG%

# Build (replace %CONFIG% accordingly)
cmake --build . --config %CONFIG% --parallel

```



## Linux

```bash
# Install dependencies (Fedora/RHEL family)
sudo dnf install -y \
  @development-tools \
  cmake \
  mesa-libGL-devel \
  glew-devel \
  glfw-devel \
  python2-devel \
  libpng-devel \
  openal-soft-devel \
  luajit-devel \
  libserialport-devel \
  libsndfile-devel \
  gcc \
  g++ \
  wayland-devel \
  wayland-protocols-devel \
  libxkbcommon-devel

# Clone repository with submodules
git clone --recursive https://github.com/eu07/maszyna.git

cd maszyna

# Create directory for CMake build files
mkdir build
cd build

# Generate Makefiles (NVRHI is not supported on Linux)
cmake .. -DWITH_BETTER_RENDERER=OFF

# Compile using all cores
make -j"$(nproc)"

```

> **Tip (Ubuntu/Debian)**: Package names differ; install equivalent `build-essential`, `libgl1-mesa-dev`, `libglew-dev`, `libglfw3-dev`, `python2-dev`, `libpng-dev`, `libopenal-dev`, `libluajit-5.1-dev`, `libserialport-dev`, `libsndfile1-dev`, and Wayland/X11 dev packages as needed.


## Installing

There is no `make install` yet. Copy the built executable to your MaSzyna installation manually.

-   Output directory: `./bin`
    
-   File name format: `eu07_YYYY-MM-DD_<commit>[_d]`
    
    -   `YYYY-MM-DD` – build date
        
    -   `<commit>` – short commit hash
        
    -   `_d` – present in Debug builds
        

If you already have the MaSzyna assets, copy the executable to the install dir. Assets can be downloaded from [eu07.pl](https://eu07.pl/).

**Rainsted on Linux**

If `Rainsted` (a third‑party starter) fails to detect the executable under Linux, you can create a small wrapper script named `eu07.exe`:

```sh
#!/bin/sh
./eu07 "$1" "$2" "$3" "$4" "$5"

```

If detection still fails, padding the file (with comments) up to ~1 MB may help.


## Known Issues

-   **GLFW linking** – On some systems you must provide the GLFW library path explicitly with `-DGLFW3_LIBRARIES`.
    
-   **X11/Wayland linking order** – Linking order of X11 and related libs can matter.
    
-   **NVRHI on Linux** – The NVRHI/"Better Renderer" path targets DirectX 12 and is currently unsupported on Linux.
    


## Support

-   **Issues:** [https://github.com/eu07/maszyna/issues](https://github.com/eu07/maszyna/issues)
    
-   **Developer chat:** [https://milek7.pl:8065/eu07/channels/dev](https://milek7.pl:8065/eu07/channels/dev)
    
-   **Project page:** [https://eu07.pl/](https://eu07.pl/)
    
## Minimum requirements

-   Processor:  **Intel Core i5-7600 or AMD Ryzen 5 1600**
-   RAM:  **16 GB**
-   Graphics Card:  **NVIDIA GeForce GTX 970 or AMD Radeon RX 580**
-   **75 GB of free disk space**
-   Sound card supporting OpenAL with stereo
-   Optional COM ports for custom control panel setups.

## License

-   Source code: [MPL 2.0](https://www.mozilla.org/en-US/MPL/2.0/)
    
-   Assets: [Custom license](https://eu07.pl/theme/Maszyna/dokumentacja/inne/readme_pliki/en-licence.html)

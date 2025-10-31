# Adding yaml-cpp to this project

This file describes several practical ways to add yaml-cpp to the project on Windows/MSYS2 environments and how to update the project's build task to include and link yaml-cpp.

Summary (recommended order):
- Use vcpkg (recommended on Windows) and integrate with your build.
- Use MSYS2 / pacman if you are building inside MSYS2/MinGW environments.
- Build yaml-cpp from source and install somewhere and point the build to it.

1) Using vcpkg (recommended for Windows)

- Install vcpkg (if not already):

  1. Clone vcpkg and bootstrap (one-time):

     git clone https://github.com/microsoft/vcpkg.git
     cd vcpkg
     .\bootstrap-vcpkg.bat

  2. Install yaml-cpp (example x64):

     .\vcpkg.exe install yaml-cpp:x64-windows

  3. Integrate with your environment (optional global integration):

     .\vcpkg.exe integrate install

  Notes for this project (non-CMake build):
  - If you use vcpkg's integration, it can make packages visible to MSVC builds automatically. For our MinGW/g++ workflow you will likely want to point include and lib paths directly from the vcpkg installed tree (e.g. `C:\path\to\vcpkg\installed\x64-windows\include` and `...\lib`).
  - Example g++ flags (adjust path and triplet):

    -I C:\path\to\vcpkg\installed\x64-windows\include
    -L C:\path\to\vcpkg\installed\x64-windows\lib -lyaml-cpp


2) Using MSYS2 / pacman (if you are compiling inside MSYS2)

- If you use MSYS2 (mingw) toolchain, install yaml-cpp via pacman:

  pacman -S mingw-w64-x86_64-yaml-cpp

  Then add these flags to g++:

    -I /mingw64/include
    -L /mingw64/lib -lyaml-cpp

  (paths may differ if you use ucrt64 or i686; adjust accordingly)


3) Build yaml-cpp from source

- Clone, configure and install with CMake:

  git clone https://github.com/jbeder/yaml-cpp.git
  mkdir build && cd build
  cmake -DYAML_BUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=C:/local/yaml-cpp ..
  cmake --build . --config Release --target install

  Then use:

    -I C:/local/yaml-cpp/include
    -L C:/local/yaml-cpp/lib -lyaml-cpp


4) Example: update the project's g++ task

The project's current build invokes g++ with arguments via the workspace filelist; you need two things:

- Add an include path so headers can be found: `-I ${workspaceFolder}\\lib\\yaml-cpp\\include` (or to the folder where yaml-cpp headers live).
- Add a link path and `-lyaml-cpp` after object files: `-L C:\path\to\yamlcpp\lib -lyaml-cpp`.

Example modification of the existing VS Code g++ task (see `.vscode/tasks.json` in this repo for a ready snippet). Replace the placeholder paths with the real install paths.


5) Example compile line (Windows PowerShell style)

g++ -g @filelist.txt -I . -I C:\path\to\yaml-cpp\include -L C:\path\to\yaml-cpp\lib -lyaml-cpp -o smite.exe


6) Using pkg-config (if available)

If your environment provides pkg-config entries for yaml-cpp, you can add the output of `pkg-config --cflags --libs yaml-cpp` to the g++ command.


7) Example runtime caveats

- If you link a shared yaml-cpp (DLL/.dll), ensure the DLL is available on PATH or next to the executable.
- For static linking, you might need to also link transitive dependencies; prefer shared to start.


8) Example test source

- There's a small example in `core/yaml_example.cpp` to validate the integration. Build and run it after installing yaml-cpp.

If you'd like, I can: (a) add vcpkg integration automation, (b) vendor yaml-cpp under `lib/` and add it to compilation, or (c) convert the project to CMake for easier dependency management. Tell me which you prefer and I can implement it.

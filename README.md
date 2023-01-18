# Steps of configuring the project
1. download vulkan sdk from https://vulkan.lunarg.com/sdk/home
2. download llvm from https://github.com/llvm/llvm-project/releases/tag/llvmorg-15.0.6
3. download ninja from https://github.com/ninja-build/ninja/releases
4. download cmake from https://cmake.org/download/
5. download vcpkg according to https://vcpkg.io/en/getting-started.html
6. Modify CMakePresets.json file
- modify "cmakeExecutable" to your cmake.exe folder
- modify "CMAKE_C_COMPILER" to your llvm/bin/clang.exe path
- modify "CMAKE_CXX_COMPILER" to your llvm/bin/clang++.exe path
- modify "CMAKE_MAKE_PROGRAM" to your ninja.exe path
- modify "CMAKE_TOOLCHAIN_FILE" to your vcpkg.cmake path
- append  cmake/bin and llvm/bin path to "PATH" in "environment"
7. Modify "CMAKE_TOOLCHAIN_FILE" in .vscode/setting.json to your vcpkg.cmake path

# Steps of building the project
1. open the project by VSCode
2. install CMake Extension
3. select configure preset to Windows
4. execute cmake configure
5. execute cmake build
6. press F5 to debug the project
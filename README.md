Steps of configuring the project
1. download vulkan sdk from https://vulkan.lunarg.com/sdk/home
2. download llvm from https://github.com/llvm/llvm-project/releases/tag/llvmorg-15.0.6
3. download ninja from https://github.com/ninja-build/ninja/releases
4. download cmake from https://cmake.org/download/
5. download vcpkg according to https://vcpkg.io/en/getting-started.html
6. Modify CMakePresets.json file
    6.1 modify "cmakeExecutable" to your cmake.exe folder
    6.2 modify "CMAKE_C_COMPILER" to your llvm/bin/clang.exe path
    6.3 modify "CMAKE_CXX_COMPILER" to your llvm/bin/clang++.exe path
    6.4 modify "CMAKE_MAKE_PROGRAM" to your ninja.exe path
    6.5 modify "CMAKE_TOOLCHAIN_FILE" to your vcpkg.cmake path
    6.6 append  cmake/bin and llvm/bin path to "PATH" in "environment"
7. Modify "CMAKE_TOOLCHAIN_FILE" in .vscode/setting.json to your vcpkg.cmake path
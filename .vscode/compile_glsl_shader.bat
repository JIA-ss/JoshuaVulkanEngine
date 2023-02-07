@echo off
echo "=== === Compile GLSL Shaders Begin === ==="

set glslPath=%1
set spirvPath=%2

if not exist "%spirvPath%" (
    echo ">> [Creating Dir] %spirvPath%"
    md "%spirvPath%"
)

for %%i in (%glslPath%/./*) do (
    call :CompileShader %%~ni%%~xi %glslPath% %spirvPath%
)

goto End

:CompileShader
set Name=%1
set SrcPath=%2
set TarPath=%3
echo ">> [Compiling Shader] %SrcPath%/%Name% ==> %TarPath%/%Name%.spv"
glslc %SrcPath%/%Name% -o %TarPath%/%Name%.spv
goto :eof
:End

echo "=== === Compile GLSL Shaders End === ==="

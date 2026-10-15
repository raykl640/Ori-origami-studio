@echo off
REM ORI - build on Windows with MinGW-w64 g++ (no CMake / make needed)
REM Usage:  build.bat          builds build\ori.exe and build\ori_tests.exe
REM         build\ori.exe      renders every demo into output\
if not exist build mkdir build
echo Building ori.exe ...
g++ -std=c++17 -O2 -Wall -Wextra -o build\ori.exe app\main.cpp demos\*.cpp src\core\*.cpp src\math\*.cpp src\geometry\*.cpp src\raster\*.cpp src\curves\*.cpp src\shading\*.cpp src\editor\*.cpp src\fold\*.cpp src\layering\*.cpp src\models\*.cpp
if errorlevel 1 goto :fail
echo Building ori_tests.exe ...
g++ -std=c++17 -O2 -Wall -Wextra -o build\ori_tests.exe tests\*.cpp src\core\*.cpp src\math\*.cpp src\geometry\*.cpp src\raster\*.cpp src\curves\*.cpp src\shading\*.cpp src\editor\*.cpp src\fold\*.cpp src\layering\*.cpp src\models\*.cpp
if errorlevel 1 goto :fail
echo.
echo Done. Run:  build\ori_tests.exe   then   build\ori.exe
goto :eof
:fail
echo Build failed.
exit /b 1

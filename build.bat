@echo off
REM ORI - build on Windows with MinGW-w64 g++ (no CMake / make needed)
REM Usage:  build.bat          builds build\ori.exe and build\ori_tests.exe
REM                            (+ build\ori_studio.exe when SDL2 is found)
REM         build\ori.exe      renders every demo into output\
REM For the studio, unpack the SDL2 MinGW development package and set
REM   set SDL2_DIR=C:\path\to\SDL2-2.x\x86_64-w64-mingw32
if not exist build mkdir build
set SRC=src\core\*.cpp src\math\*.cpp src\geometry\*.cpp src\raster\*.cpp src\curves\*.cpp src\shading\*.cpp src\editor\*.cpp src\fold\*.cpp src\layering\*.cpp src\models\*.cpp
echo Building ori.exe ...
g++ -std=c++17 -O2 -Wall -Wextra -o build\ori.exe app\main.cpp demos\*.cpp %SRC%
if errorlevel 1 goto :fail
echo Building ori_tests.exe ...
g++ -std=c++17 -O2 -Wall -Wextra -o build\ori_tests.exe tests\*.cpp demos\*.cpp studio\*.cpp %SRC%
if errorlevel 1 goto :fail
if "%SDL2_DIR%"=="" goto :nostudio
echo Building ori_studio.exe ...
g++ -std=c++17 -O2 -Wall -Wextra -I"%SDL2_DIR%\include\SDL2" -o build\ori_studio.exe studio\sdl\main.cpp demos\*.cpp studio\*.cpp %SRC% -L"%SDL2_DIR%\lib" -lmingw32 -lSDL2
if errorlevel 1 goto :fail
copy /y "%SDL2_DIR%\bin\SDL2.dll" build\ >nul
goto :done
:nostudio
echo SDL2_DIR not set - skipped ori_studio.exe
:done
echo.
echo Done. Run:  build\ori_tests.exe   then   build\ori.exe   (or build\ori_studio.exe)
goto :eof
:fail
echo Build failed.
exit /b 1

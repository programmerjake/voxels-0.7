@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
::echo on
set startdir=%cd%
set include_list=/I external-lib /I include /I external-lib/zlib-1.2.8 /I external-lib/SDL2-2.0.3/include /I external-lib/lpng1617 /I external-lib/libvorbis-1.3.5/include /I external-lib/libogg-1.3.2/include
set library_list=external-lib/libogg-1.3.2/Release/libogg_static.lib
set library_list=%library_list% external-lib/libvorbis-1.3.5/Release/libvorbis_static.lib
set library_list=%library_list% external-lib/libvorbis-1.3.5/Release/libvorbisfile_static.lib
set library_list=%library_list% external-lib/lpng1617/Release/libpng16.lib
set library_list=%library_list% external-lib/zlib-1.2.8/Release/zlib.lib
set library_list=%library_list% external-lib/SDL2-2.0.3/Release/SDL2.lib
set library_list=%library_list% external-lib/SDL2-2.0.3/Release/SDL2main.lib
set object_file_list=
set source_file_list=
set any_errors=false
call :construct_file_list "external-lib/videoInput.cpp"
cd src
for /r %%i in (*.c;*.cpp) do call :construct_file_list "%%i"
cd "%startdir%"
for %%i in (%source_file_list%) do call :compile_file %%i
if "%any_errors%"=="false" (
cl /nologo /O2 /Fevoxels-release.exe %object_file_list% %library_list% /link /SUBSYSTEM:CONSOLE
)
pause
exit /b

:construct_file_list
set source_file_list=%source_file_list% "%~1"
set object_file="%~1-release.obj"
::del %object_file% 2>nul
exit /b

:compile_file
set object_file="%~1-release.obj"
if "%any_errors%"=="false" (
::cl /nologo /c /O2 /Fo%object_file% /Z7 /EHsc %include_list% %1
if errorlevel 1 (
set any_errors=true
)
)
set object_file_list=%object_file_list% %object_file%
exit /b
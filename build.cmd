@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
set startdir=%cd%
set include_list=/I external-lib /I include /I external-lib/zlib-1.2.8 /I external-lib/SDL2-2.0.3/include /I external-lib/lpng1617 /I external-lib/libvorbis-1.3.5/include /I external-lib/libogg-1.3.2/include
set object_file_list=
set source_file_list=
set any_errors=false
call :construct_file_list "external-lib/videoInput.cpp"
cd src
for /r %%i in (*.c;*.cpp) do call :construct_file_list "%%i"
cd "%startdir%"
for %%i in (%source_file_list%) do call :compile_file %%i
if "%any_errors%"=="false" (
echo cl /Fevoxels-release /Z7 /EHsc %include_list% %object_file_list%
rem cl /Fevoxels-release /Z7 /EHsc %include_list% %object_file_list%
)
pause
exit /b

:construct_file_list
set source_file_list=%source_file_list% "%~1"
exit /b

:compile_file
set object_file="%~1-release.obj"
if "%any_errors%"=="false" (
echo cl /c /Fo%object_file% /Z7 /EHsc %include_list% %1
:: cl /c /Fo%object_file% /Z7 /EHsc %include_list% %1
if errorlevel 1 (
set any_errors=true
)
)
set object_file_list=%object_file_list% %object_file%
exit /b
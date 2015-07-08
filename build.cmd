@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
set startdir=%cd%
set file_list=external-lib/videoInput.cpp
cd src
for /r %%i in (*.c;*.cpp) do call :construct_file_list "%%i"
cd "%startdir%"
cl /Fevoxels-release /Gm /Zi /EHsc /I external-lib /I include /I external-lib/zlib-1.2.8 /I external-lib/SDL2-2.0.3/include /I external-lib/lpng1617 /I external-lib/libvorbis-1.3.5/include /I external-lib/libogg-1.3.2/include %file_list%
pause
exit /b

:construct_file_list
set object_file="%~1.obj"
cl /c /Fo%object_file% /Z7 /EHsc /I external-lib /I include /I external-lib/zlib-1.2.8 /I external-lib/SDL2-2.0.3/include /I external-lib/lpng1617 /I external-lib/libvorbis-1.3.5/include /I external-lib/libogg-1.3.2/include %1
set file_list=%file_list% %object_file%
exit /b
@echo off
pushd ..\build

set CompilerFlags=-O2 -MT -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4505 -wd4201 -wd4100 -wd4189 -wd4115 -wd4101 -wd4996 -FAsc
set LinkerFlags=-incremental:no -opt:ref SDL2.lib opengl32.lib shell32.lib comdlg32.lib pathcch.lib shlwapi.lib

cl %CompilerFlags% W:\simply-slide\code\ss.cpp W:\simply-slide\code\dear_imgui\imgui*.cpp /link %LinkerFlags%

popd
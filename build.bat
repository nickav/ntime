@echo off
set pr=%~dp0%
pushd %pr%

if not exist build (mkdir build)
pushd build


cl /nologo ../src/main_win32.c /link /subsystem:console -out:ntime.exe
copy ntime.exe ntime2.exe > 0

ntime2.exe odin build ../src/hello.odin -file -out:ohello.exe
ntime2.exe odin build ../src/main.odin -file -out:otime.exe

ntime2.exe cl /nologo ../src/hello.c /link /subsystem:console -out:chello.exe
ntime2.exe cl /nologo /O2 ../src/main_win32.c /link /subsystem:console -out:ntime.exe


popd

popd
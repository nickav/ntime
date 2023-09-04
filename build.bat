@echo off
set pr=%~dp0%
pushd %pr%

if not exist build (mkdir build)

pushd build

odin build ../src/main.odin -file -out:otime.exe

otime.exe odin build ../src/hello.odin -file -out:ohello.exe
otime.exe cl /nologo ../src/hello.c /link /subsystem:console -out:chello.exe

otime.exe cl /nologo ../src/main.c /link /subsystem:console -out:ntime.exe

ntime.exe cl /nologo ../src/hello.c /link /subsystem:console -out:chello.exe

popd

popd
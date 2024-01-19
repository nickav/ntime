#!/bin/bash
set -e # exit on error

project_root="$(cd "$(dirname "$0")" && pwd -P)"
exe_name="ntime"

mkdir -p $project_root/build

pushd $project_root/build
    clang ../src/main_unix.c -o $exe_name
    ./$exe_name clang ../src/hello_unix.c -o hello
    ./$exe_name ./hello
popd

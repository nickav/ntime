package main

import "core:fmt"
import "core:time"
import "core:os"

import win32 "core:sys/windows"

main :: proc() {
    fmt.println("hello...", os.args);
    time.sleep(200 * time.Millisecond);
    fmt.println("goodbye.");
}
package main

import "core:fmt"
import "core:os"
import "core:strings"
import "core:time"
//import "core:unicode/utf16"

import win32 "core:sys/windows"

foreign import kernel32 "system:Kernel32.lib"

STARTUPINFOA :: struct {
    cb: win32.DWORD,
    lpReserved: win32.LPSTR,
    lpDesktop: win32.LPSTR,
    lpTitle: win32.LPSTR,
    dwX: win32.DWORD,
    dwY: win32.DWORD,
    dwXSize: win32.DWORD,
    dwYSize: win32.DWORD,
    dwXCountChars: win32.DWORD,
    dwYCountChars: win32.DWORD,
    dwFillAttribute: win32.DWORD,
    dwFlags: win32.DWORD,
    wShowWindow: win32.WORD,
    cbReserved2: win32.WORD,
    lpReserved2: win32.LPBYTE,
    hStdInput: win32.HANDLE,
    hStdOutput: win32.HANDLE,
    hStdError: win32.HANDLE,
}

@(default_calling_convention="stdcall")
foreign kernel32 {
    CreateProcessA :: proc(
        lpApplicationName: win32.LPSTR,
        lpCommandLine: win32.LPSTR,
        lpProcessAttributes: win32.LPSECURITY_ATTRIBUTES,
        lpThreadAttributes: win32.LPSECURITY_ATTRIBUTES,
        bInheritHandles: win32.BOOL,
        dwCreationFlags: win32.DWORD,
        lpEnvironment: win32.LPVOID,
        lpCurrentDirectory: win32.LPSTR,
        lpStartupInfo: ^STARTUPINFOA,
        lpProcessInformation: win32.LPPROCESS_INFORMATION,
    ) -> win32.BOOL ---
}

error :: proc(str: string) {
    fmt.println(str);
    os.exit(1);
}

main :: proc() {
    using win32;

    if len(os.args) <= 1 {
        prgm := os.args[0]
        fmt.println(prgm, "<command>");
        fmt.println();
        fmt.println("Example:");
        fmt.println("\t", prgm, "cl main.c");
    }

    sa: SECURITY_ATTRIBUTES;
    sa.nLength = size_of(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nil;

    g_hChildStd_IN_Rd: HANDLE = nil;
    g_hChildStd_IN_Wr: HANDLE = nil;
    g_hChildStd_OUT_Rd: HANDLE = nil;
    g_hChildStd_OUT_Wr: HANDLE = nil;

    // Create a pipe for the child process's STDOUT. 
    if !win32.CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &sa, 0) {
        error("Failed to create pipe.");
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if !win32.SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) {
        error("Stdout SetHandleInformation failed.");
    }

    // Create a pipe for the child process's STDIN. 
    if !win32.CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &sa, 0) {
        error("CreatePipe failed.");
    }

    // Ensure the write handle to the pipe for STDIN is not inherited. 
    if !win32.SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) {
        error("SetHandleInformation failed.");
    }

    // Set up members of the PROCESS_INFORMATION structure. 
    piProcInfo: PROCESS_INFORMATION; 
 
    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.
    siStartInfo: STARTUPINFOA;
    siStartInfo.cb = size_of(STARTUPINFOA); 
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    //
    // TODO(nick): i have no idea how to convert an Odin string to utf16...
    //

    arg_slice := os.args[1:]
    cmd := strings.join(arg_slice, " ")
    szCmdline := strings.clone_to_cstring(cmd)
    
    // Create the child process. 
    bSuccess := CreateProcessA(nil, 
        cast(^u8)szCmdline,     // command line 
        nil,          // process security attributes 
        nil,          // primary thread security attributes 
        win32.TRUE,          // handles are inherited 
        0,             // creation flags 
        nil,          // use parent's environment 
        nil,          // use parent's current directory 
        &siStartInfo,  // STARTUPINFO pointer 
        &piProcInfo);  // receives PROCESS_INFORMATION 

    if !bSuccess {
        error("CreateProcess failed.");
    }

    timer : time.Stopwatch;
    time.stopwatch_start(&timer);

    // Close handles to the stdin and stdout pipes no longer needed by the child process.
    // If they are not explicitly closed, there is no way to recognize that the child process has ended.
    win32.CloseHandle(g_hChildStd_OUT_Wr);
    win32.CloseHandle(g_hChildStd_IN_Rd);

    // Read from pipe that is the standard output for child process. 

    dwRead: DWORD;
    dwWritten: DWORD;
    hParentStdOut := GetStdHandle(STD_OUTPUT_HANDLE);
    chBuf: [4096]CHAR;

    for ;;
    {
        bSuccess: BOOL = FALSE;
        bSuccess = ReadFile(g_hChildStd_OUT_Rd, cast(rawptr)&chBuf, len(chBuf), &dwRead, nil);
        if !bSuccess || dwRead == 0 { break; }

        bSuccess = WriteFile(hParentStdOut, cast(rawptr)&chBuf, dwRead, &dwWritten, nil);
        if !bSuccess { break; }
    }

    time.stopwatch_stop(&timer);
    fmt.println("[time] ", cmd, " (", time.stopwatch_duration(timer), ")", sep = "");
}
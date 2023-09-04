//
// Source: https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
//

//
// NOTE(nick): this literally takes the compile time from 90ms -> 45ms on my machine
//

#if 0
#define WIN32_LEAN_AND_MEAN
#define WIN32_MEAN_AND_LEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h> 
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN
#else

#include "gb_windows.h"

typedef struct _STARTUPINFOA {
    DWORD  cb;
    LPSTR  lpReserved;
    LPSTR  lpDesktop;
    LPSTR  lpTitle;
    DWORD  dwX;
    DWORD  dwY;
    DWORD  dwXSize;
    DWORD  dwYSize;
    DWORD  dwXCountChars;
    DWORD  dwYCountChars;
    DWORD  dwFillAttribute;
    DWORD  dwFlags;
    WORD   wShowWindow;
    WORD   cbReserved2;
    BYTE   *lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD  dwProcessId;
    DWORD  dwThreadId;
} PROCESS_INFORMATION;

#define STARTF_USESTDHANDLES 0x00000100
#define HANDLE_FLAG_INHERIT 0x00000001
#endif

#include <stdio.h> 
#include <assert.h>
#include <stdint.h>

typedef double f64;
typedef uint64_t u64;


#define BUFSIZE 4096 

void error(const char *string) 
{ 
    printf("%s", string);
    fflush(stdout);
    ExitProcess(1);
}

f64 os_time()
{
    static u64 win32_ticks_per_second = 0;
    static u64 win32_counter_offset = 0;

    if (win32_ticks_per_second == 0)
    {
        LARGE_INTEGER perf_frequency = {0};
        if (QueryPerformanceFrequency(&perf_frequency)) {
            win32_ticks_per_second = perf_frequency.QuadPart;
        }
        LARGE_INTEGER perf_counter = {0};
        if (QueryPerformanceCounter(&perf_counter)) {
            win32_counter_offset = perf_counter.QuadPart;
        }

        assert(win32_ticks_per_second != 0);
    }

    f64 result = 0;

    LARGE_INTEGER perf_counter;
    if (QueryPerformanceCounter(&perf_counter)) {
        perf_counter.QuadPart -= win32_counter_offset;
        result = (f64)(perf_counter.QuadPart) / win32_ticks_per_second;
    }

    return result;
}
 
int main(int argc, char *argv[]) 
{ 
    // Display help.
    if (argc <= 1)
    {
        char *prgm = argv[0];
        printf("%s <command>\n", prgm);
        printf("\n");
        printf("Example:\n");
        printf("\t%s cl main.c\n", prgm);

        return 0;
    }

    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    // Set the bInheritHandle flag so pipe handles are inherited. 
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL; 

    // Create a pipe for the child process's STDOUT. 
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
        error("StdoutRd CreatePipe"); 
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        error("Stdout SetHandleInformation"); 
    }

    // Create a pipe for the child process's STDIN. 
    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) {
        error("Stdin CreatePipe"); 
    }

    // Ensure the write handle to the pipe for STDIN is not inherited. 
    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
        error("Stdin SetHandleInformation"); 
    }
    
    char cmd[4096];
    int offset = 0;
    for (int i = 1; i < argc; i += 1)
    {
        int len = strlen(argv[i]);
        memcpy(cmd + offset, argv[i], len);
        offset += len;

        if (i < argc - 1)
        {
            cmd[offset] = ' ';
            offset += 1;
        }

        assert(offset < 4096);
    }

    cmd[offset] = '\0';
     
    // Set up members of the PROCESS_INFORMATION structure. 
    PROCESS_INFORMATION piProcInfo = {0};
     
    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.
    STARTUPINFOA siStartInfo = {0};
    siStartInfo.cb = sizeof(STARTUPINFOA); 
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
    // Create the child process. 
    BOOL bSuccess = CreateProcessA(NULL, 
        cmd,           // command line 
        NULL,          // process security attributes 
        NULL,          // primary thread security attributes 
        TRUE,          // handles are inherited 
        0,             // creation flags 
        NULL,          // use parent's environment 
        NULL,          // use parent's current directory 
        &siStartInfo,  // STARTUPINFO pointer 
        &piProcInfo);  // receives PROCESS_INFORMATION 
        
    // If an error occurs, exit the application. 
    if (!bSuccess) {
        error("CreateProcess");
    }

    f64 start_time = os_time();

    // Close handles to the child process and its primary thread.
    // Some applications might keep these handles to monitor the status
    // of the child process, for example. 
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
    
    // Close handles to the stdin and stdout pipes no longer needed by the child process.
    // If they are not explicitly closed, there is no way to recognize that the child process has ended.
    CloseHandle(g_hChildStd_OUT_Wr);
    CloseHandle(g_hChildStd_IN_Rd);
 
    // Read from pipe that is the standard output for child process. 

    DWORD dwRead, dwWritten; 
    CHAR chBuf[BUFSIZE]; 
    HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    for (;;) 
    { 
        BOOL bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) break; 

        bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
        if (!bSuccess) break; 
    } 

    // The remaining open handles are cleaned up when this process terminates. 
    // To avoid resource leaks in a larger application, close handles explicitly. 

    f64 end_time = os_time();
    printf("[time] %s (%.2fms)\n", cmd, (end_time - start_time) * 1000.0);

    return 0; 
}
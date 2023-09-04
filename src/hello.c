#include "gb_windows.h"

#include <stdio.h>
#include <stdint.h>

typedef int bool;
#define true 1
#define false 0

typedef double f64;
typedef uint64_t u64;

#pragma comment(lib, "kernel32")

HANDLE CreateWaitableTimerA(
    SECURITY_ATTRIBUTES *lpTimerAttributes, BOOL bManualReset, char *lpTimerName
);

void os_sleep(f64 seconds)
{
    // @Incomplete: only do this if win32_sleep_is_granular
    // Otherwise do some sort of busy wait thing

    static bool win32_sleep_is_granular = false;
    static bool win32_did_init_sleep = false;

    if (!win32_did_init_sleep)
    {
        HMODULE libwinmm = LoadLibraryA("winmm.dll");
        typedef UINT (WINAPI * timeBeginPeriod_t)(UINT);
        timeBeginPeriod_t timeBeginPeriod = (timeBeginPeriod_t)GetProcAddress(libwinmm, "timeBeginPeriod");
        if (timeBeginPeriod) {
            win32_sleep_is_granular = timeBeginPeriod(1) == 0 /* TIMERR_NOERROR */;
        }

        win32_did_init_sleep = true;
    }


    LARGE_INTEGER ft;
    ft.QuadPart = -(10 * (__int64)(seconds * 1000 * 1000));

    HANDLE timer = CreateWaitableTimerA(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}

int main(int argc, char **argv)
{
    printf("hello...");

    printf("[");
    for (int i = 0; i < argc; i += 1)
    {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(", ");
    }
    printf("]");
    printf("\n");

    os_sleep(200 / 1000.0);

    printf("goodbye.\n");
}
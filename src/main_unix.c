#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#if defined(__linux)
#  define HAVE_POSIX_TIMER
#  include <time.h>
#  ifdef CLOCK_MONOTONIC
#     define CLOCKID CLOCK_MONOTONIC
#  else
#     define CLOCKID CLOCK_REALTIME
#  endif
#elif defined(__APPLE__)
#  define HAVE_MACH_TIMER
#  include <mach/mach_time.h>
#elif defined(_WIN32)
# error "Windows is not supported. Use main.c instead"
#endif

typedef double f64;
typedef uint64_t u64;


void error(const char *string) 
{ 
    printf("%s\n", string);
    printf("Error Code: %d\n", errno);
    fflush(stdout);
    exit(1);
}

f64 os_time()
{
#if defined(__APPLE__)
    #if __MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
        static f64 macos_initial_clock = 0;
        if (!macos_initial_clock) {
            macos_initial_clock = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
        }
        return (f64)(clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW) - macos_initial_clock) / (f64)(1e9);
    #else
        static uint64_t is_init = 0;
        static mach_timebase_info_data_t info;
        if (0 == is_init) {
          mach_timebase_info(&info);
          is_init = 1;
        }
        uint64_t now;
        now = mach_absolute_time();
        now *= info.numer;
        now /= info.denom;
        return (f64)now / 1e-6;
    #endif
#elif defined(__linux)
    static uint64_t is_init = 0;
    static struct timespec linux_rate;
    if (0 == is_init) {
      clock_getres(CLOCKID, &linux_rate);
      is_init = 1;
    }
    uint64_t now;
    struct timespec spec;
    clock_gettime(CLOCKID, &spec);
    now = spec.tv_sec * 1.0e9 + spec.tv_nsec;
    return (f64)now / 1e-6;
#elif defined(_WIN32)
    return 0;
#endif
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

    f64 start_time = os_time();

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        error("Failed to run cmd");
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("%s", buffer);
    }

    int result = pclose(fp);

    f64 end_time = os_time();
    f64 elapsed_ms = (f64)(end_time - start_time) * 1000.0;
    printf("[time] %s (%.2fms)\n", cmd, elapsed_ms);

    return result;
}

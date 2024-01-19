#include <stdio.h> 
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>


void error(const char *string) 
{ 
    printf("%s\n", string);
    printf("Error Code: %d\n", errno);
    fflush(stdout);
    exit(1);
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

    clock_t start_time = clock();

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        error("Failed to run cmd");
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("%s", buffer);
    }

    int result = pclose(fp);

    clock_t end_time = clock();

    float elapsed = (float)(end_time - start_time) * 1000 / CLOCKS_PER_SEC;

    printf("[time] %s (%.2fms)\n", cmd, elapsed);

    return result;
}
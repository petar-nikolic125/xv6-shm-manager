// user/shmcreate.c
// ------------------------------------------------------------
// Create (and size) a named shared‐memory object and then
// fork/daemonize to hold one descriptor open so others can map.
// Supports “-h” for help, and “shmcreate <name>” defaults size to 8192.
// ------------------------------------------------------------

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

static void
usage(void)
{
    fprintf(2,
            "usage: shmcreate [-h] <name> [size-bytes]\n"
            "  -h            show this help message\n"
            "  <name>        shared object name\n"
            "  [size-bytes]  size in bytes (default 8192)\n"
    );
    exit();
}

int
main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3) {
        usage();
    }
    if (strcmp(argv[1], "-h") == 0) {
        usage();
    }

    char *name = argv[1];
    int sz = 8192;
    if (argc == 3) {
        sz = atoi(argv[2]);
        if (sz <= 0) {
            fprintf(2, "shmcreate: size must be > 0\n");
            exit();
        }
    }

    int fd = shm_open(name);
    if (fd < 0) {
        fprintf(2, "shmcreate: shm_open failed for \"%s\"\n", name);
        exit();
    }

    if (shm_trunc(fd, sz) < 0) {
        fprintf(2, "shmcreate: shm_trunc failed (already sized?)\n");
    } else {
        printf("shmcreate: object \"%s\" sized to %d bytes\n", name, sz);
    }

    // Fork and let parent exit so prompt returns
    int pid = fork();
    if (pid < 0) {
        fprintf(2, "shmcreate: fork failed\n");
        exit();
    } else if (pid > 0) {
        exit();
    }

    // Child holds the descriptor indefinitely
    printf("shmcreate: holding reference in pid %d - press ^C to terminate...\n", getpid());
    for (;;)
        sleep(1000);

    // never reached
    return 0;
}

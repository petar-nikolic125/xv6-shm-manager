// user/shmmap.c
// ------------------------------------------------------------
// Map an existing shared-memory object and optionally read/write a byte,
// then fork/daemonize to keep the mapping alive for inspection.
// Supports “-h” for help.
// ------------------------------------------------------------

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

static void
usage(void)
{
    fprintf(2,
            "usage: shmmap [-h] <name> r|rw [offset] [value]\n"
            "  -h           show this help message\n"
            "  <name>       shared object name\n"
            "  r|rw         map read-only or read-write\n"
            "  offset       byte index (default 0)\n"
            "  value        if supplied, byte to store at offset\n"
    );
    exit();
}

int
main(int argc, char *argv[])
{
    if (argc < 3 || argc > 5) {
        usage();
    }
    if (strcmp(argv[1], "-h") == 0) {
        usage();
    }

    char *name = argv[1];
    int write_mode = (argv[2][0]=='r' && argv[2][1]=='w');
    int flags      = write_mode ? O_RDWR : O_RDONLY;
    int offset     = (argc >= 4) ? atoi(argv[3]) : 0;
    int do_write   = (argc == 5);
    int new_val    = do_write ? (atoi(argv[4]) & 0xFF) : 0;

    int fd = shm_open(name);
    if (fd < 0) {
        fprintf(2, "shmmap: shm_open failed for \"%s\"\n", name);
        exit();
    }

    void *base;
    if (shm_map(fd, &base, flags) < 0) {
        fprintf(2, "shmmap: shm_map failed for \"%s\"\n", name);
        exit();
    }

    char *p = (char*)base + offset;
    if (do_write) {
        *p = new_val;
        printf("shmmap: wrote %d at offset %d\n", new_val, offset);
    } else {
        printf("shmmap: value at offset %d = %d\n", offset, (*p & 0xFF));
    }

    // Fork so prompt returns
    int pid = fork();
    if (pid < 0) {
        fprintf(2, "shmmap: fork failed\n");
        exit();
    } else if (pid > 0) {
        exit();
    }

    // Child keeps mapping alive
    printf("shmmap: mapped \"%s\" at %p - press ^C to terminate...\n", name, base);
    for (;;)
        sleep(1000);

    // never reached
    return 0;
}

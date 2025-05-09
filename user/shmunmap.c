// user/shmunmap.c
// ------------------------------------------------------------
// Drop one reference to a named shared-memory object.
// Supports “-h” for help.
// ------------------------------------------------------------

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

static void
usage(void)
{
    fprintf(2, "usage: shmunmap [-h] <name>\n");
    exit();
}

int
main(int argc, char *argv[])
{
    if (argc != 2 && !(argc == 2 && strcmp(argv[1], "-h")==0)) {
        usage();
    }
    if (strcmp(argv[1], "-h") == 0) {
        usage();
    }

    char *name = argv[1];
    int fd = shm_open(name);
    if (fd < 0) {
        fprintf(2, "shmunmap: shm_open failed for \"%s\"\n", name);
        exit();
    }

    if (shm_close(fd) < 0) {
        fprintf(2, "shmunmap: shm_close failed for \"%s\"\n", name);
    } else {
        printf("shmunmap: dropped reference to \"%s\"\n", name);
    }

    return 0;
}

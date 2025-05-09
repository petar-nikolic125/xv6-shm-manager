// user/value.c
// ------------------------------------------------------------
// Read or write a single byte at a given virtual address.
// Usage: value <addr> [newval]
//   <addr>    either decimal or 0x-prefixed hex
//   [newval]  (optional) byte to store at that address
// ------------------------------------------------------------

#include "kernel/types.h"
#include "user.h"

// Parse either decimal or 0x-hex into a uint
static uint
parse_addr(const char *s)
{
    uint v = 0;
    if (s[0]=='0' && (s[1]=='x' || s[1]=='X')) {
        // hex
        s += 2;
        char c;
        while ((c = *s++) != 0) {
            v <<= 4;
            if (c >= '0' && c <= '9')
                v |= c - '0';
            else if (c >= 'a' && c <= 'f')
                v |= c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                v |= c - 'A' + 10;
            else
                break;
        }
    } else {
        // decimal
        v = atoi(s);
    }
    return v;
}

static void
usage(void)
{
    printf("usage: value <addr> [newval]\n");
    exit();
}

int
main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3)
        usage();

    uint addr = parse_addr(argv[1]);
    uchar *p = (uchar*)addr;

    if (argc == 3) {
        uchar v = (uchar)(atoi(argv[2]) & 0xFF);
        *p = v;
        printf("value: wrote %d at %#x\n", v, addr);
    }

    printf("value: value at %#x = %d\n", addr, *p & 0xFF);
    exit();
}

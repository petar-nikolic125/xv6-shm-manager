#ifndef XV6_STRING_H
#define XV6_STRING_H

#include "types.h"

void  *memset(void *dst, int c, uint n);
int    memcmp(const void *v1, const void *v2, uint n);
void  *memmove(void *dst, const void *src, uint n);
void  *memcpy(void *dst, const void *src, uint n);
int    strncmp(const char *p, const char *q, uint n);
char  *strncpy(char *s, const char *t, int n);
char  *safestrcpy(char *s, const char *t, int n);
int    strlen(const char *s);

#endif  /* XV6_STRING_H */

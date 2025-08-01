#ifndef USER_USER_H
#define USER_USER_H

#include <kernel/types.h>

struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int  shm_open(char *name);
int  shm_trunc(int shmfd, int size);
int  shm_map(int shmfd, void **va, int flags);   // flags: O_RDONLY/O_RDWR
int  shm_close(int shmfd);


// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
char* strncpy(char*, const char*, int);
char* safestrcpy(char*, const char*, int);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...);
void printf(const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

#endif // USER_USER_H

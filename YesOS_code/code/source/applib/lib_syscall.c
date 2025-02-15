

#include "lib_syscall.h"

void msleep(int ms)
{
    if (ms <= 0)
        return;
    sycall_args_t args;
    args.id = SYS_sleep;
    args.arg0 = ms;
    sys_call(&args);
}

int getpid(void)
{
    sycall_args_t args;
    args.id = SYS_getpid;
    return sys_call(&args);
}

void print_msg(const char *fmt, int arg)
{
    sycall_args_t args;
    args.id = SYS_printmsg;
    args.arg0 = (int)fmt;
    args.arg1 = arg;
    sys_call(&args);
}
int fork()
{
    sycall_args_t args;
    args.id = SYS_fork;
    return sys_call(&args);
}

int execve(const char *name, char *const *argv, char *const *env)
{
    sycall_args_t args;
    args.id = SYS_execve;
    args.arg0 = (int)name;
    args.arg1 = (int)argv;
    args.arg2 = (int)env;
    return sys_call(&args);
}

int yield()
{
    sycall_args_t args;
    args.id = SYS_yield;
    return sys_call(&args);
}

int open(const char *name, int flags, ...)
{
    sycall_args_t args;
    args.id = SYS_open;
    args.arg0 = (int)name;
    args.arg1 = (int)flags;
    return sys_call(&args);
}
int read(int file, char *ptr, int len)
{
    sycall_args_t args;
    args.id = SYS_read;
    args.arg0 = (int)file;
    args.arg1 = (int)ptr;
    args.arg2 = (int)len;
    return sys_call(&args);
}
int write(int file, const char *ptr, int len)
{
    sycall_args_t args;
    args.id = SYS_write;
    args.arg0 = (int)file;
    args.arg1 = (int)ptr;
    args.arg2 = (int)len;
    return sys_call(&args);
}
int close(int file)
{
    sycall_args_t args;
    args.id = SYS_close;
    args.arg0 = (int)file;
    return sys_call(&args);
}
int lseek(int file, int ptr, int dir)
{
    sycall_args_t args;
    args.id = SYS_lseek;
    args.arg0 = (int)file;
    args.arg1 = (int)ptr;
    args.arg2 = (int)dir;
    return sys_call(&args);
}

int isatty(int file)
{
    sycall_args_t args;
    args.id = SYS_isatty;
    args.arg0 = (int)file;
    return sys_call(&args);
}
int fstat(int file, struct stat *st)
{
    sycall_args_t args;
    args.id = SYS_fstat;
    args.arg0 = (int)file;
    args.arg1 = (int)st;
    return sys_call(&args);
}

void *sbrk(ptrdiff_t incr)
{
    sycall_args_t args;
    args.id = SYS_sbrk;
    args.arg0 = (int)incr;
    return (void *)sys_call(&args);
}

int dup(int file)
{
    sycall_args_t args;
    args.id = SYS_dup;
    args.arg0 = (int)file;
    return sys_call(&args);
}

void _exit(int status)
{
    sycall_args_t args;
    args.id = SYS_exit;
    args.arg0 = status;
    sys_call(&args);
    while(1);
}

int wait(int *status)
{
    sycall_args_t args;
    args.id = SYS_wait;
    args.arg0 = (int)status;
    return sys_call(&args);
}
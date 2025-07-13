/**
 * @FilePath     : /code/source/shell/main.c
 * @Description  :  shell实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-12 12:27:19
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "lib_syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "main.h"
#include <sys/file.h>
#include "fs/file.h"
#include "dev/tty.h"
static cli_t cli;                   // 命令解释器
static const char *promot = "sh>>"; // 提示符
/**
 * @brief        : 打印提示符
 * @return        {*}
 **/
static void show_promot(void)
{
    printf("%s", promot);
    fflush(stdout); // 清空缓存
}
/**
 * @brief        : help命令
 * @param         {int} argc: argv中的参数个数
 * @param         {char} *: 参数
 * @return        {int} : 执行成功返回0,失败返回-1
 **/
static int do_help(int argc, char **argv)
{
    printf(ESC_COLOR_HELP ESC_COLOR_BOLD "Available Commands:\n" ESC_COLOR_RESET);
    printf(ESC_COLOR_HELP "==================\n" ESC_COLOR_DEFAULT);
    const cli_cmd_t *start = cli.cmd_start;
    while (start < cli.cmd_end)
    {
        printf("%s %s\n", start->name, start->usage);
        start++;
    }
    return 0;
}
/**
 * @brief        : clear命令,清空显示屏
 * @param         {int} argc: 参数个数
 * @param         {char} *: 具体参数
 * @return        {int} : 执行成功返回0,失败返回-1
 **/
static int do_clear(int argc, char **argv)
{
    printf("%s", ESC_CLEAR_SCREEN);      // 清屏
    printf("%s", ESC_MOVE_CURSOR(0, 0)); // 设置光标
    return 0;
}
/**
 * @brief        : echo命令,打印某些信息
 * @param         {int} argc: 参数个数
 * @param         {char} *: 具体参数
 * @return        {int} : 执行成功返回0,失败返回-1
 **/
static int do_echo(int argc, char **argv)
{
    if (argc == 1) // 只有echo一个参数
    {
        printf(ESC_COLOR_INFO "Enter message: " ESC_COLOR_DEFAULT);
        fflush(stdout);                         // 强制刷新输出缓冲区
        char msg_buf[128];                      // 等待输入缓冲区
        fgets(msg_buf, sizeof(msg_buf), stdin); // 等待用户输入
        msg_buf[strlen(msg_buf) - 1] = '\0';    // 将'\n'替换
        puts(msg_buf);                          // 输出
        return 0;
    }
    int count = 1; // 默认echo一次;
    int ch;
    while ((ch = getopt(argc, argv, "n:h")) != -1)
    {
        switch (ch)
        {
        case 'h':
            // puts("echo message");
            // puts("echo [-n count] message -- echo some message");
            printf(ESC_COLOR_HELP "Usage: echo message\n");
            printf("       echo [-n count] message -- echo some message\n" ESC_COLOR_DEFAULT);
            optind = 1; // getopt需要多次调用，需要重置
            return 0;
        case 'n':
            count = atoi(optarg);
            break;
        case '?':
            if (optarg)
            {
                fprintf(stderr, ESC_COLOR_ERROR "Unknown option: %s\n" ESC_COLOR_DEFAULT, cli.curr_input);
            }
            optind = 1; // getopt需要多次调用，需要重置
            return -1;
        default:
            break;
        }
    }
    if (optind > argc - 1)
    {
        fprintf(stderr, ESC_COLOR_ERROR "message is empty: %s\n" ESC_COLOR_DEFAULT, cli.curr_input);
        optind = 1; // getopt需要多次调用，需要重置
        return -1;
    }
    char *msg = argv[optind];
    for (int i = 0; i < count; i++)
    {
        puts(msg);
    }
    optind = 1; // getopt需要多次调用，需要重置
    return 0;
}
/**
 * @brief        : 结束当前进程
 * @param         {int} argc: 参数个数
 * @param         {char} *: 传递的参数
 * @return        {int} : 退出的状态
 **/
static int do_exit(int argc, char **argv)
{
    printf(ESC_COLOR_WARNING "Goodbye! Exiting shell...\n" ESC_COLOR_DEFAULT);
    exit(0); // 通知OS退出当前进程
    return 0;
}
/**
 * @brief        : 列出当前目录下的文件
 * @param         {int} argc: 参数个数
 * @param         {char} *: 传递的参数
 * @return        {int} : 退出的状态
 **/
static int do_ls(int argc, char **argv)
{
    DIR *p_dir = opendir("temp"); // 打开文件目录
    if (p_dir == NULL)            // 打开失败
    {

        printf("dir not exist.\n");
        return -1;
    }

    // 打印表头
    printf("Type  %-20s %10s\n", "Name", "Size");
    printf("----  %-20s %10s\n", "--------------------", "----------");

    struct dirent *entry;
    while ((entry = readdir(p_dir)) != NULL)
    {
        char type_char = entry->type == FILE_DIR ? 'D' : 'F';
        printf(" %c    %-20s %10d\n",
               type_char,
               strlwr(entry->name),
               entry->size);
    }

    closedir(p_dir); // 记得关闭目录
    return 0;
}
static int do_less(int argc, char **argv)
{
    int line_mode = 0;
    int ch;
    while ((ch = getopt(argc, argv, "lh")) != -1)
    {
        switch (ch)
        {
        case 'h':
            // puts("echo message");
            // puts("echo [-n count] message -- echo some message");
            printf(ESC_COLOR_HELP "Usage: show file content\n");
            printf("       less [-l] file -- show file content\n" ESC_COLOR_DEFAULT);
            optind = 1; // getopt需要多次调用，需要重置
            return 0;
        case 'l':
            line_mode = 1;
            break;
        case '?':
            if (optarg)
            {
                fprintf(stderr, ESC_COLOR_ERROR "Unknown option: %s\n" ESC_COLOR_DEFAULT, cli.curr_input);
            }
            optind = 1; // getopt需要多次调用，需要重置
            return -1;
        default:
            break;
        }
    }
    if (optind > argc - 1)
    {
        fprintf(stderr, ESC_COLOR_ERROR "no  file: %s\n" ESC_COLOR_DEFAULT);
        optind = 1; // getopt需要多次调用，需要重置
        return -1;
    }
    FILE *file = fopen(argv[optind], "r"); // 打开该文件
    if (NULL == file)                      // 打开失败
    {
        fprintf(stderr, ESC_COLOR_WARNING "open file failed,file : %s\n" ESC_COLOR_DEFAULT, argv[argc - 1]);
        optind = 1;
        return -1;
    }
    char *buf = (char *)malloc(255); // 申请缓冲区空间
    if (!line_mode)
    {
        while ((fgets(buf, 255, file)) != NULL) // 读取文件
        {
            fputs(buf, stdout);
        }
    }
    else
    {
        // 不使用缓存，这样能直接立即读取到输入而不用等回车
        setvbuf(stdin, NULL, _IONBF, 0);
        ioctl(0, TTY_CMD_ECHO, 0, 0);
        while (1)
        {
            char *b = fgets(buf, 255, file);
            if (b == NULL)
            {
                break;
            }
            fputs(buf, stdout);

            int ch;
            while ((ch = fgetc(stdin)) != 'n')
            {
                if (ch == 'q')
                {
                    goto less_quit;
                }
            }
        }
    less_quit:
        // 恢复为行缓存
        setvbuf(stdin, NULL, _IOLBF, BUFSIZ);
        ioctl(0, TTY_CMD_ECHO, 1, 0);
    }
    free(buf);    // 释放缓冲区
    fclose(file); // 关闭文件
    return 0;
}
static int do_cp(int argc, char **argv)
{
    if (argc < 3) // 参数没有三个
    {
        printf(ESC_COLOR_ERROR "no [from] or no [to]\n" ESC_COLOR_DEFAULT);
        return -1;
    }
    FILE *from, *to;             // 源文件指针与目的文件指针
    from = fopen(argv[1], "rb"); // 打开源文件以二进制形式
    to = fopen(argv[2], "wb");   // 打开目标文件
    if (!from || !to)
    {
        printf(ESC_COLOR_ERROR "open file [from] or [to] file failed.\n" ESC_COLOR_DEFAULT);
        goto cp_failed;
    }
    char *buf = (char *)malloc(255); // 申请255字节空间
    int size;
    while ((size = fread(buf, 1, 255, from)) > 0) // 读取文件内容
    {
        fwrite(buf, 1, size, to); // 将内容从buf中写入目的文件
    }
    free(buf); // 释放缓冲区
    return 0;
cp_failed:
    if (from)
    {
        fclose(from);
    }
    if (to)
    {
        fclose(to);
    }
    return -1;
}
static int do_rm(int argc,char **argv)
{
    if(argc < 2)
    {
        printf(ESC_COLOR_ERROR"no select file\n"ESC_COLOR_DEFAULT);
        return -1;
    }
    int err = unlink(argv[1]);
    if(err <0)
    {
        printf(ESC_COLOR_ERROR"rm file failed.\n"ESC_COLOR_DEFAULT);
        return -1;
    }
    return 0;
}
/**
 * 命令表
 */
static const cli_cmd_t cmd_list[] = {
    {
        .name = "help",
        .usage = "help --list supported command",
        .do_func = do_help,
    },
    {
        .name = "clear",
        .usage = "clear -- clear screen",
        .do_func = do_clear,
    },
    {
        .name = "echo",
        .usage = "echo [-n count] message -- echo some message",
        .do_func = do_echo,
    },
    {
        .name = "exit",
        .usage = "exit from current task",
        .do_func = do_exit,
    },
    {
        .name = "ls",
        .usage = "ls --list director",
        .do_func = do_ls,
    },
    {
        .name = "less",
        .usage = "less [-l] file -- show file content",
        .do_func = do_less,
    },
    {
        .name = "cp",
        .usage = "copy src dest -- copy file",
        .do_func = do_cp,
    },
    {
        .name = "rm",
        .usage="rm file -- remove file from disk",
        .do_func = do_rm
    },
};
/**
 * @brief        : 执行可执行的程序
 * @param         {char} *path: 程序的路径
 * @param         {int} argc: 参数个数
 * @param         {char} *: 参数
 * @return        {int} : 返回0
 **/
static int run_exec_file(const char *path, int argc, char **argv)
{
    int pid = fork(); // 创建子进程
    if (pid < 0)
    {
        fprintf(stderr, "fork failed %s", path);
    }
    else if (pid == 0) // 子进程
    {
        int err = execve(path, argv, (char *const *)0); // 执行可执行程序
        if (err < 0)
        {
            printf(ESC_COLOR_WARNING "exec failed.\n" ESC_COLOR_DEFAULT);
        }
        exit(-1); // 执行失败退出
    }
    else
    {
        int status;
        int pid = wait(&status); // 返回释放的子进程pid
        printf(ESC_COLOR_WARNING "task execute finish %s result: %d, pid = %d\n" ESC_COLOR_DEFAULT, path, status, pid);
    }
    return 0;
}
/**
 * @brief        : 查找内置的cmd命令
 * @param         {char} *name: 命令名称
 * @return        {cli_cmd_t *}: 返回找到的命令
 **/
static const cli_cmd_t *find_builtin(const char *name)
{
    for (const cli_cmd_t *cmd = cli.cmd_start; cmd < cli.cmd_end; cmd++) // 查找相关命令
    {
        if (strcmp(cmd->name, name) != 0) // 不是该命令,跳过
        {
            continue;
        }
        return cmd; // 找到了
    }
    return (cli_cmd_t *)0;
}
/**
 * @brief        : 执行内置cmd命令
 * @param         {cli_cmd_t} *cmd: 执行的命令的指针
 * @param         {int} argc: 指明argv中参数的个数
 * @param         {char} *: 传递的参数
 * @return        {*}
 **/
static void run_builtin(const cli_cmd_t *cmd, int argc, char **argv)
{
    int ret = cmd->do_func(argc, argv); // 执行该命令
    if (ret < 0)                        // 执行错误打印错误码
    {
        fprintf(stderr, ESC_COLOR_ERROR "Unknown error: %s\n" ESC_COLOR_DEFAULT, cli.curr_input);
    }
}
/**
 * @brief        : 命令行程序初始化
 * @param         {char *} promot: 提示符
 * @param         {cli_cmd_t *} cmd_list: 命令表
 * @param         {int} size: 大小
 * @return        {void}
 **/
static void cli_init(const char *promot, const cli_cmd_t *cmd_list, int size)
{
    cli.promot = promot;
    memset((void *)cli.curr_input, 0, CLI_INPUT_SIZE);
    cli.cmd_start = cmd_list;
    cli.cmd_end = cmd_list + size;
    
}
/**
 * @brief        : 查找可执行文件的路径
 * @param         {char *} filename: 文件名
 * @return        {const char *} : 可执行文件路径
 **/
static const char *find_exec_path(const char *filename)
{
    int fd = open(filename, 0);
    if (fd < 0)
    {
        return (const char *)0;
    }
    close(fd);
    return filename;
}
int main(int argc, char **argv)
{

    open(argv[0], O_RDWR); // int fd = 0 , stdin
    dup(0);                // 标准输出
    dup(0);                // 标准错误输出
    printf(ESC_CLEAR_SCREEN ESC_MOVE_CURSOR(0, 0));
    printf(ESC_COLOR_SUCCESS ESC_COLOR_BOLD "Launched successfully, Welcome to YOS!\n" ESC_COLOR_RESET);
    printf(ESC_COLOR_INFO "Build date: %s\n" ESC_COLOR_DEFAULT, __DATE__);
    cli_init(promot, cmd_list, sizeof(cmd_list) / sizeof(cmd_list[0])); // 命令行解释器初始化
    printf(ESC_COLOR_WARNING "Shell %c is running.\n" ESC_COLOR_DEFAULT, (argv[0][4] + 1));
    printf(ESC_COLOR_HELP "Type 'help' for available commands.\n" ESC_COLOR_DEFAULT);
    while (1)
    {
        show_promot();
        char *input_str = fgets(cli.curr_input, CLI_INPUT_SIZE, stdin); // 获取输入的字符
        if (input_str == (char *)0)
        {
            printf("%s\n", input_str);
            continue;
        }
        char *cr = strchr(cli.curr_input, '\n'); // 找到'\n'字符
        if (cr)                                  // 替换成'\0'
        {
            *cr = '\0';
        }
        cr = strchr(cli.curr_input, '\r'); // 找到'\r'字符
        if (cr)                            // 替换成'\0'
        {
            *cr = '\0';
        }
        int argc = 0;                  // 记录输入的参数的个数
        char *argv[CLI_MAX_ARG_COUNT]; // 存储输入的参数
        const char *space = " ";
        char *token = strtok(cli.curr_input, space); // 将字符串分割成多个字符串
        while (token)
        {
            if (argc >= CLI_MAX_ARG_COUNT) // 不超过最大参数数量
            {
                break;
            }
            argv[argc++] = token;
            token = strtok(NULL, space); // 下一个字符串
        }
        if (argc == 0) // 没有任何输入
        {
            continue;
        }
        const cli_cmd_t *cmd = find_builtin(argv[0]); // 查找内部命令
        if (cmd)
        {
            run_builtin(cmd, argc, argv);
            continue;
        }
        const char *path = find_exec_path(argv[0]); // 查找可执行程序路径
        if (path)
        {
            run_exec_file(path, argc, argv); // 执行可执行程序
            continue;
        }
        fprintf(stderr, ESC_COLOR_ERROR "Unknown command: %s\n" ESC_COLOR_DEFAULT, cli.curr_input);
    }
    return 0;
}

/**
 * @FilePath     : /code/source/shell/main.c
 * @Description  :  shell实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-09 11:12:57
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "lib_syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "main.h"

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
        for (int i = 0; i < argc; i++)
        {
            msleep(1000);
            printf("argc %d = %s\n", i, argv[i]);
        }
        exit(-1); // 结束子进程
    }
    else
    {
        int status;
        int pid = wait(&status); // 返回释放的子进程pid
        printf(ESC_COLOR_WARNING "cmd %s result: %d, pid = %d\n" ESC_COLOR_DEFAULT, path, status, pid);
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

int main(int argc, char **argv)
{

    open(argv[0], 0); // int fd = 0 , stdin
    dup(0);           // 标准输出
    dup(0);           // 标准错误输出
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

        run_exec_file("", argc, argv);

        fprintf(stderr, ESC_COLOR_ERROR "Unknown command: %s\n" ESC_COLOR_DEFAULT, cli.curr_input);
    }
    return 0;
}

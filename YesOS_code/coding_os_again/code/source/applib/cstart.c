/**
 * @FilePath     : /code/source/applib/cstart.c
 * @Description  :  cstart启动文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-06 16:14:11
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include <stdint.h>
int main(int argc, char **argv);
extern uint8_t __bss__start__[], __bss__end__[];
void cstart(int argc, char **argv)
{
    // bss区域清空
    uint8_t *start = __bss__start__;
    while (start < __bss__end__)
    {
        *start++ = 0;
    }
    main(argc, argv); // 跳转至应用程序运行
}

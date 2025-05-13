/**
 * @FilePath     : /code/source/kernel/include/tools/bitmap.h
 * @Description  :  位图相关定义头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-05-10 20:55:54
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef BITMAP_H
#define BITMAP_H

#include "comm/types.h"

typedef struct _bitmap_t
{
    int bit_count;
    uint8_t *bits;
} bitmap_t;

int bitmap_byte_count(int bit_count);
void bitmap_init(bitmap_t *bitmap, uint8_t *bits, int count, int init_bit);

// 获取指定位的分配情况
int bitmap_get_bit(bitmap_t *bitmap, int index);

// 连续设置count个位位
void bitmap_set_bit(bitmap_t *bitmap, int index, int count, int bit);

int bitmap_is_set(bitmap_t *bitmap, int index);

int bitmap_alloc_nbits(bitmap_t *bitmap, int bit, int count);

#endif
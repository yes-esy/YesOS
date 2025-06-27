/**
 * @FilePath     : /code/source/kernel/include/tools/bitmap.h
 * @Description  :  位图相关定义头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-07 20:20:45
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef BITMAP_H
#define BITMAP_H

#include "comm/types.h"

/**
 * @brief 位图数据结构
 */
typedef struct _bitmap_t
{
    int bit_count; // 位图的总位数
    uint8_t *bits; // 位图空间
} bitmap_t;
/**
 * @brief        : 获取所需要的字节数量
 * @param         {int} bit_count: 位图位数
 * @return        {int} 需要的字节数
 **/
int bitmap_byte_count(int bit_count);
/**
 * @brief        : 位图初始化
 * @param         {bitmap_t} *bitmap: 位图
 * @param         {uint8_t} *bits:  位图空间
 * @param         {int} count: 位图的总位数
 * @param         {int} init_bit: 初始化为0还是1
 * @return        {*}
 **/
void bitmap_init(bitmap_t *bitmap, uint8_t *bits, int count, int init_bit);

/**
 * @brief        : 获取指定位图索引的分配情况
 * @param         {bitmap_t} *bitmap: 位图指针
 * @param         {int} index: 指定位图的索引
 * @return        {int} 1为已被分配，0为未被分配
 **/
int bitmap_get_bit(bitmap_t *bitmap, int index);

/**
 * @brief        : 连续设置若干位
 * @param         {bitmap_t} *bitmap: 需要设置的位图
 * @param         {int} index: 要操作的起始位索引
 * @param         {int} count: 设置多少个连续的位
 * @param         {int} bit: 置0或1
 * @return        {*}
 **/
void bitmap_set_bit(bitmap_t *bitmap, int index, int count, int bit);

/**
 * @brief        : 检查位图某一个位是否被设置，1为已被设置，0为未被设置
 * @param         {bitmap_t} *bitmap:位图指针
 * @param         {int} index: 需要检查的位置的索引
 * @return        {int} 1为已被设置；0为未被设置
 **/
int bitmap_is_set(bitmap_t *bitmap, int index);
/**
 * @brief        : 连续分配若干指定比特位，返回起始索引
 * @param         {bitmap_t} *bitmap:
 * @param         {int} bit: 待设置的值1或0
 * @param         {int} count: 需要设置的位数
 * @return        {int} 连续分配的索引
 **/
int bitmap_alloc_nbits(bitmap_t *bitmap, int bit, int count);

#endif
/**
 * @FilePath     : /code/source/kernel/tools/bitmap.c
 * @Description  :  位图实现.c文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-07 20:19:37
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "tools/bitmap.h"
#include "tools/klib.h"

/**
 * @brief        : 获取所需要的字节数量
 * @param         {int} bit_count: 位图位数
 * @return        {int} 需要的字节数
 **/
int bitmap_byte_count(int bit_count)
{
    return (bit_count + 8 - 1) / 8; // 向上取整
}
/**
 * @brief        : 位图初始化
 * @param         {bitmap_t} *bitmap: 位图
 * @param         {uint8_t} *bits:  位图空间
 * @param         {int} count: 位图的总位数
 * @param         {int} init_bit: 初始化为0还是1
 * @return        {*}
 **/
void bitmap_init(bitmap_t *bitmap, uint8_t *bits, int count, int init_bit)
{
    bitmap->bit_count = count;
    bitmap->bits = bits;

    int bytes = bitmap_byte_count(bitmap->bit_count);

    // 对该位图所在区域清空
    kernel_memset((void *)bitmap->bits, init_bit ? 0xFF : 0, bytes);
}

/**
 * @brief        : 获取指定位图索引的分配情况
 * @param         {bitmap_t} *bitmap: 位图指针
 * @param         {int} index: 指定位图的索引
 * @return        {int} 1为已被分配，0为未被分配
 **/
int bitmap_get_bit(bitmap_t *bitmap, int index)
{
    // index/8 第几个字节
    // index%8 index在字节中的偏移量
    // @todo
    return (bitmap->bits[index / 8] & (1 << (index % 8))) ? 1 : 0;
}

/**
 * @brief        : 连续设置若干位
 * @param         {bitmap_t} *bitmap: 需要设置的位图
 * @param         {int} index: 要操作的起始位索引
 * @param         {int} count: 设置多少个连续的位
 * @param         {int} bit: 置0或1
 * @return        {*}
 **/
void bitmap_set_bit(bitmap_t *bitmap, int index, int count, int bit)
{
    /**
     * @todo 后续考虑优化加速
     */
    for (int i = 0; (i < count) && (index < bitmap->bit_count); i++, index++)
    {
        if (bit)
        {
            // index / 8：确定目标位所在的字节位置
            // index % 8: 确定目标位所在的字节偏移位置0~7
            bitmap->bits[index / 8] |= 1 << (index % 8);
        }
        else
        {
            bitmap->bits[index / 8] &= ~(1 << (index % 8));
        }
    }
}
/**
 * @brief        : 检查位图某一个位是否被设置，1为已被设置，0为未被设置
 * @param         {bitmap_t} *bitmap:位图指针
 * @param         {int} index: 需要检查的位置的索引
 * @return        {int} 1为已被设置；0为未被设置
 **/
int bitmap_is_set(bitmap_t *bitmap, int index)
{
    return bitmap_get_bit(bitmap, index) ? 1 : 0;
}

/**
 * @brief        : 连续分配若干指定比特位，返回起始索引
 * @param         {bitmap_t} *bitmap:
 * @param         {int} bit: 待设置的值1或0
 * @param         {int} count: 需要设置的位数
 * @return        {int} 连续分配的索引
 **/
int bitmap_alloc_nbits(bitmap_t *bitmap, int bit, int count)
{
    /**
     * @todo 后续考虑优化
     */
    int search_index = 0;
    int ok_index = -1;

    while (search_index < bitmap->bit_count)
    {
        if (bitmap_get_bit(bitmap, search_index) != bit)
        {
            search_index++;
            continue;
        }
        // 找到了
        ok_index = search_index;
        int i = 0;
        for (i = 1; (i < count) && (search_index < bitmap->bit_count); i++)
        {
            if (bitmap_get_bit(bitmap, search_index++) != bit)
            {
                ok_index = -1;
                break;
            }
        }
        if (i >= count)
        {
            bitmap_set_bit(bitmap, ok_index, count, !bit);
            return ok_index;
        }
    }
    return -1;
}
/**
 * @FilePath     : /code/source/kernel/tools/bitmap.c
 * @Description  :  位图实现.c文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-05-13 19:45:48
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "tools/bitmap.h"
#include "tools/klib.h"

/**
 * @brief        : 获取所需要的字节数量
 * @param         {int} bit_count:
 * @return        {*}
 **/
int bitmap_byte_count(int bit_count)
{
    return (bit_count + 8 - 1) / 8; // 向上取整
}

void bitmap_init(bitmap_t *bitmap, uint8_t *bits, int count, int init_bit)
{
    bitmap->bit_count = count;
    bitmap->bits = bits;

    int bytes = bitmap_byte_count(bitmap->bit_count);
    kernel_memset((void *)bitmap->bits, init_bit ? 0xFF : 0, bytes);
}

// 获取指定位的分配情况
int bitmap_get_bit(bitmap_t *bitmap, int index)
{
    // index/ 8 第几个字节
    // @todo
    return (bitmap->bits[index / 8] & (1 << (index % 8))) ? 1 : 0;
}

// 连续设置count个位位
void bitmap_set_bit(bitmap_t *bitmap, int index, int count, int bit)
{
    for (int i = 0; i < count && (index < bitmap->bit_count); i++, index++)
    {
        if (bit)
        {
            bitmap->bits[index / 8] |= (1 << (index % 8));
        }
        else
        {
            bitmap->bits[index / 8] &= ~(1 << (index % 8));
        }
    }
}
int bitmap_is_set(bitmap_t *bitmap, int index)
{
    return bitmap_get_bit(bitmap, index) ? 1 : 0;
}

int bitmap_alloc_nbits(bitmap_t *bitmap, int bit, int count)
{
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
            bitmap_set_bit(bitmap, ok_index, count, ~bit);
            return ok_index;
        }
    }
    return -1;
}
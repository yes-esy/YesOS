/**
 * @FilePath     : /code/source/kernel/tools/list.c
 * @Description  :  链表相关接口实现
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-25 21:59:30
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "tools/list.h"

/**
 * @brief        : 链表初始化函数
 * @param         {list_t *} list: 待初始化的链表
 * @return        {*}
 **/
void list_init(list_t *list)
{
    list->first = list->last = (list_node_t *)0;
    list->count = 0;
}
/**
 * @brief        : 链表头部插入结点
 * @param         {list_t} *list: 待插入的链表
 * @param         {list_node_t} *node: 待插入的结点
 * @return        {*}
**/
void list_insert_first(list_t *list, list_node_t *node)
{
    node->next = list->first;
    node->pre = (list_node_t *)0;
    // 链表为空;
    if (list_is_empty(list))
    {
        list->first = list->last = node;
    }
    else
    {
        list->first->pre = node;
        list->first = node;
    }
    list->count++;
}

/**
 * @brief        : 链表尾部插入结点
 * @param         {list_t} *list: 待插入的链表
 * @param         {list_node_t} *node: 待插入的结点
 * @return        {*}
**/
void list_insert_last(list_t *list, list_node_t *node)
{
    node->pre = list->last;
    node->next = (list_node_t *)0;

    if (list_is_empty(list))
    {
        list->first = list->last = node;
    }
    else
    {
        list->last->next = node;
        list->last = node;
    }
    list->count++;
}
/**
 * @brief        : 移除链表的头结点
 * @param         {list_t} *list: 待移除结点的链表的指针
 * @return        {list_node_t*}: 被移除的结点的指针
 **/
list_node_t *list_remove_first(list_t *list)
{
    if (list_is_empty(list))
    {
        return (list_node_t *)0;
    }

    list_node_t *remove_node = list->first;

    list->first = remove_node->next;

    if (list->first == (list_node_t *)0)
    {
        list->last = (list_node_t *)0;
    }
    else
    {
        remove_node->next->pre = (list_node_t *)0;
        // list->first->pre = (list_node_t *) 0;
    }

    remove_node->pre = remove_node->next = (list_node_t *)0;
    list->count--;
    return remove_node;
}

/**
 * @brief        : 移除链表中的指定结点
 * @param         {list_t} *list: 待移除结点的链表的指针
 * @param         {list_node_t} *node: 需要移除的结点的指针
 * @return        {list_node_t*}: 被移除的结点的指针
**/
list_node_t *list_remove(list_t *list, list_node_t *node)
{
    if (node == list->first)
    {
        list->first = node->next;
    }

    if (node == list->last)
    {
        list->last = node->pre;
    }

    if (node->pre)
    {
        node->pre->next = node->next;
    }

    if (node->next)
    {
        node->next->pre = node->pre;
    }
    node->pre = node->next = (list_node_t *)0;

    list->count--;
    return node;
}
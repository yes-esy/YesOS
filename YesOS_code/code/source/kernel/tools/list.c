#include "tools/list.h"

void list_init(list_t *list)
{
    list->first = list->last = (list_node_t *)0;
    list->count = 0;
}

/**
 * 将指定表项插入到指定链表的头部
 * @param list 待插入的链表
 * @param node 待插入的结点
 */
void list_insert_first(list_t *list, list_node_t *node)
{
    // 设置好待插入结点的前后，前面为空
    node->next = list->first;
    node->prev = (list_node_t *)0;

    // 如果为空，需要同时设置first和last指向自己
    if (list_is_empty(list))
    {
        list->last = list->first = node;
    }
    else
    {
        // 否则，设置好原本第一个结点的pre
        list->first->prev = node;

        // 调整first指向
        list->first = node;
    }

    list->count++;
}

/**
 * 将指定表项插入到指定链表的尾部
 * @param list 操作的链表
 * @param node 待插入的结点
 */
void list_insert_last(list_t *list, list_node_t *node)
{
    // 设置好结点本身
    node->prev = list->last;
    node->next = (list_node_t *)0;

    // 表空，则first/last都指向唯一的node
    if (list_is_empty(list))
    {
        list->first = list->last = node;
    }
    else
    {
        // 否则，调整last结点的向一指向为node
        list->last->next = node;

        // node变成了新的后继结点
        list->last = node;
    }

    list->count++;
}

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
        remove_node->next->prev = (list_node_t *)0;
    }

    remove_node->next = remove_node->prev = (list_node_t *)0;

    list->count--;
    return remove_node;
}

list_node_t *list_remove(list_t *list, list_node_t *node)
{
    if (node == list->first)
    {
        list->first = node->next;
    }

    if (node == list->last)
    {
        list->last = node->prev;
    }

    if (node->prev)
    {
        node->prev->next = node->next;
    }

    if (node->next)
    {
        node->next->prev = node->prev;
    }

    node->prev = node->next = (list_node_t *)0;
    list->count--;
    return node;
}
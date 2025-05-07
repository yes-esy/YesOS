/**
 * @FilePath     : /code/source/kernel/include/tools/list.h
 * @Description  :  链表相关定义
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-26 14:29:20
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef LIST_H
#define LIST_H

typedef struct _list_node_t
{
    struct _list_node_t *pre;
    struct _list_node_t *next;
} list_node_t;

/**
 * @brief        : 链表节点初始化;
 * @param         {list_node_t *} node: 需要初始化的节点
 * @return        {*}
 **/
static inline void list_node_init(list_node_t *node)
{
    node->pre = node->next = (list_node_t *)0;
}

/**
 * @brief        : 链表的前驱节点
 * @param         {list_node_t *} node: 当前节点
 * @return        {list_node_t *} 当前节点的前去节点
 **/
static inline list_node_t *list_node_pre(list_node_t *node)
{
    return node->pre;
}

/**
 * @brief        : 链表的后继节点
 * @param         {list_node_t *} node: 当前节点
 * @return        {list_node_t *} 当前节点的后继节点
 **/
static inline list_node_t *list_node_next(list_node_t *node)
{
    return node->next;
}

typedef struct _list_t
{
    list_node_t *first; // 指向第一个结点
    list_node_t *last;  // 指向下一个结点
    int count;          // 链表中的结点个数
} list_t;

// 链表初始化函数
void list_init(list_t *list);

/**
 * @brief        : 判断链表是否为空
 * @param         {list_t} *list: 需要判断的链表
 * @return        {int}: 为1表示为空,0表示不为空
 **/
static inline int list_is_empty(list_t *list)
{
    return list->count == 0;
}
/**
 * @brief        : 返回链表的头结点
 * @param         {list_t} *list: 链表
 * @return        {list_node_t*}: 链表的头结点
 **/
static inline list_node_t *list_first(list_t *list)
{
    return list->first;
}
/**
 * @brief        : 返回链表的尾结点
 * @param         {list_t} *list: 对应链表
 * @return        {list_node_t*}: 链表尾结点
 **/
static inline list_node_t *list_last(list_t *list)
{
    return list->last;
}
/**
 * @brief        : 返回链表的结点数量
 * @param         {list_t} *list: 对应链表
 * @return        {int}: 链表数量
**/
static inline int list_count(list_t *list)
{
    return list->count;
}

// 链表头部插入节点
void list_insert_first(list_t *list, list_node_t *node);
// 链表尾部插入节点
void list_insert_last(list_t *list, list_node_t *node);
// 删除头节点
list_node_t *list_remove_first(list_t *list);
// 删除指定节点
list_node_t *list_remove(list_t *list, list_node_t *node);

/**
 * 1. 求结点在所在结构中的偏移:
 * 定义一个指向0的指针，用(struct aa *)&0->node，
 * 所得即为node字段在整个结构体的偏移
 */
#define offset_in_parent(parent_type, node_name) \
    ((uint32_t)&(((parent_type *)0)->node_name))

/**
 * 2. 求node所在的结构体首址：node的地址 - node的偏移
 * 即已知a->node的地址，求a的地址
 */
#define offset_to_parent(node, parent_type, node_name) \
    ((uint32_t)node - offset_in_parent(parent_type, node_name))

/**
 * 3. 进行转换: (struct aa *)addr
 * list_node_parent(node_addr, struct aa, node_name)
 */
#define list_node_parent(node, parent_type, node_name) \
    ((parent_type *)(node ? offset_to_parent((node), parent_type, node_name) : 0))

#endif

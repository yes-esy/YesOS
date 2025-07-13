/**
 * @FilePath     : /code/source/kernel/include/dev/dev.h
 * @Description  :  设备管理抽象层
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-08 10:25:17
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef DEV_H
#define DEV_H
#define DEV_NAME_SIZE 32
/**
 * 描述一种具体的设备
 */
typedef struct _device_t
{
    struct _dev_desc_t *desc; // 设备的相关描述
    int mode;                 // 模式
    int minor;                // 次设备号
    void *data;               // 存放的参数
    int open_count;           // 打开次数
} device_t;

/**
 * 设备类型
 */
enum
{
    DEV_UNKNOWN = 0, // 未知设备
    DEV_TTY,         // tty设备类型
    DEV_DISK,        // disk(磁盘)设备类型
};

/**
 * 描述某一种类型设备的结构
 */
typedef struct _dev_desc_t
{
    char name[DEV_NAME_SIZE]; // 设备名
    int major;                // 主设备号
    /**
     * @brief        : 打开某一设备的函数接口
     * @param         {device_t *} dev: 打开的设备
     * @return        {int} : 若成功打开返回0，否则返回-1
     **/
    int (*open)(device_t *dev); // 打开设备的接口
    /**
     * @brief        : 读取设备的数据
     * @param         {device_t} *dev: 具体的设备的指针
     * @param         {int} addr: 从设备的何处读取
     * @param         {char *} buf: 读取到哪里
     * @param         {int} size: 读取的数据量
     * @return        {int} : 返回0
     **/
    int (*read)(device_t *dev, int addr, char *buf, int size);
    /**
     * @brief        : 往设备写入数据
     * @param         {device_t} *dev: 写入数据的设备
     * @param         {int} addr: 从设备的哪里开始写
     * @param         {char} *buf: 写入的数据的起始地址
     * @param         {int} size: 写入的数据量
     * @return        {int} : 返回0
     **/
    int (*write)(device_t *dev, int addr, char *buf, int size);

    /**
     * @brief        : 控制设备
     * @param         {device_t *} dev: 具体的设备描述指针
     * @param         {int} cmd: 控制命令
     * @param         {int} arg0: 命令的参数1
     * @param         {int} arg1: 命令的参数2
     * @return        {int} : 返回0
     **/
    int (*control)(device_t *dev, int cmd, int arg0, int arg1);
    /**
     * @brief        : 关闭设备
     * @param         {device_t *} dev: 具体的设备描述的指针
     * @return        {void}
     **/
    void (*close)(device_t *dev);
} dev_desc_t;

/**
 * @brief        : 设备的打开
 * @param         {int} major: 主设备号
 * @param         {int} minor: 次设备号
 * @param         {void *} data: 传输的数据
 * @return        {int} : 返回设备Id
 **/
int dev_open(int major, int minor, void *data);
/**
 * @brief        : 从设备读取数据
 * @param         {int} dev_id: 设备Id
 * @param         {int} addr: 从哪个地址开始读取
 * @param         {char} *buf: 数据存放位置
 * @param         {int} size: 读取数据量
 * @return        {int} : 读取的数据量
 **/
int dev_read(int dev_id, int addr, char *buf, int size);
/**
 * @brief        : 往设备写入数据
 * @param         {int} dev_id: 设备Id
 * @param         {int} addr: 写入的地址
 * @param         {char} *buf: 从哪里取数据写
 * @param         {int} size: 写入的数据量
 * @return        {int} : 写入的数据量
 **/
int dev_write(int dev_id, int addr, char *buf, int size);
/**
 * @brief        : 控制设备
 * @param         {int} dev_id: 设备Id
 * @param         {int} cmd: 控制命令
 * @param         {int} arg0: 控制参数1
 * @param         {int} arg1: 控制参数2
 * @return        {int} : 返回0
 **/
int dev_control(int dev_id, int cmd, int arg0, int arg1);
/**
 * @brief        : 关闭设备
 * @param         {int} dev_id: 设备Id
 * @return        {void}
 **/
void dev_close(int dev_id);
#endif

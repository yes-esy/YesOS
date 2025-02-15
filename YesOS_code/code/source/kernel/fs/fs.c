#include "fs/fs.h"
#include "comm/types.h"
#include "tools/klib.h"
#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "sys/stat.h"
#include "tools/log.h"
#include "ipc/sem.h"
#include "dev/console.h"
#include "fs/file.h"
#include "core/task.h"
#include "dev/dev.h"
#include <sys/file.h>
#include "dev/disk.h"

#define TEMP_FILE_ID 100
#define TEMP_ADDR (8 * 1024 * 1024) // 在0x800000处缓存原始
#define FS_TABLE_SIZE 10

static list_t mounted_list;
static fs_t fs_table[FS_TABLE_SIZE];
static list_t free_list;

static uint8_t *temp_pos; // 当前位置

extern fs_op_t devfs_op;

/**
 * @brief 使用LBA48位模式读取磁盘
 *
static void read_disk(int sector, int sector_count, uint8_t *buf)
{
    outb(0x1F6, (uint8_t)(0xE0));

    outb(0x1F2, (uint8_t)(sector_count >> 8));
    outb(0x1F3, (uint8_t)(sector >> 24)); // LBA参数的24~31位
    outb(0x1F4, (uint8_t)(0));            // LBA参数的32~39位
    outb(0x1F5, (uint8_t)(0));            // LBA参数的40~47位

    outb(0x1F2, (uint8_t)(sector_count));
    outb(0x1F3, (uint8_t)(sector));       // LBA参数的0~7位
    outb(0x1F4, (uint8_t)(sector >> 8));  // LBA参数的8~15位
    outb(0x1F5, (uint8_t)(sector >> 16)); // LBA参数的16~23位

    outb(0x1F7, (uint8_t)0x24);

    // 读取数据
    uint16_t *data_buf = (uint16_t *)buf;
    while (sector_count-- > 0)
    {
        // 每次扇区读之前都要检查，等待数据就绪
        while ((inb(0x1F7) & 0x88) != 0x8)
        {
        }

        // 读取并将数据写入到缓存中
        for (int i = 0; i < SECTOR_SIZE / 2; i++)
        {
            *data_buf++ = inw(0x1F0);
        }
    }
}
*/

/**
 * @brief 检查路径是否正常
 */
static int is_path_valid(const char *path)
{
    if ((path == (const char *)0) || (path[0] == '\0'))
    {
        return 0;
    }

    return 1;
}

int path_to_num(char *path, int *num)
{
    int n = 0;
    const char *c = path;
    while (*c)
    {
        n = n * 10 + (*c - '0');
        c++;
    }
    *num = n;
    return 0;
}

const char *path_next_child(const char *path)
{
    const char *c = path;
    while (*c++ == '/')
    {
        ;
    }

    while (*c++ != '/')
    {
        ;
    }
    return *c ? c : (const char *)0;
}

/**
 * @brief 比较路径是否相同
 * @param path 路径
 * @param str 字符串
 */
int path_begin_with(const char *path, const char *str)
{
    const char *s1 = path, *s2 = str;
    while (*s1 && *s2 && *s1 == *s2)
    {
        s1++;
        s2++;
    }

    return *s2 == '\0';
}

// 互斥锁
static void fs_protect(fs_t *fs)
{
    if (fs->mutex)
    {
        mutex_lock(fs->mutex);
    }
}
static void fs_unprotect(fs_t *fs)
{
    if (fs->mutex)
    {
        mutex_unlock(fs->mutex);
    }
}

static int is_fd_bad(int fd)
{
    if ((fd < 0) && (fd >= TASK_OFILE_NR))
    {
        return 1;
    }
    return 0;
}

/**
 * @brief 打开文件
 * @param name 文件名
 * @param flags 文件标志
 *
 */
int sys_open(const char *name, int flags, ...)
{

    if (kernel_strncmp(name, "/shell.elf", 4) == 0)
    {
        int dev_id = dev_open(DEV_DISK,0xa0,(void *)0);
        dev_read(dev_id,5000,(uint8_t *)TEMP_ADDR,80);
        // read_disk(5000, 80, (uint8_t *)TEMP_ADDR);
        temp_pos = (uint8_t *)TEMP_ADDR;
        return TEMP_FILE_ID;
    }

    file_t *file = file_alloc();
    if (!file)
    {
        log_printf("No free file\n");
        goto sys_open_err;
    }
    int fd = -1;

    fd = task_alloc_fd(file);
    if (fd < 0)
    {
        log_printf("No free fd\n");
        goto sys_open_err;
    }

    fs_t *fs = (fs_t *)0;

    list_node_t *node = list_first(&mounted_list);
    while (node)
    {
        fs_t *curr = list_node_parent(node, fs_t, node);
        if (path_begin_with(name, curr->mount_point))
        {
            fs = curr;
            break;
        }
        node = list_node_next(node);
    }

    if (fs)
    {
        name = path_next_child(name);
    }
    else
    {
    }
    file->mode = flags;
    file->fs = fs;
    kernel_strncpy(file->file_name, name, FILE_NAME_SIZE);
    fs_protect(fs);
    int err = fs->op->open(fs, name, file);
    if (err < 0)
    {
        fs_unprotect(fs);
        log_printf("open failed");
        goto sys_open_err;
    }
    fs_unprotect(fs);
    return fd;
sys_open_err:
    if (file)
    {
        file_free(file);
    }
    if (fd >= 0)
    {
        task_remove_fd(fd);
    }
    return -1;
}

/**
 * @brief 读取文件
 * @param file 文件id
 * @param ptr 缓冲区
 * @param len 长度
 */
int sys_read(int file, char *ptr, int len)
{
    if (file == TEMP_FILE_ID)
    {
        kernel_memcpy(ptr, temp_pos, len);
        temp_pos += len;
        return len;
    }
    if (is_fd_bad(file) || !ptr || !len)
    {
        return 0;
    }
    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("No file");
        return -1;
    }
    if (p_file->mode == O_WRONLY)
    {
        log_printf("file is write only");
        return -1;
    }
    fs_t *fs = p_file->fs;
    fs_protect(fs);
    int err = fs->op->read(ptr, len, p_file);
    fs_unprotect(fs);
    return err;
}

/**
 * @brief 写文件
 * @param file 文件id
 * @param ptr 缓冲区
 * @param len 长度
 */
int sys_write(int file, char *ptr, int len)
{
    if (is_fd_bad(file) || !ptr || !len)
    {
        return 0;
    }
    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("No file");
        return -1;
    }
    if (p_file->mode == O_RDONLY)
    {
        log_printf("file is read only");
        return -1;
    }
    fs_t *fs = p_file->fs;
    fs_protect(fs);
    int err = fs->op->write(ptr, len, p_file);
    fs_unprotect(fs);
    return err;
}

/**
 * @brief 移动文件指针
 * @param file 文件id
 * @param ptr 指针
 * @param dir
 */
int sys_lseek(int file, int ptr, int dir)
{
    if (file == TEMP_FILE_ID)
    {
        temp_pos = (uint8_t *)(TEMP_ADDR + ptr);
        return 0;
    }
    if (is_fd_bad(file))
    {
        return 0;
    }
    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("No file");
        return -1;
    }

    fs_t *fs = p_file->fs;
    fs_protect(fs);
    int err = fs->op->seek(p_file, ptr, dir);
    fs_unprotect(fs);
    return err;
}

/**
 * @brief 关闭文件
 * @param file 文件id
 */
int sys_close(int file)
{
    if (file == TEMP_FILE_ID)
    {
        return 0;
    }
    if (is_fd_bad(file))
    {
        log_printf("invalid file %d", file);
        return 0;
    }
    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("No file");
        return -1;
    }
    ASSERT(p_file->ref > 0);

    if (p_file->ref-- == 1)
    {
        fs_t *fs = p_file->fs;
        fs_protect(fs);
        fs->op->close(p_file);
        fs_unprotect(fs);
        file_free(p_file);
    }
    task_remove_fd(file);
    return 0;
}

int sys_isatty(int file)
{
    if (is_fd_bad(file))
    {
        return 0;
    }
    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("No file");
        return -1;
    }
    return p_file->type == FILE_TTY;
}

int sys_fstat(int file, struct stat *st)
{
    if (is_fd_bad(file))
    {
        return 0;
    }
    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("No file");
        return -1;
    }
    fs_t *fs = p_file->fs;
    kernel_memset((void *)st, 0, sizeof(struct stat));
    fs_protect(fs);
    int err = fs->op->stat(p_file, st);
    fs_unprotect(fs);
    return err;
}

int sys_dup(int file)
{
    if (is_fd_bad(file))
    {
        log_printf("invalid file %d\n", file);
        return -1;
    }
    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("No file\n");
        return -1;
    }

    int fd = task_alloc_fd(p_file);
    if (fd < 0)
    {
        log_printf("No free fd\n");
        return -1;
    }
    else
    {
        file_inc_ref(p_file);
        return fd;
    }
}

static void mount_list_init()
{
    list_init(&free_list);

    for (int i = 0; i < FS_TABLE_SIZE; i++)
    {
        list_insert_first(&free_list, &fs_table[i].node);
    }
    list_init(&mounted_list);
}
static fs_op_t *get_fs_op(fs_type_t type, int major)
{
    switch (type)
    {
    case FS_DEVFS:
        return &(devfs_op);
    default:
        return (fs_op_t *)0;
    }
}
static fs_t *mount(fs_type_t type, char *mount_point, int dev_major, int dev_minor)
{
    fs_t *fs = (fs_t *)0;
    log_printf("mount file system, name:%s ,dev : %x", mount_point, dev_major);
    list_node_t *curr = list_first(&mounted_list);
    while (curr)
    {
        fs_t *fs = list_node_parent(curr, fs_t, node);
        if (kernel_strncmp(fs->mount_point, mount_point, FS_MOUNTP_SIZE) == 0)
        {
            log_printf("file system already mounted");
            goto mount_failed;
        }
        curr = list_node_next(curr);
    }

    list_node_t *free_node = list_remove_first(&free_list);

    if (!free_node)
    {
        log_printf("no free fs");
        goto mount_failed;
    }

    fs = list_node_parent(free_node, fs_t, node);
    fs_op_t *op = get_fs_op(type, dev_major);
    if (!op)
    {
        log_printf("no fs op");
        goto mount_failed;
    }
    kernel_memset((void *)fs, 0, sizeof(fs_t));
    kernel_strncpy(fs->mount_point, mount_point, FS_MOUNTP_SIZE);
    fs->op = op;
    if (op->mount(fs, dev_major, dev_minor) < 0)
    {
        log_printf("mount failed %s", mount_point);
        goto mount_failed;
    }
    list_insert_last(&mounted_list, &fs->node);
    return fs;
mount_failed:
    log_printf("mount failed");
    if (fs)
    {
        list_insert_first(&free_list, &fs->node);
    }
    return (fs_t *)0;
}

void fs_init()
{
    mount_list_init();
    file_table_init();
    // log_printf("fs init done.\n");
    disk_init();

    fs_t *fs = mount(FS_DEVFS, "/dev", 0, 0);

    ASSERT(fs != (fs_t *)0);
}
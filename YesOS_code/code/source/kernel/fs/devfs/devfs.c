#include "fs/devfs/devfs.h"
#include "fs/fs.h"
#include "dev/dev.h"
#include "tools/klib.h"
#include "tools/log.h"

static devfs_type_t devfs_type_list[] = {
    {
        .name = "tty",
        .dev_type = DEV_TTY,
        .file_type = FILE_TTY,
    }};
int devfs_mount(fs_t *fs, int major, int minor)
{
    fs->type = FS_DEVFS;
}
/**
 * @brief 卸载指定的设备
 * @param fs 
 */
int devfs_unmount (fs_t * fs) {
    return 0;
}

/**
 * @brief 打开指定的设备以进行读写
 */
int devfs_open(fs_t *fs, const char *path, file_t *file)
{
    // 遍历所有支持的设备类型列表，根据path中的路径，找到相应的设备类型
    for (int i = 0; i < sizeof(devfs_type_list) / sizeof(devfs_type_list[0]); i++) {
        devfs_type_t * type = devfs_type_list + i;

        // 查找相同的名称，然后从中提取后续部分，转换成字符串
        int type_name_len = kernel_strlen(type->name);

        // 如果存在挂载点路径，则跳过该路径，取下级子目录
        if (kernel_strncmp(path, type->name, type_name_len) == 0) {
            int minor;

            // 转换得到设备子序号
            if ((kernel_strlen(path) > type_name_len) && (path_to_num((char *)(path + type_name_len), &minor)) < 0) {
                log_printf("Get device num failed. %s", path);
                break;
            }

            // 打开设备
            int dev_id = dev_open(type->dev_type, minor, (void *)0);
            if (dev_id < 0) {
                log_printf("Open device failed:%s", path);
                break;
            }

            // 纪录所在的设备号
            file->dev_id = dev_id;
            file->fs = fs;
            file->pos = 0;
            file->size = 0;
            file->type = type->file_type;
            return 0;
        }
    }

    return -1;
}

int devfs_write(char *buf, int size, file_t *file)
{
    return dev_write(file->dev_id, file->pos, buf, size);
}

int devfs_read(char *buf, int size, file_t *file)
{
    return dev_read(file->dev_id, file->pos, buf, size);
    // fs->type = FS_DEVFS;
}

void devfs_close(file_t *file)
{
    return dev_close(file->dev_id);
}

int devfs_seek(file_t *file, uint32_t offset, int dir)
{
    // fs->type = FS_DEVFS
    return -1;
}

int devfs_stat(file_t *file, struct stat *st)
{
    // fs->type = FS_DEVFS;
    return -1;
}

fs_op_t devfs_op = {
    .mount = devfs_mount,
    .unmount = devfs_unmount,
    .open = devfs_open,
    .write = devfs_write,
    .read = devfs_read,
    .close = devfs_close,
    .seek = devfs_seek,
    .stat = devfs_stat,
};
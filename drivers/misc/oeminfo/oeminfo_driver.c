#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <uapi/linux/oeminfo_driver.h>

#undef  pr_fmt
#define pr_fmt(fmt)    "OEMINFOLog: " fmt

#define OEMINFO_FILENAME "/dev/block/bootdevice/by-name/oeminfo"
#define IS_INVALID_FILP(filp) (!filp || IS_ERR(filp))

static struct file *oeminfo_filp = NULL;
static DEFINE_MUTEX(oeminfo_lock);
static char blockName[ONE_BLOCK_SIZE];

const char *oeminfo_map[] = {
    [OEMINFO_PRODUCTLINE]      = "PRODUCTLINE",
    [OEMINFO_CUSTOMIZATION]    = "CUSTOMIZATION",
    [OEMINFO_LCD]              = "LCD",
    [OEMINFO_CALIBRATION]      = "CALIBRATION",
    [OEMINFO_MISC]             = "MISC",
    [OEMINFO_FCTINFO]          = "FCTINFO"
};

int oeminfo_node_open(void)
{
    int ret = 0;

    if (IS_INVALID_FILP(oeminfo_filp)) {
        pr_info("%s: open \n", __func__);
        oeminfo_filp = filp_open(OEMINFO_FILENAME, O_RDWR | O_SYNC, 0);
        if (IS_INVALID_FILP(oeminfo_filp))
        {
            pr_err("%s: invalid oeminfo_filp \n", __func__);
            ret = -1;
        }
    }

    return ret;
}

static int oeminfo_get_pageoffset(const char *buf)
{
    int i;

    if(NULL == buf)
    {
        pr_err("%s: buf is NULL \n", __func__);
        return -EINVAL;
    }

    for (i = 0; i < OEMINFO_COUNT; i++) {
        if (!strncmp(oeminfo_map[i], buf, TAG_LENGTH)) {
            pr_err("find tag name is [%s]\n", buf);
            return i;
        }
    }

    pr_err("can't find tag name [%s]\n", buf);
    return -ERANGE;
}

int oeminfo_open (struct inode* inode ,struct file* file)
{
    int ret;

    mutex_lock(&oeminfo_lock);
    ret = oeminfo_node_open();
    mutex_unlock(&oeminfo_lock);

    return ret;
}

static ssize_t oeminfo_read(struct file *file, char __user *buf,
                    size_t count , loff_t *ppos) {
    int len;
    int block;
    u_char buffer[512];

    if (count != ONE_BLOCK_SIZE)
    {
        pr_err("%s: count must be ONE_BLOCK_SIZE[%d] \n", __func__, ONE_BLOCK_SIZE);
        return -1;
    }

    block = oeminfo_get_pageoffset(blockName);
    if (block < 0) {
        pr_err("%s: Can't find tag %s !\n", __func__, blockName);
        return -ERANGE;
    }

    mutex_lock(&oeminfo_lock);
    if (-1 == oeminfo_node_open())
    {
        mutex_unlock(&oeminfo_lock);
        pr_err("%s: open fail\n", __func__);
        return -1;
    }

    len = kernel_read(oeminfo_filp, block*ONE_BLOCK_SIZE, buffer, ONE_BLOCK_SIZE);
    mutex_unlock(&oeminfo_lock);

    if (copy_to_user(buf, buffer, ONE_BLOCK_SIZE))
        return -EFAULT;

    pr_info("%s: count=%zu len=%d \n", __func__, count, len);

    if (len <= 0)
    {
        pr_err("%s: read fail\n", __func__);
        return -1;
    }

    return len;
}

static ssize_t oeminfo_write(struct file *file, const char __user *buf,
                 size_t count, loff_t *ppos) {
    int block;
    int result;
    const char __user *ptr = buf;
    u_char buffer[512];

    if(NULL == buf)
    {
        pr_err("%s: buf is NULL \n", __func__);
        return -EINVAL;
    }

    if (count != ONE_BLOCK_SIZE)
    {
        pr_err("%s: count must be ONE_BLOCK_SIZE[%d] \n", __func__, ONE_BLOCK_SIZE);
        return -1;
    }

    if (copy_from_user(buffer, ptr, count))
        return -EFAULT;

    block = oeminfo_get_pageoffset(buffer);
    if (block < 0) {
        pr_err("%s: Can't find tag %s !\n", __func__, buffer);
        return -ERANGE;
    }

    mutex_lock(&oeminfo_lock);
    if (-1 == oeminfo_node_open())
    {
        mutex_unlock(&oeminfo_lock);
        pr_err("%s: open fail \n", __func__);
        return -1;
    }
    result = kernel_write(oeminfo_filp, buffer, ONE_BLOCK_SIZE, block*ONE_BLOCK_SIZE);
    mutex_unlock(&oeminfo_lock);

    pr_info("%s: block=%d count=%zu result=%d \n", __func__, block, count, result);

    if (result <= 0) {
        pr_err("%s: write fail \n", __func__);
        return -1;
    }

    return (ssize_t)count;
}

static int oeminfo_release(struct inode* inode, struct file* file)
{
    // pr_err("%s: \n",__func__);

    // filp_close(oeminfo_filp,NULL);
    // oeminfo_filp = NULL;

    return 0;
}
static long oeminfo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    char __user *ubuf = (char __user *)arg;

    if (_IOC_TYPE(cmd) != OEMINFO_IOC_MAGIC)
        return -EINVAL;
    if (_IOC_NR(cmd) > OEMINFO_IOC_MAXNR)
        return -EINVAL;

    switch(cmd) {
        case OEMINFO_IOCGETDATA:
            {
                if (copy_to_user(ubuf, blockName, ONE_BLOCK_SIZE))
                return -EFAULT;
                break;
            }
        case OEMINFO_IOCSETDATA:
            {
                memset(blockName, 0, ONE_BLOCK_SIZE);
                if (copy_from_user(blockName, ubuf, ONE_BLOCK_SIZE))
                return -EFAULT;
                break;
            }
        default:
        return -EINVAL;
    }
    return ret;
}

static struct file_operations oeminfo_fops = {
    .owner = THIS_MODULE,
    .open = oeminfo_open,
    .read = oeminfo_read,
    .write = oeminfo_write,
    .release = oeminfo_release,
    .unlocked_ioctl = oeminfo_ioctl,
};

static struct miscdevice oeminfo_device = {
    .minor =MISC_DYNAMIC_MINOR ,
    .name = "oeminfo",
    .fops = &oeminfo_fops,
};

static int __init oeminfo_drv_init(void)
{
    unsigned int ret;

    ret = misc_register(&oeminfo_device);
    if (ret) {
        pr_err("%s: register zeusis oeminfo device fail \n", __func__);
        return ret;
    }

    return 0;
}

static void __exit oeminfo_drv_exit(void)
{
    misc_deregister(&oeminfo_device);
    pr_info("oeminfo exit \n");
}

module_init(oeminfo_drv_init);
module_exit(oeminfo_drv_exit);

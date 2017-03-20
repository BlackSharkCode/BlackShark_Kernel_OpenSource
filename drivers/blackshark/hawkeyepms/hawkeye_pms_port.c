#include "hawkeye_pms_port.h"
#include "hawkeye_pms_compat.h"

struct hawkeyepms_port{
    u8                      binit;//init flag
    wait_queue_head_t       waitq;
};

static struct hawkeyepms_port g_heyepms_port = {0};

//=================================================================//

static const struct file_operations heye_pms_clt_fops = {
    .owner          =   THIS_MODULE,
    .llseek         =   no_llseek,
    .unlocked_ioctl =   hawkeye_pms_clt_ioctl,
    .compat_ioctl   =   hawkeye_pms_clt_compat_ioctl,
    .open           =   hawkeye_pms_clt_open,
    .release        =   hawkeye_pms_clt_close,
};


static struct miscdevice heye_pms_clt_miscdev =
{
    .minor          = MISC_DYNAMIC_MINOR,
    .name           = "heye_pms_clt",
    .fops           = &heye_pms_clt_fops,
    .mode           = S_IRUGO,
};
//=================================================================//

static const struct file_operations heye_pms_svr_fops = {
    .owner          = THIS_MODULE,
    .llseek         = no_llseek,
    .unlocked_ioctl = hawkeye_pms_svr_ioctl,
    .open           = hawkeye_pms_svr_open,
    .release        = hawkeye_pms_svr_close,
    .poll           = hawkeye_pms_svr_poll,
};


static struct miscdevice heye_pms_svr_miscdev =
{
    .minor          = MISC_DYNAMIC_MINOR,
    .name           = "heye_pms_svr",
    .fops           = &heye_pms_svr_fops,
    .mode           = S_IRUGO,
};

//=================================================================//
int hawkeye_pms_pollwait(struct file *file,poll_table *wait)
{
    /*================================================================*/
    /*1.check if bug server already init,if not return erro*/
    /*================================================================*/
    if(!g_heyepms_port.binit){
        HAWKEYE_LOG_ERR("bug manger not init\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.do poll wait*/
    /*================================================================*/
    poll_wait(file,&g_heyepms_port.waitq,wait);
    return 0;
}

int hawkeye_pms_wakeup(void)
{
    /*================================================================*/
    /*1.check if bug server already init,if not return erro*/
    /*================================================================*/
    if(!g_heyepms_port.binit){
        HAWKEYE_LOG_ERR("hawkeye port not init\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.sigle the poll wait*/
    /*================================================================*/
    wake_up_interruptible_all(&g_heyepms_port.waitq);
    return 0;
}
EXPORT_SYMBOL(hawkeye_pms_wakeup);

//=================================================================//
/*  hawkeye import interface */
//=================================================================//
int hawkeye_pms_clt_open(struct inode *inode, struct file *file)
{
    HAWKEYE_LOG_DEBUG("open pms clt\n");
    return 0;
}

int hawkeye_pms_clt_close(struct inode *inode, struct file *file)
{
    HAWKEYE_LOG_DEBUG("close pms clt\n");
    return 0;
}

long hawkeye_pms_clt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret;

    union {
        struct hawkeye_param_func_start funcstart;
        struct hawkeye_param_func_stop funcstop;
    } data;

    preempt_disable_notrace();
    /*================================================================*/
    /*1.get user params*/
    /*================================================================*/
    if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
    {
        HAWKEYE_LOG_ERR("copy from user failed \n");
        ret = -EFAULT;
        goto out;
    }

    /*================================================================*/
    /*2.handle user command*/
    /*================================================================*/
    switch(cmd)
    {
        /*================================================================*/
        /*2.1 recoder fun start time*/
        /*================================================================*/
        case HAWKEYE_START_FUNC:
            {
                ret = hawkeye_pms_start_func(&data.funcstart.func_id);
            }
            break;

        /*================================================================*/
        /*2.2 recoder fun stop time*/
        /*================================================================*/
        case HAWKEYE_STOP_FUNC:
            {
                ret = hawkeye_pms_stop_func(&data.funcstop.func_id);
            }
            break;

       default:
            {
               HAWKEYE_LOG_ERR("unknow cmd=%d\n",cmd);
               ret = -EINVAL;
            }
            break;
    }

    /*================================================================*/
    /*3.revert user params*/
    /*================================================================*/
    if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd))) {
        HAWKEYE_LOG_ERR("copy to user failed \n");
        ret = -EFAULT;
    }
out:
    preempt_enable_notrace();
    return ret;
}

//=================================================================//
/*  hawkeye export interface */
//=================================================================//
int hawkeye_pms_svr_open(struct inode *inode, struct file *file)
{
    HAWKEYE_LOG_DEBUG("junk open pms\n");
    return 0;
}

int hawkeye_pms_svr_close(struct inode *inode, struct file *file)
{
    HAWKEYE_LOG_DEBUG("junk close pms\n");
    return 0;
}

unsigned int hawkeye_pms_svr_poll(struct file *file, poll_table *wait)
{
    unsigned int mask = 0;
    /*================================================================*/
    /*1.poll wait */
    /*================================================================*/
    if(hawkeye_pms_pollwait(file,wait) < 0){
        HAWKEYE_LOG_ERR("poll wait failed\n");
    }
    /*================================================================*/
    /*2.check if have ready msg*/
    /*================================================================*/
    if(!hawkeye_pms_empty()){
        mask = POLLIN | POLLRDNORM;
    }
    HAWKEYE_LOG_DEBUG("mask %d \n",mask);
    return mask;
}
long hawkeye_pms_svr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = -EINVAL;
    union {
        struct hawkeye_param_get_func_count funcount;
        struct hawkeye_param_get_func getfun;
        struct hawkeye_param_clr_func clrfun;
    } data;

    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if (_IOC_SIZE(cmd) > sizeof(data))
    {
        HAWKEYE_LOG_ERR("size %d erro \n",_IOC_SIZE(cmd));
        return -EINVAL;
    }
    /*================================================================*/
    /*2.get user params*/
    /*================================================================*/
    if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
    {
        HAWKEYE_LOG_ERR("copy from user failed \n");
        return -EFAULT;
    }

    /*================================================================*/
    /*3.handle user command*/
    /*================================================================*/
    switch(cmd)
    {
        case HAWKEYE_GET_FUNC_COUNT:
            {
                if(data.funcount.func_flgs){
                    ret = hawkeye_pms_get_total_count_by_funcid(data.funcount.func_id);
                }else{
                   ret = hawkeye_pms_get_total_count();
                }
            }
            break;
        case HAWKEYE_GET_FUNC_INFO:
            {
                if(data.getfun.func_flgs){
                    ret = hawkeye_pms_get_func_by_funcid(data.getfun.func_idx,&data.getfun.func,data.getfun.func_id);
                }else{
                    ret = hawkeye_pms_get_func_by_index(data.getfun.func_idx,&data.getfun.func);
                }
            }
            break;
        case HAWKEYE_CLR_FUNC_INFO:
            {
                ret = hawkeye_pms_clr_all();
            }
            break;

        default:
            {
                HAWKEYE_LOG_ERR("unknow cmd=%d\n",cmd);
                return -EPERM;
            }
    }

    /*================================================================*/
    /*4.revert user params*/
    /*================================================================*/
    if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd))) {
        HAWKEYE_LOG_ERR("copy to user failed \n");
        return -EFAULT;
    }

    return ret;
}

//=================================================================//
//public interface
//=================================================================//
int hawkeye_pms_register(void)
{
    int ret = -EIO;
    memset(&g_heyepms_port, 0, sizeof(struct hawkeyepms_port));
    init_waitqueue_head(&g_heyepms_port.waitq);
    /*================================================================*/
    /*1.register hawkeye pms for client user*/
    /*================================================================*/
    ret = misc_register(&heye_pms_clt_miscdev);
    if(ret){
        HAWKEYE_LOG_ERR("misc register failed\n");
        return ret;
    }

    /*================================================================*/
    /*2.register hawkeye pms for junkserver*/
    /*================================================================*/
    ret = misc_register(&heye_pms_svr_miscdev);
    if(ret){
        HAWKEYE_LOG_ERR("misc register failed\n");
        return ret;
    }
    g_heyepms_port.binit = 1;
    return ret;
}

int hawkeye_pms_unregister(void)
{
    //do nothing
    g_heyepms_port.binit = 0;
    return 0;
}


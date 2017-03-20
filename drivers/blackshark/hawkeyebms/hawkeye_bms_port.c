#include "hawkeye_bms_port.h"
#include "hawkeye_bms_compat.h"
/*================================================================*/
/*hawkeye bug manger info*/
/*================================================================*/

struct hawkeyebms_port{
    u8                      binit;//init flag
    wait_queue_head_t       waitbugq;//poll wait for junk server
    wait_queue_head_t       waiteventq;//poll wait for junk server
};
static struct hawkeyebms_port g_heyebms_port = {0};

//=================================================================//

static const struct file_operations heye_bms_clt_fops = {
    .owner          =   THIS_MODULE,
    .llseek         =   no_llseek,
    .unlocked_ioctl =   hawkeye_bms_clt_ioctl,
    .compat_ioctl   =   hawkeye_bms_clt_compat_ioctl,
    .open           =   hawkeye_bms_clt_open,
    .release        =   hawkeye_bms_clt_close,
};


static struct miscdevice heye_bms_clt_miscdev =
{
    .minor          = MISC_DYNAMIC_MINOR,
    .name           = "heye_bms_clt",
    .fops           = &heye_bms_clt_fops,
    .mode           = S_IRUGO,
};

//=================================================================//

static const struct file_operations heye_bms_svr_fops = {
    .owner          = THIS_MODULE,
    .llseek         = no_llseek,
    .unlocked_ioctl = hawkeye_bms_svr_ioctl,
    .open           = hawkeye_bms_svr_open,
    .release        = hawkeye_bms_svr_close,
    .poll           = hawkeye_bms_svr_poll,
};


static struct miscdevice heye_bms_svr_miscdev =
{
    .minor          = MISC_DYNAMIC_MINOR,
    .name           = "heye_bms_svr",
    .fops           = &heye_bms_svr_fops,
    .mode           = S_IRUGO,
};

//=================================================================//

//=================================================================//

static const struct file_operations heye_ems_clt_fops = {
    .owner          =   THIS_MODULE,
    .llseek         =   no_llseek,
    .unlocked_ioctl =   hawkeye_ems_clt_ioctl,
    .compat_ioctl   =   hawkeye_ems_clt_compat_ioctl,
    .open           =   hawkeye_ems_clt_open,
    .release        =   hawkeye_ems_clt_close,
};


static struct miscdevice heye_ems_clt_miscdev =
{
    .minor          = MISC_DYNAMIC_MINOR,
    .name           = "heye_ems_clt",
    .fops           = &heye_ems_clt_fops,
    .mode           = S_IRUGO,
};
//=================================================================//

static const struct file_operations heye_ems_svr_fops = {
    .owner          = THIS_MODULE,
    .llseek         = no_llseek,
    .unlocked_ioctl = hawkeye_ems_svr_ioctl,
    .open           = hawkeye_ems_svr_open,
    .release        = hawkeye_ems_svr_close,
    .poll           = hawkeye_ems_svr_poll,
};


static struct miscdevice heye_ems_svr_miscdev =
{
    .minor          = MISC_DYNAMIC_MINOR,
    .name           = "heye_ems_svr",
    .fops           = &heye_ems_svr_fops,
    .mode           = S_IRUGO,
};

//=================================================================//
int hawkeye_bms_pollwait(struct file *file,poll_table *wait)
{
    /*================================================================*/
    /*1.check if bug server already init,if not return erro*/
    /*================================================================*/
    if(!g_heyebms_port.binit){
        HAWKEYE_LOG_ERR("hawkeye port not init\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.do poll wait*/
    /*================================================================*/
    poll_wait(file,&g_heyebms_port.waitbugq,wait);
    return 0;
}

int hawkeye_ems_pollwait(struct file *file,poll_table *wait)
{
    /*================================================================*/
    /*1.check if bug server already init,if not return erro*/
    /*================================================================*/
    if(!g_heyebms_port.binit){
        HAWKEYE_LOG_ERR("hawkeye port not init\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.do poll wait*/
    /*================================================================*/
    poll_wait(file,&g_heyebms_port.waiteventq,wait);
    return 0;
}

int hawkeye_bms_wakeup(int msgtype)
{
    /*================================================================*/
    /*1.check if bug server already init,if not return erro*/
    /*================================================================*/
    if(!g_heyebms_port.binit){
        HAWKEYE_LOG_ERR("hawkeye port not init\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.sigle the poll wait*/
    /*================================================================*/
    if(msgtype == MSGTYPE_EVENT){
        wake_up_interruptible_all(&g_heyebms_port.waiteventq);
    }else{
        wake_up_interruptible_all(&g_heyebms_port.waitbugq);
    }
    return 0;
}
EXPORT_SYMBOL(hawkeye_bms_wakeup);

//=================================================================//
/*  hawkeye import interface */
//=================================================================//
int hawkeye_bms_clt_open(struct inode *inode, struct file *file)
{
    int ret = -1;
    long clt_data = 0;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!file){
        HAWKEYE_LOG_ERR("file null\n");
        return -EFAULT;
    }

    /*================================================================*/
    /*2.register default client, need update client name by ioctl*/
    /*================================================================*/
    ret = hawkeye_bms_default_client();
    if(ret < 0){
        HAWKEYE_LOG_ERR("hawkeye_bms_register_client failed\n");
        return -EFAULT;
    }
    clt_data = ret;
    /*================================================================*/
    /*3.save client fd to private data*/
    /*================================================================*/
    file->private_data = (void*)clt_data;
    HAWKEYE_LOG_DEBUG("client_fd=%ld\n",clt_data);
    return 0;
}

int hawkeye_bms_clt_close(struct inode *inode, struct file *file)
{
    long clt_data = 0;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!file){
        HAWKEYE_LOG_ERR("file null\n");
        return -EFAULT;
    }
    /*================================================================*/
    /*2.get client fd from private data*/
    /*================================================================*/
    clt_data = (long)file->private_data;
    HAWKEYE_LOG_DEBUG("client_fd=%ld\n",clt_data);

    /*================================================================*/
    /*3.unreg client by client fd*/
    /*================================================================*/
    return hawkeye_bms_unregister_client((int)clt_data);
}

long hawkeye_bms_clt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = -EINVAL;
    long clt_data = 0;

    union {
        struct hawkeye_param_updateclient updateclt;
        struct hawkeye_param_startmsg msgstart;
        struct hawkeye_param_stopmsg msgstop;
        struct hawkeye_param_printmsg msgprint;
        struct hawkeye_param_confg cfg;
    } data;

    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!file){
        HAWKEYE_LOG_ERR("file null\n");
        return -EFAULT;
    }

    /*================================================================*/
    /*2.get client fd and user params*/
    /*================================================================*/
    clt_data = (long)file->private_data;
    if (_IOC_SIZE(cmd) > sizeof(data))
    {
        HAWKEYE_LOG_ERR("size %d erro \n",_IOC_SIZE(cmd));
        return -EINVAL;
    }
    if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
    {
        HAWKEYE_LOG_ERR("copy from user failed \n");
        return -EFAULT;
    }

    /*================================================================*/
    /*2.handle user command*/
    /*================================================================*/
    switch(cmd)
    {
        /*================================================================*/
        /*2.1 update client info*/
        /*================================================================*/
        case HAWKEYE_UPDATE_CLIENT:
            {
                ret = hawkeye_bms_update_client((int)clt_data,
                    data.updateclt.client_name,
                    data.updateclt.private_string);
                /*update file private data, for close the right one*/
                if(ret != clt_data){
                    file->private_data = (void*)ret;
                }
            }
            break;

        /*================================================================*/
        /*2.2 begin printf msg by client fd and errno*/
        /*================================================================*/
        case HAWKEYE_START_MSG:
            {
                ret = hawkeye_bms_msg_start((int)clt_data,
                    data.msgstart.err_no);
            }
            break;
        /*================================================================*/
        /*2.3 write msg to mem ,and it can be called more than once during start and stop*/
        /*================================================================*/
        case HAWKEYE_PRINT_MSG:
            {
                ret = hawkeye_bms_msg_copy(data.msgprint.msg_id,
                    data.msgprint.msg_str,
                    data.msgprint.msg_len);
            }
            break;
        /*================================================================*/
        /*2.3 stop write msg by msg id*/
        /*================================================================*/
        case HAWKEYE_STOP_MSG:
            {
                ret = hawkeye_bms_msg_stop(data.msgstop.msg_id);
            }
            break;
        /*================================================================*/
        /*2.4 set BMS config*/
        /*================================================================*/
        case HAWKEYE_SET_BMSCFG:
            {
                ret = hawkeye_config_set(MSGTYPE_BUG,&data.cfg.cfg);
            }
            break;
        /*================================================================*/
        /*2.5 get BMS config*/
        /*================================================================*/
        case HAWKEYE_GET_BMSCFG:
            {
                ret = hawkeye_config_get(MSGTYPE_BUG,&data.cfg.cfg);
            }
            break;
       default:
            {
               HAWKEYE_LOG_ERR("unknow cmd=%d\n",cmd);
               return -ENOTTY;
            }
    }

    /*================================================================*/
    /*3.revert user params*/
    /*================================================================*/
    if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd))) {
        HAWKEYE_LOG_ERR("copy to user failed \n");
        return -EFAULT;
    }

    return ret;
}

//=================================================================//
/*  hawkeye export interface */
//=================================================================//
int hawkeye_bms_svr_open(struct inode *inode, struct file *file)
{
    HAWKEYE_LOG_DEBUG("junk open bms\n");
    return 0;
}

int hawkeye_bms_svr_close(struct inode *inode, struct file *file)
{
    /*================================================================*/
    /*1.if there're uprocess msg when junk exit, remove them to ready msg list */
    /*================================================================*/
    hawkeye_msg_unprocess(MSGTYPE_BUG);
    HAWKEYE_LOG_DEBUG("junk close bms\n");
    return 0;
}

unsigned int hawkeye_bms_svr_poll(struct file *file, poll_table *wait)
{
    unsigned int mask = 0;
    /*================================================================*/
    /*1.poll wait */
    /*================================================================*/
    if(hawkeye_bms_pollwait(file,wait) < 0){
        return mask;
    }
    /*================================================================*/
    /*2.check if have ready msg*/
    /*================================================================*/
    if(!hawkeye_msg_empty(MSGTYPE_BUG)){
        mask = POLLIN | POLLRDNORM;
    }
    return mask;
}
long hawkeye_bms_svr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = -EINVAL;
    union {
        struct hawkeye_param_capturemsg msgcap;
        struct hawkeye_param_releasemsg msgrelase;
        struct hawkeye_param_share memshare;
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
        case HAWKEYE_MEM_SHARE:
            {
                /*================================================================*/
                /*3.1 share mem,if success read share fd*/
                /*================================================================*/
                ret = hawkeye_bugfs_share(&data.memshare.mm_sz);
                if(ret < 0){
                    data.memshare.mm_fd = -1;
                    data.memshare.mm_sz = 0;
                    HAWKEYE_LOG_ERR("mem share failed\n");
                }else{
                    data.memshare.mm_fd = ret;
                }
            }
            break;
        case HAWKEYE_MSG_CAPTURE:
            {
                /*================================================================*/
                /*3.2 get ready msg msg id, page id, errno list but not delete from the list*/
                /*================================================================*/
                ret = hawkeye_caputre_msg(MSGTYPE_BUG,&data.msgcap);
            }
            break;

        case HAWKEYE_MSG_RELEASE:
            {
                /*================================================================*/
                /*3.3 release ready msg msg and gc page*/
                /*================================================================*/
                ret = hawkeye_release_msg(MSGTYPE_BUG,data.msgrelase.msg_id);
                if(ret < 0){
                    HAWKEYE_LOG_ERR("msg release %d failed\n",data.msgrelase.msg_id);
                }
            }
            break;
        default:
            {
                HAWKEYE_LOG_ERR("unknow cmd=%d\n",cmd);
                return -ENOTTY;
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
/*  hawkeye import interface */
//=================================================================//
int hawkeye_ems_clt_open(struct inode *inode, struct file *file)
{
    int ret = -1;
    long clt_data = 0;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!file){
        HAWKEYE_LOG_ERR("file null\n");
        return -EFAULT;
    }

    /*================================================================*/
    /*2.register default client, need update client name by ioctl*/
    /*================================================================*/
    ret = hawkeye_bms_default_client();
    if(ret < 0){
        HAWKEYE_LOG_ERR("hawkeye_bms_register_client failed\n");
        return -EFAULT;
    }
    clt_data = ret;
    /*================================================================*/
    /*3.save client fd to private data*/
    /*================================================================*/
    file->private_data = (void*)clt_data;
    HAWKEYE_LOG_DEBUG("client_fd=%ld\n",clt_data);
    return 0;
}

int hawkeye_ems_clt_close(struct inode *inode, struct file *file)
{
    long clt_data = 0;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!file){
        HAWKEYE_LOG_ERR("file null\n");
        return -EFAULT;
    }
    /*================================================================*/
    /*2.get client fd from private data*/
    /*================================================================*/
    clt_data = (long)file->private_data;
    HAWKEYE_LOG_DEBUG("client_fd=%ld\n",clt_data);

    /*================================================================*/
    /*3.unreg client by client fd*/
    /*================================================================*/
    return hawkeye_bms_unregister_client((int)clt_data);
}

long hawkeye_ems_clt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = -EINVAL;
    long clt_data = 0;
    union {
        struct hawkeye_param_notify_event event;
        struct hawkeye_param_confg cfg;
    } data;

    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!file){
        HAWKEYE_LOG_ERR("file null\n");
        return -EFAULT;
    }

    /*================================================================*/
    /*2.get client fd and user params*/
    /*================================================================*/
    clt_data = (long)file->private_data;
    if (_IOC_SIZE(cmd) > sizeof(data))
    {
        HAWKEYE_LOG_ERR("size %d erro \n",_IOC_SIZE(cmd));
        return -EINVAL;
    }
    if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
    {
        HAWKEYE_LOG_ERR("copy from user failed \n");
        return -EFAULT;
    }

    /*================================================================*/
    /*2.handle user command*/
    /*================================================================*/
    switch(cmd)
    {
        /*================================================================*/
        /*2.1 notify junk server*/
        /*================================================================*/
        case HAWKEYE_NOTIFY_EVENT:
            {
                ret = hawkeye_ems_notify((int)clt_data,data.event.event_id,data.event.data,data.event.data_len);
            }
            break;
        /*================================================================*/
        /*2.2 set EMS config*/
        /*================================================================*/
        case HAWKEYE_SET_EMSCFG:
            {
                ret = hawkeye_config_set(MSGTYPE_EVENT,&data.cfg.cfg);
            }
            break;
        /*================================================================*/
        /*2.3 get EMS config*/
        /*================================================================*/
        case HAWKEYE_GET_EMSCFG:
            {
                ret = hawkeye_config_get(MSGTYPE_EVENT,&data.cfg.cfg);
            }
            break;
       default:
            {
               HAWKEYE_LOG_ERR("unknow cmd=%d\n",cmd);
               return -ENOTTY;
            }
    }

    /*================================================================*/
    /*3.revert user params*/
    /*================================================================*/
    if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd))) {
        HAWKEYE_LOG_ERR("copy to user failed \n");
        return -EFAULT;
    }

    return ret;
}

//=================================================================//
/*  hawkeye export interface */
//=================================================================//
int hawkeye_ems_svr_open(struct inode *inode, struct file *file)
{
    HAWKEYE_LOG_DEBUG("junk open ems\n");
    return 0;
}

int hawkeye_ems_svr_close(struct inode *inode, struct file *file)
{
    hawkeye_msg_unprocess(MSGTYPE_EVENT);
    HAWKEYE_LOG_DEBUG("junk close ems\n");
    return 0;
}

unsigned int hawkeye_ems_svr_poll(struct file *file, poll_table *wait)
{
    unsigned int mask = 0;
    /*================================================================*/
    /*1.poll wait */
    /*================================================================*/
    if(hawkeye_ems_pollwait(file,wait) < 0){
        return mask;
    }
    /*================================================================*/
    /*2.check if have ready msg*/
    /*================================================================*/
    if(!hawkeye_msg_empty(MSGTYPE_EVENT)){
        mask = POLLIN | POLLRDNORM;
    }

    return mask;
}

long hawkeye_ems_svr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = -EINVAL;
    union {
        struct hawkeye_param_get_event getevent;
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
        case HAWKEYE_GET_EVENT:
            {
                /*================================================================*/
                /*3.1 share mem,if success read share fd*/
                /*================================================================*/
                ret = hawkeye_ems_get(&data.getevent.event);
            }
            break;
        default:
            {
                HAWKEYE_LOG_ERR("unknow cmd=%d\n",cmd);
                return -ENOTTY;
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
int hawkeye_bms_register(void)
{
    int ret = -EIO;
    memset(&g_heyebms_port, 0, sizeof(struct hawkeyebms_port));
    init_waitqueue_head(&g_heyebms_port.waitbugq);
    init_waitqueue_head(&g_heyebms_port.waiteventq);
    /*================================================================*/
    /*1.register hawkeye bms for client user*/
    /*================================================================*/
    ret = misc_register(&heye_bms_clt_miscdev);
    if(ret){
        HAWKEYE_LOG_ERR("misc register failed\n");
        return ret;
    }
    /*================================================================*/
    /*2.register hawkeye bms for junkserver*/
    /*================================================================*/
    ret = misc_register(&heye_bms_svr_miscdev);
    if(ret){
        HAWKEYE_LOG_ERR("misc register failed\n");
        return ret;
    }
    g_heyebms_port.binit = 1;
    return ret;
}

int hawkeye_bms_unregister(void)
{
    //do nothing
    g_heyebms_port.binit = 0;
    
    return 0;
}

int hawkeye_ems_register(void)
{
    int ret = -EIO;
    /*================================================================*/
    /*1.register hawkeye bms for client user*/
    /*================================================================*/
    ret = misc_register(&heye_ems_clt_miscdev);
    if(ret){
        HAWKEYE_LOG_ERR("misc register failed\n");
        return ret;
    }
    /*================================================================*/
    /*2.register hawkeye bms for junkserver*/
    /*================================================================*/
    ret = misc_register(&heye_ems_svr_miscdev);
    if(ret){
        HAWKEYE_LOG_ERR("misc register failed\n");
        return ret;
    }

    return ret;
}

int hawkeye_ems_unregister(void)
{
    //do nothing
    return 0;
}


#include <linux/compat.h>
#include "hawkeye_bms_port.h"
#include "hawkeye_bms_compat.h"

/*================================================================*/
/*hawkeye_command params for bug report clienta */
/*================================================================*/
/*----------------------------------------------------------------
    hawkeye_compat_param_startmsg:
    command param of HAWKEYE_COMPAT_START_MSG
    err_no: errno of the msg & not null
----------------------------------------------------------------*/
struct hawkeye_compat_param_startmsg{
    compat_int_t err_no;
};
/*----------------------------------------------------------------
    hawkeye_compat_param_stopmsg:
    command param of HAWKEYE_COMPAT_STOP_MSG
    msg_id: value of HAWKEYE_COMPAT_START_MSG return;
----------------------------------------------------------------*/
struct hawkeye_compat_param_stopmsg{
    compat_int_t msg_id;
};
/*----------------------------------------------------------------
    hawkeye_compat_param_printmsg:
    command param of HAWKEYE_COMPAT_PRINT_MSG
    msg_id: value of HAWKEYE_COMPAT_START_MSG return;
    msg_len: strlen of msg info & not lager than HAWKEYE_MAX_BUGMSG_BUFF_LEN
    msg_str: msg info
----------------------------------------------------------------*/
struct hawkeye_compat_param_printmsg{
    compat_int_t msg_id;
    compat_int_t msg_len;
    char msg_str[HAWKEYE_MAX_BUGMSG_BUFF_LEN];
};
/*================================================================*/
/*hawkeye_command params for ems report client */
/*================================================================*/
/*----------------------------------------------------------------
    command param of HAWKEYE_COMPAT_NOTIFY_EVENT
    event_id: event id
----------------------------------------------------------------*/
struct hawkeye_compat_param_notify_event{
    compat_uint_t event_id;
    compat_int_t data_len;
    char data[HAWKEYE_MAX_BUGMSG_BUFF_LEN];

};
/*================================================================*/
/*hawkeye_command for hawkeye bms */
/*================================================================*/
#define HAWKEYE_COMPAT_START_MSG                  _IOWR(HAWKEYE_MAGIC_NUMBER, 1, struct hawkeye_compat_param_startmsg)      
#define HAWKEYE_COMPAT_STOP_MSG                   _IOWR(HAWKEYE_MAGIC_NUMBER, 2, struct hawkeye_compat_param_stopmsg) 
#define HAWKEYE_COMPAT_PRINT_MSG                  _IOWR(HAWKEYE_MAGIC_NUMBER, 3, struct hawkeye_compat_param_printmsg)
/*================================================================*/
/*hawkeye_command for hawkeye ems */
/*================================================================*/
#define HAWKEYE_COMPAT_NOTIFY_EVENT               _IOWR(HAWKEYE_MAGIC_NUMBER, 4, struct hawkeye_compat_param_notify_event)

long hawkeye_bms_clt_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret = -1;
    void __user *arg64 = compat_ptr(arg);
    if(!filp->f_op || !filp->f_op->unlocked_ioctl){
        HAWKEYE_LOG_ERR("filp->f_op || filp->f_op->unlocked_ioctl is null\n");
        return -ENOTTY;
    }

    switch(cmd){
        case HAWKEYE_COMPAT_START_MSG:
            ret = hawkeye_bms_clt_ioctl(filp,HAWKEYE_START_MSG,(unsigned long)arg64);
            break;
        case HAWKEYE_COMPAT_STOP_MSG:
            ret = hawkeye_bms_clt_ioctl(filp,HAWKEYE_STOP_MSG,(unsigned long)arg64);
            break;
        case HAWKEYE_COMPAT_PRINT_MSG:
            ret = hawkeye_bms_clt_ioctl(filp,HAWKEYE_PRINT_MSG,(unsigned long)arg64);
            break;
        default:
            ret = hawkeye_bms_clt_ioctl(filp,cmd,(unsigned long)arg64);
            break;
    }
    return ret;
}

long hawkeye_ems_clt_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret = -1;
    void __user *arg64 = compat_ptr(arg);
    if(!filp->f_op || !filp->f_op->unlocked_ioctl){
        HAWKEYE_LOG_ERR("filp->f_op || filp->f_op->unlocked_ioctl is null\n");
        return -ENOTTY;
    }

    switch(cmd){
        case HAWKEYE_COMPAT_NOTIFY_EVENT:
            ret = hawkeye_ems_clt_ioctl(filp,HAWKEYE_NOTIFY_EVENT,(unsigned long)arg64);
            break;
        default:
            ret = hawkeye_ems_clt_ioctl(filp,cmd,(unsigned long)arg64);
            break;
    }

    return ret;
}


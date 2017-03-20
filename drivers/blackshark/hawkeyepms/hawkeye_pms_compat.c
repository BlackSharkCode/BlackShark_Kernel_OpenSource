#include <linux/compat.h>
#include "hawkeye_pms_port.h"
#include "hawkeye_pms_compat.h"

/*================================================================*/
/*hawkeye_command params for hawkeye pms */
/*================================================================*/
/*----------------------------------------------------------------
    command param of HAWKEYE_START_FUNC
    fun_id: fun id;
----------------------------------------------------------------*/
struct hawkeye_compat_param_func_start{
    compat_uint_t func_id;
};

/*----------------------------------------------------------------
    command param of HAWKEYE_STOP_FUNC
    fun_id: fun id;
----------------------------------------------------------------*/
struct hawkeye_compat_param_func_stop{
    compat_uint_t func_id;
};
/*================================================================*/
/*hawkeye_command for hawkeye pms */
/*================================================================*/
#define HAWKEYE_COMPAT_START_FUNC                  _IOWR(HAWKEYE_MAGIC_NUMBER, 0, struct hawkeye_compat_param_func_start)
#define HAWKEYE_COMPAT_STOP_FUNC                   _IOWR(HAWKEYE_MAGIC_NUMBER, 1, struct hawkeye_compat_param_func_stop)

long hawkeye_pms_clt_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret = -1;
    void __user *arg64 = compat_ptr(arg);
    if(!filp->f_op || !filp->f_op->unlocked_ioctl){
        HAWKEYE_LOG_ERR("filp->f_op || filp->f_op->unlocked_ioctl is null\n");
        return -ENOTTY;
    }

    switch(cmd){
        case HAWKEYE_COMPAT_START_FUNC:
            ret = hawkeye_pms_clt_ioctl(filp,HAWKEYE_START_FUNC,(unsigned long)arg64);
            break;
        case HAWKEYE_COMPAT_STOP_FUNC:
            ret = hawkeye_pms_clt_ioctl(filp,HAWKEYE_STOP_FUNC,(unsigned long)arg64);
            break;
        default:
            ret = hawkeye_pms_clt_ioctl(filp,cmd,(unsigned long)arg64);
            break;
    }

    return ret;
}



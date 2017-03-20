#ifndef __HAWKEYE_PMS_INF_H_
#define __HAWKEYE_PMS_INF_H_
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/msm_ion.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/hawkeye/hawkeye_pub.h>
#include <linux/hawkeye/hawkeye_proc.h>
#include "./../include/hawkeye_priv.h"


/*----------------------------------------------------------------
    funtion:hawkeye_pms_empty
        for check if has oldest page
    params:none
    return:true->empty else not
----------------------------------------------------------------*/
int hawkeye_pms_empty(void);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_get_total_count_by_funcid
        for get total num of fun idfun
    params:idfun->fun id
    return:num else errno
----------------------------------------------------------------*/
int hawkeye_pms_get_total_count_by_funcid(u16 func_id);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_get_func_by_funcid
        for get specify fun by idfun & index
    params:idx->index of fun will get
           pfuns->return  the fun will get
           idfun->fun id
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_get_func_by_funcid(int idx,struct hawkeye_func *pfuns,u16 idfun);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_get_total_count
        for get total num fun
    params:idfun->fun id
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_get_total_count(void);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_get_func_by_index
        for get specify fun by index
    params:idx->index of fun will get
           pfuns->return  the fun will get
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_get_func_by_index(int idx,struct hawkeye_func *pfuns);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_clr_all
        for clear current page
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_clr_all(void);
#endif//__HAWKEYE_PMS_INF_H_
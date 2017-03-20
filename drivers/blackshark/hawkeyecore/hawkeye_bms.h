#ifndef __HAWKEYE_BMS_H__
#define __HAWKEYE_BMS_H__
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
#include "hawkeye_bms_priv.h"
#include "hawkeye_bms_config.h"
#include "hawkeye_bms_govern.h"
/*================================================================*/
/*hawkeye bug manger info*/
/*================================================================*/

struct hawkeye_bug_server{
    u8                      binit;                                      //bug server init flag
    struct timer_list       msg_gctimer;                                //timer for msg gc
};

/*================================================================*/
/*hawkeye bug manger interface*/
/*================================================================*/
/*----------------------------------------------------------------
    funtion:libhawkeye_bms_init
        for init bug manger
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_bms_init(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_bms_uninit
        for uninit bug manger
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_bms_uninit(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_bms_check_by_clientid
        for check if the client id exist
    params:client_id->client id of reigter client
    return:0->not find else find
----------------------------------------------------------------*/
int libhawkeye_bms_check_by_clientid(int client_id);

/*----------------------------------------------------------------
    funtion:libhawkeye_bms_check_by_clientfd
        for check if the client fd exist
    params:client_fd->client fd of reigter client
    return:0->not find else find
----------------------------------------------------------------*/
int libhawkeye_bms_check_by_clientfd(int client_fd);

/*----------------------------------------------------------------
    funtion:libhawkeye_bms_default_client
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/
int libhawkeye_bms_default_client(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_bms_update_client
        for update specify client info
    params:client_fd->hawkeye_bug_register_default_client/hawkeye_bug_register_client return value
           client_name->new client name
           private_data->new client extra info
    return:client fd
----------------------------------------------------------------*/
int libhawkeye_bms_update_client(int client_fd, const char *client_name, const char *private_data);

/*----------------------------------------------------------------
    funtion:libhawkeye_bms_dbg_callback
        for printf client info
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_bms_dbg_callback(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_ems_get
        for printf client info
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_ems_get(struct hawkeye_event *pevent);


/*----------------------------------------------------------------
    funtion:libhawkeye_bms_get_default_clientfd
        for printf client info
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_bms_get_default_clientfd(void);

#endif//_HAWKEYE_CORE_H
#ifndef __HAWKEYE_BMS_INF_H_
#define __HAWKEYE_BMS_INF_H_
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
#include <linux/hawkeye/hawkeye_list.h>
#include "./../include/hawkeye_priv.h"


#define HAWKEYE_DEFAULT_CLIENTFD    (0x00FFE001)
//bit number of mask for client id
#define HAWKEYE_CLIENT_MASK_ID                     (12)
/*----------------------------------------------------------------
    funtion:hawkeye_bms_default_client
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/
void hawkeye_client_mgr_init(void);
/*----------------------------------------------------------------
    funtion:hawkeye_bms_default_client
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/
void hawkeye_client_mgr_uninit(void);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_default_client
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/
int hawkeye_bms_default_client(void);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_update_client
        for update specify client info
    params:client_fd->hawkeye_bug_register_default_client/hawkeye_bug_register_client return value
           client_name->new client name
           private_data->new client extra info
    return:client fd
----------------------------------------------------------------*/
int hawkeye_bms_update_client(int client_fd, const char *client_name, const char *private_data);

/*----------------------------------------------------------------
    funtion:hawkeye_msg_unprocess
        for clean all uprocess msg when junksvr exit
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_msg_unprocess(int msgtype);

/*----------------------------------------------------------------
    funtion:hawkeye_msg_empty
        for check if have ready msg
    params:none
    return:1->empty else not empty
----------------------------------------------------------------*/
int hawkeye_msg_empty(int msgtype);

/*----------------------------------------------------------------
    funtion:hawkeye_config_set
        for update configs
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_config_set(int type,struct hawkeye_config *pcfg);

/*----------------------------------------------------------------
    funtion:hawkeye_config_get
        for get configs
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_config_get(int type,struct hawkeye_config *pcfg);

/*----------------------------------------------------------------
    funtion:hawkeye_bugfs_share
        for get share mem fd
    params:psz->return share mem size
    return:share mem fd else errno
----------------------------------------------------------------*/
int hawkeye_bugfs_share(int *psz);

/*----------------------------------------------------------------
    funtion:hawkeye_caputre_msg
        for get a ready msg and move to process msg list
    params:pgid->return the fst page id of the msg
           errno->return the errno of the msg
    return:msg id
----------------------------------------------------------------*/
int hawkeye_caputre_msg(int msgtype,struct hawkeye_param_capturemsg *pmsg);

/*----------------------------------------------------------------
    funtion:hawkeye_release_msg
        for move the msg from process to idle
    params:msgid->return from hawkeye_caputre_msg
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_release_msg(int msgtype,int msgid);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_dbg_callback
        for printf client info
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_ems_get(struct hawkeye_event *pevent);
#endif//__HAWKEYE_BMS_TEST_H_
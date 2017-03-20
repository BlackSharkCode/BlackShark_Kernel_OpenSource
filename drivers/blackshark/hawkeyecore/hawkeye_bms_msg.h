#ifndef __HAWKEYE_MSG_H__
#define __HAWKEYE_MSG_H__
#include "hawkeye_bms.h"
/*================================================================*/
/*hawkeye bug msg info                                            */
/*================================================================*/
enum hawkeye_errno_pro{
    ERRNOPRO_CLOSE = 0,
    ERRNOPRO_CRITCAL,
    ERRNOPRO_NORMAL,
    ERRNOPRO_WARNING,
    ERRNOPRO_IGNORE,
    ERRNOPRO_MAX,
};

struct hawkeye_bug_msg{
    ktime_t ktm;
    struct hawkeye_msg msg;
    struct list_head node;
};

/*================================================================*/
/*hawkeye bug msg interface                                       */
/*================================================================*/
/*----------------------------------------------------------------
    funtion:libhawkeye_init_msgmgr
        for init msg list & create idle msg
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_init_msgmgr(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_uninit_msgmgr
        for release msg list & other resource
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_uninit_msgmgr(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_create_msg
        for get a idle msg and init it & move to prepare msg list
    params:client_id->client id
           err_no->the errno of this msg
    return:msg id
----------------------------------------------------------------*/
int libhawkeye_create_msg(int client_id,u32 errid, int msgtype);

/*----------------------------------------------------------------
    funtion:libhawkeye_load_msg
        for get a prepare msg and update it's data
    params:msg_id->return from libhawkeye_create_msg
           pbuf,len->the data of this msg will be write to share mem
    return:the num of data have been writen else errno
----------------------------------------------------------------*/
int libhawkeye_load_msg(int msg_id,const char *pbuf, int len);

/*----------------------------------------------------------------
    funtion:libhawkeye_launch_msg
        for get a prepare msg and move it to ready msg list,then wake up junksvr
    params:msg_id->return from libhawkeye_create_msg
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_launch_msg(int msg_id);

/*----------------------------------------------------------------
    funtion:libhawkeye_destory_msg
        for get a process msg and move it to idle msg list
    params:msg_id->return from libhawkeye_create_msg
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_destory_msg(struct hawkeye_bug_msg *pmsg);

/*----------------------------------------------------------------
    funtion:libhawkeye_msg_empty
        for check if have ready msg
    params:none
    return:1->empty else not empty
----------------------------------------------------------------*/
int libhawkeye_msg_empty(int msgtype);

/*----------------------------------------------------------------
    funtion:libhawkeye_caputre_msg
        for get a ready msg and move to process msg list
    params:pgid->return the fst page id of the msg
           errno->return the errno of the msg
    return:msg id
----------------------------------------------------------------*/
int libhawkeye_caputre_msg(int msgtype,struct hawkeye_param_capturemsg *pmsg);

/*----------------------------------------------------------------
    funtion:libhawkeye_release_msg
        for move the msg from process to idle
    params:msgid->return from libhawkeye_caputre_msg
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_release_msg(int msgtype,int msgid);

/*----------------------------------------------------------------
    funtion:libhawkeye_msg_clean
        for clean all prepare msg but not ready when client exit
    params:client_id->return from client register interface
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_msg_clean(int client_id);

/*----------------------------------------------------------------
    funtion:libhawkeye_msg_unprocess
        for clean all uprocess msg when junksvr exit
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_msg_unprocess(int msgtype);

/*----------------------------------------------------------------
    funtion:libhawkeye_msg_gc
        for ontime gc long time unprocess msg
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_msg_gc(int msgtype);

/*----------------------------------------------------------------
    funtion:libhawkeye_msg_dbg
        for printf msg info
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_msg_dbg(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_msg_get_rate
        for get consume rate of idle msg
    params:none
    return:rate
----------------------------------------------------------------*/
int libhawkeye_msg_get_rate(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_alloc_msgs
        for alloc nm of idle msgs
    params:nm number of idle msgs will be alloc
    return:number of idle msg alloc
----------------------------------------------------------------*/
int libhawkeye_alloc_msgs(int nm);

/*----------------------------------------------------------------
    funtion:libhawkeye_free_msgs
        for free nm of idle msgs
    params:nm number of idle msgs will be free
    return:number of idle msg free
----------------------------------------------------------------*/
int libhawkeye_free_msgs(int nm);

/*----------------------------------------------------------------
    funtion:libhawkeye_get_msg_totalcount
        for get current total msg count
    params:none
    return:msg totalcount
----------------------------------------------------------------*/
int libhawkeye_get_msg_totalcount(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_get_msg_idlecount
        for get idle msg count
    params:none
    return:msg idle count
----------------------------------------------------------------*/
int libhawkeye_get_msg_idlecount(void);


#endif

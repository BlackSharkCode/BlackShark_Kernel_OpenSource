#ifndef _HAWKEYE_PUB_H
#define _HAWKEYE_PUB_H
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/hawkeye/hawkeye.h>


/*================================================================*/
/*hawkeye common interface                                        */
/*================================================================*/
/*----------------------------------------------------------------
    funtion:hawkeye_bms_register_client
        to register specify client
    params:client_name->new client name
           private_data->new client extra info
    return:client fd
----------------------------------------------------------------*/
int hawkeye_bms_register_client(const char *client_name, const char *private_data);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_unregister_client 
        to unregister specify client
    params:client_fd->hawkeye_bug_register_default_client/hawkeye_bug_register_client/hawkeye_bug_update_client return value
           private_data->new client extra info
    return:client fd
----------------------------------------------------------------*/
int hawkeye_bms_unregister_client(int client_fd);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_msg_start
        for start a new msg
    params:client_fd->hawkeye_bug_register_default_client/hawkeye_bug_register_client/hawkeye_bug_update_client return value
           errno->tne new msg errno
    return:msg id
----------------------------------------------------------------*/
int hawkeye_bms_msg_start(int client_fd, u32 errno);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_msg_stop
        for stop specify msg
    params:msg_id->hawkeye_bug_msg_start return value
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_msg_stop(int msg_id);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_msg_record
        for format specify msg info to share mem
    params:msg_id->hawkeye_bug_msg_start return value
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_msg_record(int msg_id,const char *fmt, ...);

/*----------------------------------------------------------------
    funtion:prehawkeye_bms_msg_record
        for format specify msg info to share mem
    params:msg_id->hawkeye_bug_msg_start return value
    return:0->success else errno
----------------------------------------------------------------*/
int prehawkeye_bms_msg_record(u32 errno,const char *fmt, ...);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_msg_copy
        for copy specify msg info to share mem
    params:msg_id->hawkeye_bug_msg_start return value
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_msg_copy(int msg_id, const char *buff, int size);

/*================================================================*/
/*hawkeye ems interface */
/*================================================================*/
/*----------------------------------------------------------------
    funtion:hawkeye_ems_notify
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/
int hawkeye_ems_notify(int clientfd, u32 eventid, const char *buff, int size);

/*----------------------------------------------------------------
    funtion:prehawkeye_ems_notify
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/
int prehawkeye_ems_notify(u32 eventid, const char *buff, int size);

/*================================================================*/
/*hawkeye pms interface */
/*================================================================*/
/*----------------------------------------------------------------
    funtion:hawkeye_pms_start_func
        for recoder fun start time
    params:idfun->fun id
    return:id else errno
----------------------------------------------------------------*/
int hawkeye_pms_start_func(u32* func_id);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_stop_func
        for recoder fun stop time
    params:id->hawkeye_pms_start_func return value
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_stop_func(u32* func_id);


#endif //_HAWKEYE_PUB_H
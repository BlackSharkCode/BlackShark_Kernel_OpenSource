#ifndef _HAWKEYE_BMS_PORT_
#define _HAWKEYE_BMS_PORT_
#include "hawkeye_bms_inf.h"

/*================================================================*/
/*hawkeye bms device interface*/
/*================================================================*/
/*----------------------------------------------------------------
    funtion:hawkeye_bms_register
        for reigster bms devices of hawkeye
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_register(void);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_unregister
        for unreigster bms devices of hawkeye
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_unregister(void);

/*----------------------------------------------------------------
    funtion:hawkeye_ems_register
        for reigster ems devices of hawkeye
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_ems_register(void);

/*----------------------------------------------------------------
    funtion:hawkeye_ems_unregister
        for unreigster ems devices of hawkeye
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_ems_unregister(void);

//=================================================================//
//heye bms dev for client
//=================================================================//
/*----------------------------------------------------------------
    funtion:hawkeye_bms_clt_open
        call back function when heye_bms_clt is opend
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_clt_open(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_clt_close
        call back function when heye_bms_clt is closed
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_clt_close(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_clt_ioctl
        call back function when ioctl on heye_bms_clt
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
long hawkeye_bms_clt_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

//=================================================================//
//heye bms dev for junkserver
//=================================================================//
/*----------------------------------------------------------------
    funtion:hawkeye_bms_svr_open
        call back function when heye_bms_svr is opend
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_svr_open(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_unregister
        call back function when heye_bms_svr is closed
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_svr_close(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_unregister
        call back function when select on heye_bms_svr
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
unsigned int hawkeye_bms_svr_poll(struct file *file, poll_table *wait);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_svr_ioctl
        call back function when ioctl on heye_bms_svr
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
long hawkeye_bms_svr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

//=================================================================//
//heye ems dev for client
//=================================================================//
/*----------------------------------------------------------------
    funtion:hawkeye_ems_clt_open
        call back function when heye_ems_clt is opend
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_ems_clt_open(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_ems_clt_close
        call back function when heye_ems_clt is closed
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_ems_clt_close(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_ems_clt_ioctl
        call back function when ioctl on heye_ems_clt
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
long hawkeye_ems_clt_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

//=================================================================//
//heye bms dev for junkserver
//=================================================================//
/*----------------------------------------------------------------
    funtion:hawkeye_ems_svr_open
        call back function when heye_ems_svr is opend
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_ems_svr_open(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_ems_unregister
        call back function when heye_ems_svr is closed
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_ems_svr_close(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_ems_unregister
        call back function when select on heye_ems_svr
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
unsigned int hawkeye_ems_svr_poll(struct file *file, poll_table *wait);

/*----------------------------------------------------------------
    funtion:hawkeye_ems_svr_ioctl
        call back function when ioctl on heye_ems_svr
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
long hawkeye_ems_svr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_pollwait
        for wait for ready msg
    params:referce to poll_wait
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_pollwait(struct file *file,poll_table *wait);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_pollwait
        for wait for ready msg
    params:referce to poll_wait
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_ems_pollwait(struct file *file,poll_table *wait);

#endif //_HAWKEYE_BMS_PORT_

#ifndef _HAWKEYE_PMS_PORT_H_
#define _HAWKEYE_PMS_PORT_H_
#include "hawkeye_pms_inf.h"

/*================================================================*/
/*hawkeye performance device interface                                   */
/*================================================================*/
/*----------------------------------------------------------------
    funtion:hawkeye_pms_register
        for reigster pms devices of hawkeye
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_register(void);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_register
        for unreigster pms devices of hawkeye
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_unregister(void);

//=================================================================//
//heye pms dev for client 
//=================================================================//
/*----------------------------------------------------------------
    funtion:hawkeye_pms_clt_open
        call back function when heye_pms_clt is opend
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_clt_open(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_clt_close
        call back function when heye_pms_clt is closed
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_clt_close(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_clt_ioctl
        call back function when ioctl on heye_pms_clt
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
long hawkeye_pms_clt_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

//=================================================================//
//heye pms dev for junkserver
//=================================================================//
/*----------------------------------------------------------------
    funtion:hawkeye_pms_svr_open
        call back function when heye_pms_svr is opend
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_svr_open(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_unregister
        call back function when heye_pms_svr is closed
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_svr_close(struct inode *inode, struct file *file);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_unregister
        call back function when select on heye_pms_svr
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
unsigned int hawkeye_pms_svr_poll(struct file *file, poll_table *wait);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_svr_ioctl
        call back function when ioctl on heye_pms_svr
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
long hawkeye_pms_svr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif //_HAWKEYE_PMS_PORT_H_


#ifndef __HAWKEYE_BMS_PRIV_H__
#define __HAWKEYE_BMS_PRIV_H__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/hawkeye/hawkeye_pub.h>
#include <linux/hawkeye/hawkeye_proc.h>
#include <linux/hawkeye/hawkeye_list.h>
#include "hawkeye_priv.h"

//max share mem size
#define HAWKEYE_FS_SIZE                             (65536) //64*1024
//max idle msg num
#define HAWKEYE_MSG_MAX_COUNT                       (1024)
//default client name
#define HAWKEYE_DEFAULT_NAME                        ("UNDEFINE_%d")
//max time of gc timer
#define HAWKEYE_MAX_GCTIMER                         (1800*HZ)//0.5h
//max time of gc
#define HAWKEYE_MAX_GCTIME                          (3600000)//1h

//bit number of mask for client id
#define HAWKEYE_CLIENT_MASK_ID                     (12)
//mask of client id
#define HAWKEYE_CLIENT_MASK_INFO                   ((1<<HAWKEYE_CLIENT_MASK_ID)-1)
//check client fd
#define HAWKEYE_VERIFY_CLIENTFD(fd)                ((fd & HAWKEYE_CLIENT_MASK_INFO) - ((~(fd >>HAWKEYE_CLIENT_MASK_ID) & HAWKEYE_CLIENT_MASK_INFO)))
//conver client fd to client id
#define HAWKEYE_CONVERT_CLIENTFD2ID(fd)            (fd & HAWKEYE_CLIENT_MASK_INFO)
//conver client id to client fd
#define HAWKEYE_CONVERT_CLIENTID2FD(id)            ((id & HAWKEYE_CLIENT_MASK_INFO) | ((~(id & HAWKEYE_CLIENT_MASK_INFO)) & HAWKEYE_CLIENT_MASK_INFO)<<HAWKEYE_CLIENT_MASK_ID)
//get errno level
#define HAWKEYE_ERRNO_PRO(err)                     (((err)&0x1C00)>>10)
//get errid mode
#define HAWKEYE_ERRNO_MOD(err)                     ((err)>>20)
//check errid level
#define HAWKEYE_PRO_VALID(p)                       ((p) >= ERRNOPRO_CLOSE && (p) < ERRNOPRO_MAX)
//check mode
#define HAWKEYE_MODE_VALID(m)                      ((m) >= 0 && (m)< HAWKEYE_MAX_CONFIG)
#endif //_HAWKEYE_BMS_PRIV_H
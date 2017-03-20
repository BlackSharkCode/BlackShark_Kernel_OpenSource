#ifndef __HAWKEYE_PMS_H__
#define __HAWKEYE_PMS_H__
#include <linux/hash.h>
#include <linux/cpu.h>
#include <linux/ctype.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/sched/rt.h>
#include <linux/hawkeye/hawkeye_pub.h>
#include <linux/hawkeye/hawkeye_proc.h>
#include "hawkeye_fifo.h"
#include "hawkeye_priv.h"

//max profile of every page
#define HAWKEYE_PROFILES_PER_PAGE                   (1024)
//max page size
#define HAWKEYE_PAGE_SIZE                           (1<<3)//must be 2^n
//water mask of page
#define HAWKEYE_PAGE_WATER_MARK                     (HAWKEYE_PAGE_SIZE - 2)
//hash key bit size
#define HAWKEYE_KEYSIZE                             (7)
//hash max head count
#define HAWKEYE_MAXKEY                              ((1<<HAWKEYE_KEYSIZE))

//msg type
enum hawkeye_pms_msg_type{
    HAWKEYE_PMS_MSG_START = 0,  //the msg of func start
    HAWKEYE_PMS_MSG_STOP,       //the msg of func stop
};

//profile page state
enum hawkeye_pms_page_state{
    HAWKEYE_PMS_PAGE_IDLE = 0,  //the page is idle for write
    HAWKEYE_PMS_PAGE_USED,      //the page is now used for wiritting
    HAWKEYE_PMS_PAGE_DIRTY,     //the page is now write finished
    HAWKEYE_PMS_PAGE_UNDEF,
};
/*================================================================*/
/*hawkeye profile msg info*/
/*================================================================*/
struct hawkeye_profile_msg{
    struct hawkeye_func func;
    u16 dynamic_id;
    u8  beacon;
    u8  reserve;
};
/*================================================================*/
/*hawkeye profile info*/
/*================================================================*/
struct hawkeye_profile {
    struct hlist_node node;
    struct hawkeye_profile_msg msg;
};
/*================================================================*/
/*hawkeye profile page info*/
/*================================================================*/
struct hawkeye_profile_page {
    int index;
    enum hawkeye_pms_page_state flag;//idle,used,dirty
    struct hlist_head hash_tbl[HAWKEYE_MAXKEY];
    struct hawkeye_profile records[HAWKEYE_PROFILES_PER_PAGE];
};

/*================================================================*/
/*hawkeye profile stat info*/
/*================================================================*/
struct hawkeye_profile_stat {
    int flag;
    int current_id;
    int oldest_id;
    struct hawkeye_profile_page pages[HAWKEYE_PAGE_SIZE];
};
/*================================================================*/
/*hawkeye performance interface*/
/*================================================================*/
/*----------------------------------------------------------------
    funtion:libhawkeye_pms_init
        for init pms
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_pms_init(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_pms_uninit
        for uninit pms
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_pms_uninit(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_pms_get_total_count_by_funcid
        for get total num of fun idfun
    params:idfun->fun id
    return:num else errno
----------------------------------------------------------------*/
int libhawkeye_pms_get_total_count_by_funcid(u16 func_id);

/*----------------------------------------------------------------
    funtion:libhawkeye_pms_get_func_by_funcid
        for get specify fun by idfun & index
    params:idx->index of fun will get
           pfuns->return  the fun will get
           idfun->fun id
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_pms_get_func_by_funcid(int idx,struct hawkeye_func *pfuns,u16 idfun);

/*----------------------------------------------------------------
    funtion:libhawkeye_pms_get_total_count
        for get total num fun
    params:idfun->fun id
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_pms_get_total_count(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_pms_get_func_by_index
        for get specify fun by index
    params:idx->index of fun will get
           pfuns->return  the fun will get
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_pms_get_func_by_index(int idx,struct hawkeye_func *pfuns);

/*----------------------------------------------------------------
    funtion:libhawkeye_pms_clr_all
        for clear current page
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_pms_clr_all(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_pms_empty
        for check if has oldest page
    params:none
    return:1->empty else not
----------------------------------------------------------------*/
int libhawkeye_pms_empty(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_pms_dbg_callback
        for printf msg info
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_pms_dbg_callback(void);
#endif//__HAWKEYE_PMS_H_
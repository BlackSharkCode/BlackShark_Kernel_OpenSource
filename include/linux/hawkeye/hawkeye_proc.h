#ifndef __HAWKEYE_PROC_H__
#define __HAWKEYE_PROC_H__

#include <linux/hawkeye/hawkeye.h>

#define HAWKEYE_MAX_PREMSG          (512)
/*----------------------------------------------------------------
pre hawkeye msg:store msg before hawkeye mode install
----------------------------------------------------------------*/
enum{
    PREMSGLST_UNIT = 0,
    PREMSGLST_INIT,
    PREMSGLST_CLOSE,
};

struct prehawkeye_msg{
    int msg_type;//0:bug 1:event
    int msg_len;//msg data len
    int err_no;//define notify error number
    ktime_t create_tm;//create time
    char msg_data[HAWKEYE_MAX_BUGMSG_BUFF_LEN];
};

struct prehawkeye_msg_mgr{
    unsigned int count;
    spinlock_t spin_lock;
    struct prehawkeye_msg premsg[HAWKEYE_MAX_PREMSG];
};

struct hawkeye_bug_client{
    int     client_id;//define notify error number
    char    client_name[HAWKEYE_MAX_CLIENT_NAME_LEN];//initial clent name string
    char*   private_data;//string for client self define
    struct  list_head node;//list node
};

struct hawkeye_bms_operations{
    //hawkeye config ops
    int (*config_set)(int,struct hawkeye_config*);
    int (*config_get)(int,struct hawkeye_config*);

    //hawkeye client ops
    int (*default_client)(void);
    int (*update_client)(int,const char*,const char*);
    int (*register_client)(const char*,const char*);
    int (*unregister_client)(int);

    //hawkeye msg ops
    int (*msg_start)(int, u32);
    int (*msg_copy)(int, const char*,int);
    int (*msg_stop)(int);
    int (*msg_empty)(int);
    int (*msg_unprocess)(int);

    //hawkeye ems ops
    int (*ems_notify)(int,u32,const char*,int);
    int (*ems_get)(struct hawkeye_event*);

    //hawkeye svr ops
    int (*bugfs_share)(int*);
    int (*msg_capture)(int,struct hawkeye_param_capturemsg*);
    int (*msg_release)(int,int);
};


struct hawkeye_pms_operations{
    //hawkeye pms config ops
    int (*config_set)(int,struct hawkeye_config*);
    int (*config_get)(int,struct hawkeye_config*);

    //hawkeye fun ops
    int (*fun_start)(u32*);
    int (*fun_stop)(u32*);
    int (*fun_empty)(void);

    //hawkeye svr ops
    int (*fun_count)(void);
    int (*fun_count_by_id)(u16);
    int (*fun_info_by_idx)(int,struct hawkeye_func*);
    int (*fun_info_by_id)(int,struct hawkeye_func*,u16);
    int (*fun_clr)(void);
};


#endif //__HAWKEYE_PROC_H__
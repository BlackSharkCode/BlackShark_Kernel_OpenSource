#include "hawkeye_bms_inf.h"


struct hawkeye_bms_operations g_heyebms_ops = {
        //hawkeye config ops
        NULL,//int (*config_set)(int,struct hawkeye_config*);
        NULL,// (*config_get)(int,struct hawkeye_config*);
    
        //hawkeye client ops
        NULL,//int (*default_client)(void);
        NULL,//int (*update_client)(int,const char*,const char*);
        NULL,//int (*register_client)(const char*,const char*);
        NULL,//int (*unregister_client)(int);
    
        //hawkeye msg ops
        NULL,//int (*msg_start)(int, u32);
        NULL,//int (*msg_copy)(int, const char*,int);
        NULL,//int (*msg_stop)(int);
        NULL,//int (*msg_empty)(int);
        NULL,//int (*msg_unprocess)(int);
    
        //hawkeye ems ops
        NULL,//int (*ems_notify)(int,u32,const char*,int);
        NULL,//int (*ems_get)(struct hawkeye_event*);
    
        //hawkeye svr ops
        NULL,//int (*bugfs_share)(int*);
        NULL,//int (*msg_capture)(int,struct hawkeye_param_capturemsg*);
        NULL,//int (*msg_release)(int,int);
};
EXPORT_SYMBOL(g_heyebms_ops);

struct hawkeye_list g_heyeclient;
EXPORT_SYMBOL(g_heyeclient);

struct prehawkeye_msg_mgr g_heyepremsg;
EXPORT_SYMBOL(g_heyepremsg);


static atomic_t g_pre_state = ATOMIC_INIT(PREMSGLST_UNIT);
static atomic_t g_client_id = ATOMIC_INIT(0);

static void __prehawkeye_msg_mgr_init(void);
static void __prehawkeye_msg_mgr_uninit(void);
static void __prehawkeye_msg_mgr_reset(void);

/*================================================================*/
int hawkeye_get_client_id(void)
{
    return (atomic_inc_return(&g_client_id))%(1<<HAWKEYE_CLIENT_MASK_ID);
}
EXPORT_SYMBOL(hawkeye_get_client_id);

void prehawkeye_msg_close(void)
{//when hawkeye core is insmode,set prehawkeye state to close
    if(atomic_read(&g_pre_state) == PREMSGLST_INIT){
        atomic_set(&g_pre_state,PREMSGLST_CLOSE);
        __prehawkeye_msg_mgr_reset();
    }
}
EXPORT_SYMBOL(prehawkeye_msg_close);

void prehawkeye_msg_open(void)
{
    //when hawkeye core mode is uninstalled, set prehawkeye state to open
    if(atomic_read(&g_pre_state) == PREMSGLST_CLOSE){
        atomic_set(&g_pre_state,PREMSGLST_INIT);
        __prehawkeye_msg_mgr_reset();
    }
}
EXPORT_SYMBOL(prehawkeye_msg_open);
/*================================================================*/


/*================================================================*/
static void __prehawkeye_msg_mgr_reset(void)
{
    unsigned long f = 0;
    spin_lock_irqsave(&(g_heyepremsg.spin_lock), f);
    g_heyepremsg.count = 0;
    memset(&g_heyepremsg.premsg,0,sizeof(g_heyepremsg.premsg));
    spin_unlock_irqrestore(&(g_heyepremsg.spin_lock), f);
}

static void __prehawkeye_msg_mgr_init(void)
{
    spin_lock_init(&(g_heyepremsg.spin_lock));
    __prehawkeye_msg_mgr_reset();
}

static void __prehawkeye_msg_mgr_uninit(void)
{
    __prehawkeye_msg_mgr_reset();
}

static int __prehawkeye_get_valid_msg(void)
{
    int msgid = -1;
    unsigned long f = 0;
    spin_lock_irqsave(&(g_heyepremsg.spin_lock), f);
    if(g_heyepremsg.count < HAWKEYE_MAX_PREMSG){
        msgid = g_heyepremsg.count;
        ++g_heyepremsg.count;
    }
    spin_unlock_irqrestore(&(g_heyepremsg.spin_lock), f);
    return msgid;
}

static int __prehawkeye_create_msg(u32 errid,int msgtype)
{
    int msgid = 0;
    struct prehawkeye_msg*pmsg = NULL;
    /*================================================================*/
    /*1.if hawkeye is start,no need premsg list*/
    /*================================================================*/
    if(atomic_read(&g_pre_state) == PREMSGLST_CLOSE){
        HAWKEYE_LOG_ERR("load pre msg but is already close\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.if hawkeye is not start,and premsg list not init,then init it*/
    /*================================================================*/
    if(atomic_read(&g_pre_state) == PREMSGLST_UNIT){
        atomic_set(&g_pre_state,PREMSGLST_INIT);
        __prehawkeye_msg_mgr_init();
        HAWKEYE_LOG_INFO("load pre msg & init\n");
    }
    /*================================================================*/
    /*1.create pre msg & update total count*/
    /*================================================================*/
    msgid = __prehawkeye_get_valid_msg();
    if(msgid < 0 || msgid >= HAWKEYE_MAX_PREMSG){
        HAWKEYE_LOG_ERR("alloc msg failed\n");
        return -ENOSPC;
    }
    /*================================================================*/
    /*2.update pre msg content*/
    /*================================================================*/
    pmsg = &g_heyepremsg.premsg[msgid];
    pmsg->create_tm = ktime_get_boottime();
    pmsg->err_no = errid;
    pmsg->msg_type = msgtype;
    return msgid;
}

static int __prehawkeye_copy_msg(int msgid,const char *pbuf, int len)
{
    int size = 0;
    struct prehawkeye_msg*pmsg = NULL;
    if(msgid < 0||msgid >= HAWKEYE_MAX_PREMSG || !pbuf){
        HAWKEYE_LOG_ERR("copy pre msg invalid param\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*1.get pre msg by msgid*/
    /*================================================================*/
    pmsg = &g_heyepremsg.premsg[msgid];
    /*================================================================*/
    /*2.copy msg data*/
    /*================================================================*/
    if(pbuf && len > 0){
        size = HAWKEYE_MIN(len,(HAWKEYE_MAX_BUGMSG_BUFF_LEN - pmsg->msg_len));
        if(size > 0){
            memcpy(&pmsg->msg_len, pbuf, size);
            pmsg->msg_len += size;
        }
    }
    return size;
}

static int __prehawkeye_stop_msg(int msgid)
{
    return 0;
}
/*----------------------------------------------------------------
    funtion:hawkeye_client_mgr_init
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/
void hawkeye_client_mgr_init(void)
{
    HAWKEYE_LST_INIT(&g_heyeclient);
    __prehawkeye_msg_mgr_init();
}
/*----------------------------------------------------------------
    funtion:hawkeye_client_mgr_uninit
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/

void hawkeye_client_mgr_uninit(void)
{
    __prehawkeye_msg_mgr_uninit();
    HAWKEYE_LST_UNINIT(&g_heyeclient,struct hawkeye_bug_client,HAWKEYE_CLIENT_FREE);
}


/*----------------------------------------------------------------
    funtion:hawkeye_bms_default_client
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/
int hawkeye_bms_default_client(void)

{
    int ret = 0;
    if(g_heyebms_ops.default_client){
       ret = g_heyebms_ops.default_client();
    }else{
        ret = HAWKEYE_DEFAULT_CLIENTFD;//return default client fd
    }
    return ret;
}

/*----------------------------------------------------------------
    funtion:hawkeye_bms_update_client
        for update specify client info
    params:client_fd->hawkeye_bug_register_default_client/hawkeye_bug_register_client return value
           client_name->new client name
           private_data->new client extra info
    return:client fd
----------------------------------------------------------------*/
int hawkeye_bms_update_client(int client_fd, const char *client_name, const char *private_data)
{
    int ret = client_fd;
    if(g_heyebms_ops.update_client){
       ret = g_heyebms_ops.update_client(client_fd,client_name,private_data);
    }
    return ret;
}

/*----------------------------------------------------------------
    funtion:hawkeye_msg_unprocess
        for clean all uprocess msg when junksvr exit
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_msg_unprocess(int msgtype)
{
    int ret = 0;
    if(g_heyebms_ops.msg_unprocess){
       ret = g_heyebms_ops.msg_unprocess(msgtype);
    }
    return ret;
}

/*----------------------------------------------------------------
    funtion:hawkeye_bms_register_client
        to register specify client
    params:client_name->new client name
           private_data->new client extra info
    return:client fd
----------------------------------------------------------------*/
int hawkeye_bms_register_client(const char *client_name, const char *private_data)
{
    int ret = 0;
    if(g_heyebms_ops.register_client){
       ret = g_heyebms_ops.register_client(client_name,private_data);
    }else{
       ret = HAWKEYE_DEFAULT_CLIENTFD;//return default client fd
    }
    HAWKEYE_LOG_INFO("reigster ret=%d,ops=%p\n",ret,g_heyebms_ops.register_client);
    return ret;
}
EXPORT_SYMBOL(hawkeye_bms_register_client);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_unregister_client 
        to unregister specify client
    params:client_fd->hawkeye_bug_register_default_client/hawkeye_bug_register_client/hawkeye_bug_update_client return value
           private_data->new client extra info
    return:client fd
----------------------------------------------------------------*/
int hawkeye_bms_unregister_client(int client_fd)
{
    int ret = 0;
    if(client_fd == HAWKEYE_DEFAULT_CLIENTFD){
        /*not unregister default client*/
        return ret;
    }
    if(g_heyebms_ops.unregister_client){
       ret = g_heyebms_ops.unregister_client(client_fd); 
    }
    HAWKEYE_LOG_INFO("unreigster ret=%d,ops=%p\n",ret,g_heyebms_ops.unregister_client);
    return ret;
}
EXPORT_SYMBOL(hawkeye_bms_unregister_client);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_msg_start
        for start a new msg
    params:client_fd->hawkeye_bug_register_default_client/hawkeye_bug_register_client/hawkeye_bug_update_client return value
           errno->tne new msg errno
    return:msg id
----------------------------------------------------------------*/
int hawkeye_bms_msg_start(int client_fd, u32 errno)
{
    int ret = 0;
    if(g_heyebms_ops.msg_start){
       ret = g_heyebms_ops.msg_start(client_fd,errno);
    }else{
        //handle prehawkeye msg
        ret = __prehawkeye_create_msg(errno,MSGTYPE_BUG);
    }
    HAWKEYE_LOG_INFO("start ret=%d,ops=%p,fd=%d,errno=%d\n",ret,g_heyebms_ops.msg_start,client_fd,errno);
    return ret;
}
EXPORT_SYMBOL(hawkeye_bms_msg_start);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_msg_stop
        for stop specify msg
    params:msg_id->hawkeye_bug_msg_start return value
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_msg_stop(int msg_id)
{
    int ret = 0;
    if(msg_id < HAWKEYE_MAX_PREMSG){
        //prehawkeye msg
        ret = __prehawkeye_stop_msg(msg_id);
    }else{
        if(g_heyebms_ops.msg_stop){
           ret = g_heyebms_ops.msg_stop(msg_id);
        }
    }
    HAWKEYE_LOG_INFO("stop ret=%d,ops=%p,msgid=%d\n",ret,g_heyebms_ops.msg_stop,msg_id);
    return ret;
}
EXPORT_SYMBOL(hawkeye_bms_msg_stop);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_msg_record
        for format specify msg info to share mem
    params:msg_id->hawkeye_bug_msg_start return value
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_msg_record(int msg_id,const char *fmt, ...)
{
    int ret = 0;
    int len = 0;
    va_list args;
    char logbuf[HAWKEYE_MAX_BUGMSG_BUFF_LEN] = {0};
    /*================================================================*/
    /*1.formate msg*/
    /*================================================================*/
    va_start(args, fmt);
    len = vsnprintf(logbuf, HAWKEYE_MAX_BUGMSG_BUFF_LEN, fmt, args);
    va_end(args);
    
    if(msg_id < HAWKEYE_MAX_PREMSG){
        //prehawkeye msg
        ret = __prehawkeye_copy_msg(msg_id,logbuf,len);
    }else{
        if(g_heyebms_ops.msg_copy && len > 0){
           ret = g_heyebms_ops.msg_copy(msg_id,logbuf,len); 
        }
    }
    HAWKEYE_LOG_INFO("record ret=%d,ops=%p,msgid=%d\n",ret,g_heyebms_ops.msg_copy,msg_id);
    return ret;
}
EXPORT_SYMBOL(hawkeye_bms_msg_record);

/*----------------------------------------------------------------
    funtion:hawkeye_bms_msg_copy
        for copy specify msg info to share mem
    params:msg_id->hawkeye_bug_msg_start return value
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_bms_msg_copy(int msg_id, const char *buff, int size)
{
    int ret = 0;
    if(msg_id < HAWKEYE_MAX_PREMSG){
        //prehawkeye msg
        ret = __prehawkeye_copy_msg(msg_id,buff,size);
    }else{
        if(g_heyebms_ops.msg_copy){
           ret = g_heyebms_ops.msg_copy(msg_id,buff,size); 
        }
    }
    HAWKEYE_LOG_INFO("copy ret=%d,ops=%p,msgid=%d\n",ret,g_heyebms_ops.msg_copy,msg_id);
    return ret;
}
EXPORT_SYMBOL(hawkeye_bms_msg_copy);

/*----------------------------------------------------------------
    funtion:prehawkeye_bms_msg_record
        for format specify msg info to share mem
    params:msg_id->hawkeye_bug_msg_start return value
    return:0->success else errno
----------------------------------------------------------------*/
int prehawkeye_bms_msg_record(u32 errid,const char *fmt, ...)
{
    int ret = -1;
    int len = 0;
    va_list args;
    char logbuf[HAWKEYE_MAX_BUGMSG_BUFF_LEN] = {0};
    /*================================================================*/
    /*1.formate msg*/
    /*================================================================*/
    va_start(args, fmt);
    len = vsnprintf(logbuf, HAWKEYE_MAX_BUGMSG_BUFF_LEN, fmt, args);
    va_end(args);

    /*================================================================*/
    /*3.write msg to mem if need*/
    /*================================================================*/
    if(len > 0){
        ret = __prehawkeye_create_msg(errid,MSGTYPE_BUG);
        if(ret >= 0){
            __prehawkeye_copy_msg(ret,logbuf, len);
            __prehawkeye_stop_msg(ret);
        }
    }else{
        HAWKEYE_LOG_ERR("vsnprintf failed %d\n",len);
    }
    HAWKEYE_LOG_INFO("pre msg record ret=%d\n",ret);
    return 0;
}
EXPORT_SYMBOL(prehawkeye_bms_msg_record);
/*----------------------------------------------------------------
    funtion:hawkeye_ems_notify
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/
int hawkeye_ems_notify(int clientfd, u32 eventid, const char *buff, int size)
{
    int ret = 0;
    if(g_heyebms_ops.ems_notify){
       ret = g_heyebms_ops.ems_notify(clientfd,eventid,buff,size);
    }else{
        //handle prehawkeye msg
        ret = __prehawkeye_create_msg(eventid,MSGTYPE_EVENT);
        if(ret >= 0){
            __prehawkeye_copy_msg(ret,buff, size);
            __prehawkeye_stop_msg(ret);
        }
    }
    HAWKEYE_LOG_INFO("notify ret=%d,ops=%p\n",ret,g_heyebms_ops.ems_notify);
    return ret;
}
EXPORT_SYMBOL(hawkeye_ems_notify);

/*----------------------------------------------------------------
    funtion:prehawkeye_ems_notify
        for register client by default info
    params:none
    return:client fd
----------------------------------------------------------------*/
int prehawkeye_ems_notify(u32 eventid, const char *buff, int size)
{
    int ret = -EINVAL;
    ret = __prehawkeye_create_msg(eventid,MSGTYPE_EVENT);
    if(ret >= 0){
        __prehawkeye_copy_msg(ret,buff, size);
        __prehawkeye_stop_msg(ret);
    }
    HAWKEYE_LOG_ERR("prehawkeye_ems_notify eventid=0X%X,msgid=%d\n",eventid,ret);
    return 0;
}
EXPORT_SYMBOL(prehawkeye_ems_notify);

/*----------------------------------------------------------------
    funtion:hawkeye_msg_empty
        for check if have ready msg
    params:none
    return:1->empty else not empty
----------------------------------------------------------------*/
int hawkeye_msg_empty(int msgtype)
{
    int ret = 1;//default is empty
    if(g_heyebms_ops.msg_empty){
        ret = g_heyebms_ops.msg_empty(msgtype);
    }
    HAWKEYE_LOG_INFO("empty ret=%d,ops=%p\n",ret,g_heyebms_ops.msg_empty);
    return ret;
}

/*----------------------------------------------------------------
    funtion:hawkeye_config_set
        for update configs
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_config_set(int type,struct hawkeye_config *pcfg)
{
    int ret = 0;
    if(g_heyebms_ops.config_set){
        ret = g_heyebms_ops.config_set(type,pcfg);
    }
    HAWKEYE_LOG_INFO("set ret=%d,ops=%p\n",ret,g_heyebms_ops.config_set);
    return ret;
}

/*----------------------------------------------------------------
    funtion:hawkeye_config_get
        for get configs
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_config_get(int type,struct hawkeye_config *pcfg)
{
    int ret = 0;
    if(g_heyebms_ops.config_get){
        ret = g_heyebms_ops.config_get(type,pcfg);
    }
    HAWKEYE_LOG_INFO("get ret=%d,ops=%p\n",ret,g_heyebms_ops.config_get);
    return ret;
}
/*----------------------------------------------------------------
    funtion:hawkeye_bugfs_share
        for get share mem fd
    params:psz->return share mem size
    return:share mem fd else errno
----------------------------------------------------------------*/
int hawkeye_bugfs_share(int *psz)
{
    int ret = 0;
    if(g_heyebms_ops.bugfs_share){
        ret = g_heyebms_ops.bugfs_share(psz);
    }
    HAWKEYE_LOG_INFO("share ret=%d,ops=%p\n",ret,g_heyebms_ops.bugfs_share);
    return ret;
}

/*----------------------------------------------------------------
    funtion:hawkeye_caputre_msg
        for get a ready msg and move to process msg list
    params:pgid->return the fst page id of the msg
           errno->return the errno of the msg
    return:msg id
----------------------------------------------------------------*/
int hawkeye_caputre_msg(int msgtype,struct hawkeye_param_capturemsg *pmsg)
{
    int ret = 0;
    if(g_heyebms_ops.msg_capture){
        ret = g_heyebms_ops.msg_capture(msgtype,pmsg);
    }
    HAWKEYE_LOG_INFO("capture ret=%d,ops=%p\n",ret,g_heyebms_ops.msg_capture);
    return ret;
}

/*----------------------------------------------------------------
    funtion:hawkeye_release_msg
        for move the msg from process to idle
    params:msgid->return from hawkeye_caputre_msg
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_release_msg(int msgtype,int msgid)
{
    int ret = 0;
    if(g_heyebms_ops.msg_release){
        ret = g_heyebms_ops.msg_release(msgtype,msgid);
    }
    HAWKEYE_LOG_INFO("release ret=%d,ops=%p\n",ret,g_heyebms_ops.msg_release);
    return ret;
}
/*----------------------------------------------------------------
    funtion:hawkeye_bms_dbg_callback
        for printf client info
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_ems_get(struct hawkeye_event *pevent)
{
    int ret = 0;
    if(g_heyebms_ops.ems_get){
        ret = g_heyebms_ops.ems_get(pevent);
    }
    HAWKEYE_LOG_INFO("ems get ret=%d,ops=%p\n",ret,g_heyebms_ops.ems_get);
    return ret;
}


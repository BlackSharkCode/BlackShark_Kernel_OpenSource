#include <asm/div64.h>
#include "hawkeye_bms_fs.h"
#include "hawkeye_bms_msg.h"
#include "hawkeye_bms_govern.h"

#define HAWKEYE_MSG_FREE(pmsg)\
    do{\
        if(pmsg){\
            HAWKEYE_LOG_DEBUG("HAWKEYE_MSG_FREE %d\n",pmsg->msg.msg_id);\
            kzfree(pmsg);\
        }\
    }while(0)

#define HAWKEYE_MSG_TIMECHK(id,val) (ktime_to_ms(ktime_sub(ktime_get_boottime(),(id))) >= (val))
/*==============================
 client:
 idle----[start]----->prepare
 prepare---[stop]---->ready

 junkserver:
 ready---[capture]--->process
 process--[release]-->idle
================================*/
struct hawkeye_msg_manger{
    struct hawkeye_list lst_idlemsg;
    struct hawkeye_list lst_preparemsg;
    struct hawkeye_list lst_bugreadymsg[ERRNOPRO_MAX];//0->
    struct hawkeye_list lst_eventreadymsg[ERRNOPRO_MAX];//0->
    struct hawkeye_list lst_bugprocessmsg;
    struct hawkeye_list lst_eventprocessmsg;
};

static struct hawkeye_msg_manger g_msgmgr;
static atomic_t g_msg_id = ATOMIC_INIT(2048);//0-2048 for prehawkeye msg
static atomic_t g_consume_count = ATOMIC_INIT(0);
static atomic_t g_total_count = ATOMIC_INIT(0);

extern int hawkeye_bms_wakeup(int);
extern void prehawkeye_msg_close(void);
extern void prehawkeye_msg_open(void);

extern struct prehawkeye_msg_mgr g_heyepremsg;

/*================================================================*/
/*private interface*/
/*================================================================*/
static inline int __hawkeye_get_msg_id(void)
{
    return atomic_inc_return(&g_msg_id);
}

static inline void __hawkeye_show_lsts(void)
{
    HAWKEYE_LOG_DEBUG("#### Idle[%d],Perpare[%d],BMS[C=%d,N=%d,W=%d,I=%d,P=%d],EMS[C=%d,N=%d,W=%d,I=%d,P=%d]\n",
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_idlemsg),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_preparemsg),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_bugreadymsg[0]),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_bugreadymsg[1]),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_bugreadymsg[2]),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_bugreadymsg[3]),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_bugprocessmsg),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_eventreadymsg[0]),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_eventreadymsg[1]),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_eventreadymsg[2]),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_eventreadymsg[3]),
    HAWKEYE_LST_COUNT(&g_msgmgr.lst_eventprocessmsg));

}

static inline int __hawkeye_msg_clr(struct hawkeye_bug_msg *pmsg)
{
    if(!pmsg){
        return -EINVAL;
    }
    memset(pmsg,0,sizeof(struct hawkeye_bug_msg));
    pmsg->msg.clt_id = -1;
    pmsg->msg.msg_id = -1;
    pmsg->msg.page_id = 0;
    pmsg->msg.data_type = MSGDATATYPE_SHORT;
    pmsg->msg.msg_type = MSGTYPE_BUG;
    return 0;
}

static inline struct hawkeye_bug_msg* __hawkeye_capture_idle_msg(void)
{
    int total_count = 0;
    int cousum_count = 0;
    struct hawkeye_bug_msg *pmsg = NULL;
    total_count = atomic_read(&g_total_count);
    /*================================================================*/
    /*1.get a idle msg*/
    /*================================================================*/
    pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_idlemsg,struct hawkeye_bug_msg,msg.msg_id,-1,HAWKEYE_LST_NUMCMP);
    if(pmsg){
        cousum_count = atomic_add_return(1,&g_consume_count);
        HAWKEYE_LOG_DEBUG("#### capture cosumecount=%d,totalcount=%d\n",cousum_count,total_count);
        return pmsg;
    }

    if(total_count + 1 > HAWKEYE_MSG_MAX_COUNT){
        HAWKEYE_LOG_ERR("current msg total count=%d larger than allow",total_count);
        return NULL;
    }
    if(in_interrupt()){
        pmsg = kzalloc(sizeof(struct hawkeye_bug_msg), GFP_ATOMIC);
    }else{
        pmsg = kzalloc(sizeof(struct hawkeye_bug_msg), GFP_KERNEL);
    }
    if(NULL == pmsg){
        HAWKEYE_LOG_ERR("msg malloc failed");
        return NULL;
    }
    cousum_count = atomic_add_return(1,&g_consume_count);
    total_count = atomic_add_return(1,&g_total_count);
    HAWKEYE_LOG_DEBUG("#### alloc totalcount=%d,cosumecount=%d\n",total_count,cousum_count);
    HAWKEYE_LOG_DEBUG("#### wake up govern task\n");
    libhawkeye_govern_wakeup();
    return pmsg;
}

static inline void __hawkeye_release_idle_msg(struct hawkeye_bug_msg* pmsg)
{
    int total_count = 0;
    int cousum_count = 0;
    if(!pmsg){
        HAWKEYE_LOG_ERR("pmsg NULL\n");
        return ;
    }
    __hawkeye_msg_clr(pmsg);
    total_count = atomic_read(&g_total_count);
    cousum_count = atomic_sub_return(1,&g_consume_count);
    HAWKEYE_LOG_DEBUG("#### release cosumecount=%d,totalcount=%d\n",cousum_count,total_count);
    HAWKEYE_LIST_ADD(&g_msgmgr.lst_idlemsg,pmsg);
    return ;
}

int libprehawkeye_lunch_msg(void)
{
    int i = 0;
    int pro = 0;
    int size = 0;
    unsigned long f = 0;
    struct hawkeye_bug_msg* premsg = NULL;
    int cltfd = libhawkeye_bms_get_default_clientfd();

    spin_lock_irqsave(&(g_heyepremsg.spin_lock), f);
    for(i = 0; i < g_heyepremsg.count; ++i){
        premsg = __hawkeye_capture_idle_msg();
        if(!premsg){
            break;
        }
        premsg->msg.clt_id = cltfd;
        premsg->msg.msg_id = __hawkeye_get_msg_id();
        premsg->msg.err_no = g_heyepremsg.premsg[i].err_no;
        premsg->msg.msg_type = g_heyepremsg.premsg[i].msg_type;
        premsg->ktm = g_heyepremsg.premsg[i].create_tm;
        premsg->msg.tm_create = ktime_to_ns(premsg->ktm);//jiffies_to_msecs(jiffies);
        premsg->msg.tm_kernel = current_kernel_time().tv_sec;
        premsg->msg.data_len = g_heyepremsg.premsg[i].msg_len;
        size = HAWKEYE_MIN(g_heyepremsg.premsg[i].msg_len,HAWKEYE_MAX_BUGMSG_BUFF_LEN);
        if(size > 0){
            memcpy(premsg->msg.msg_data,g_heyepremsg.premsg[i].msg_data,size);
        }
        HAWKEYE_LOG_DEBUG("msg_id=%d,msg_type=%d, page_id=%d,clt_id=%d\n", premsg->msg_id,premsg->msg_type,premsg->page_id,premsg->clt_id);
        /*================================================================*/
        /*2.add this msg to ready msg list*/
        /*================================================================*/
        pro = HAWKEYE_ERRNO_PRO(premsg->msg.err_no);
        if(pro < ERRNOPRO_CRITCAL || pro >= ERRNOPRO_MAX){
            HAWKEYE_LOG_ERR("msg id=%d,msg_type=%d, errid=0X%X, leve=%d invalid\n",premsg->msg.msg_id,premsg->msg.msg_type,premsg->msg.err_no,pro);
            libhawkeye_destory_msg(premsg);
            return -EINVAL;
        }
        if(premsg->msg.msg_type == MSGTYPE_EVENT){
            HAWKEYE_LIST_ADD(&g_msgmgr.lst_eventreadymsg[pro - 1],premsg);
        }else{
            HAWKEYE_LIST_ADD(&g_msgmgr.lst_bugreadymsg[pro - 1],premsg);
        }

    }
    spin_unlock_irqrestore(&(g_heyepremsg.spin_lock), f);

    return 0;
}

/*================================================================*/
/*public interface*/
/*================================================================*/
int libhawkeye_init_msgmgr(void)
{
    int pro = 0;
    /*================================================================*/
    /*1.init basic msg manger info and msg list*/
    /*================================================================*/
    memset(&g_msgmgr, 0, sizeof(struct hawkeye_msg_manger));
    HAWKEYE_LST_INIT(&g_msgmgr.lst_idlemsg);
    HAWKEYE_LST_INIT(&g_msgmgr.lst_preparemsg);
    HAWKEYE_LST_INIT(&g_msgmgr.lst_bugprocessmsg);
    HAWKEYE_LST_INIT(&g_msgmgr.lst_eventprocessmsg);
    for(pro = ERRNOPRO_CRITCAL; pro < ERRNOPRO_MAX; ++pro){
        HAWKEYE_LST_INIT(&g_msgmgr.lst_bugreadymsg[pro - 1]);
    }
    for(pro = ERRNOPRO_CRITCAL; pro < ERRNOPRO_MAX; ++pro){
        HAWKEYE_LST_INIT(&g_msgmgr.lst_eventreadymsg[pro - 1]);
    }
    /*================================================================*/
    /*2.init govern*/
    /*================================================================*/
    libhawkeye_govern_init();
    /*================================================================*/
    /*3.handle premsg*/
    /*================================================================*/
    prehawkeye_msg_close();//disable prehawkeye msg handle any more
    libprehawkeye_lunch_msg();//get all prehawkeye msg and handle them
    return 0;
}

int libhawkeye_uninit_msgmgr(void)
{
    int pro = 0;
    /*================================================================*/
    /*1.stop govern*/
    /*================================================================*/
    prehawkeye_msg_open();//reenable prehawkeye msg handle
    libhawkeye_govern_unint();
    /*================================================================*/
    /*2.clear msg list*/
    /*================================================================*/
    HAWKEYE_LST_UNINIT(&g_msgmgr.lst_idlemsg,struct hawkeye_bug_msg,HAWKEYE_MSG_FREE);
    HAWKEYE_LST_UNINIT(&g_msgmgr.lst_preparemsg,struct hawkeye_bug_msg,HAWKEYE_MSG_FREE);
    HAWKEYE_LST_UNINIT(&g_msgmgr.lst_bugprocessmsg,struct hawkeye_bug_msg,HAWKEYE_MSG_FREE);
    HAWKEYE_LST_UNINIT(&g_msgmgr.lst_eventprocessmsg,struct hawkeye_bug_msg,HAWKEYE_MSG_FREE);
    for(pro = ERRNOPRO_CRITCAL; pro < ERRNOPRO_MAX; ++pro){
        HAWKEYE_LST_UNINIT(&g_msgmgr.lst_bugreadymsg[pro - 1],struct hawkeye_bug_msg,HAWKEYE_MSG_FREE);
    }
    for(pro = ERRNOPRO_CRITCAL; pro < ERRNOPRO_MAX; ++pro){
        HAWKEYE_LST_UNINIT(&g_msgmgr.lst_eventreadymsg[pro - 1],struct hawkeye_bug_msg,HAWKEYE_MSG_FREE);
    }
    return 0;
}

//get a vaild msg from idle list and add it to prepare list
int libhawkeye_create_msg(int clientid,u32 errid,int msgtype)//move msg node form idle to ready
{
    int msgid = -1;
    struct hawkeye_bug_msg *pmsg = NULL;
    /*================================================================*/
    /*1.get a idle msg*/
    /*================================================================*/
    pmsg = __hawkeye_capture_idle_msg();
    if(!pmsg){
        HAWKEYE_LOG_ERR("no idle msg failed\n");
        return -ENOSPC;
    }
    /*================================================================*/
    /*2.init msg with new msg id,client id,errid and page_id must be 0*/
    /*================================================================*/
    msgid = __hawkeye_get_msg_id();
    pmsg->ktm = ktime_get_boottime();
    pmsg->msg.clt_id = clientid;
    pmsg->msg.msg_type = msgtype;
    pmsg->msg.msg_id = msgid;
    pmsg->msg.err_no = errid;
    pmsg->msg.cur_tid = current->pid;
    pmsg->msg.tm_create = ktime_to_ns(pmsg->ktm);//jiffies_to_msecs(jiffies);
    pmsg->msg.tm_kernel = current_kernel_time().tv_sec;
    HAWKEYE_LOG_DEBUG("msg_id=%d,msg_type=%d,page_id=%d,clt_id=%d,tm_create=%lld,tm_kernel=%lld\n",
        pmsg->msg.msg_id,pmsg->msg.msg_type,pmsg->msg.page_id,pmsg->msg.clt_id,pmsg->msg.tm_create,pmsg->msg.tm_kernel);
    /*================================================================*/
    /*3.add this msg to prepare msg list and return msgid*/
    /*================================================================*/
    HAWKEYE_LIST_ADD(&g_msgmgr.lst_preparemsg,pmsg);

    __hawkeye_show_lsts();

    return msgid;
}

//get a valid msg from prepare list and update it's page id
int libhawkeye_load_msg(int msgid,const char *pbuf, int len)
{
    int ret = -1;
    int size;
    struct hawkeye_bug_msg *pmsg = NULL;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if( msgid < 0 || !pbuf || len <= 0){
        HAWKEYE_LOG_ERR("invalid params msgid=%d,pbuf=%p,len=%d\n", msgid,pbuf,len);
        return -EINVAL;
    }

    /*================================================================*/
    /*2.get a prepare msg by msgid*/
    /*================================================================*/
    pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_preparemsg,struct hawkeye_bug_msg,msg.msg_id,msgid,HAWKEYE_LST_NUMCMP);
    if(!pmsg){
        HAWKEYE_LOG_ERR("prepare msg %d not exit\n",msgid);
        return -ENOSPC;
    }

    /*================================================================*/
    /*3.write msg to mem*/
    /*================================================================*/
    if(pmsg->msg.data_type == MSGDATATYPE_SHORT){
        size = pmsg->msg.data_len + len;
        if(likely(size <= HAWKEYE_MAX_BUGMSG_BUFF_LEN)){
            memcpy(&pmsg->msg.msg_data[pmsg->msg.data_len], pbuf, len);
            pmsg->msg.data_len = size;
        }else{
            pmsg->msg.data_type = MSGDATATYPE_LONG;
            if(pmsg->msg.data_len > 0){
                ret = libhawkeye_bugfs_write(&pmsg->msg.page_id, pmsg->msg.msg_data, pmsg->msg.data_len);
                if(ret < 0){
                    HAWKEYE_LOG_ERR("move data to fs failed, ret=%d\n",ret);
                }
                pmsg->msg.data_len = 0;
            }
        }
    }

    if(pmsg->msg.data_type == MSGDATATYPE_LONG){
        ret = libhawkeye_bugfs_write(&pmsg->msg.page_id, pbuf, len);
        if(ret < 0){
            HAWKEYE_LOG_ERR("bug fs write failed, ret=%d\n",ret);
        }
    }
    HAWKEYE_LOG_DEBUG("msg_id=%d,msg_type=%d,page_id=%d,clt_id=%d\n", pmsg->msg.msg_id,pmsg->msg.msg_type,pmsg->msg.page_id,pmsg->msg.clt_id);
    /*================================================================*/
    /*4.add back msg to prepare msg list*/
    /*================================================================*/
    HAWKEYE_LIST_ADD(&g_msgmgr.lst_preparemsg,pmsg);

    __hawkeye_show_lsts();
    return 0;
}

//get a valid msg from prepare list and put it to ready list
int libhawkeye_launch_msg(int msgid)
{
    int pro = 0;
    int msgtype = 0;
    struct hawkeye_bug_msg *pmsg = NULL;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if( msgid < 0 ){
        HAWKEYE_LOG_ERR("invalid params msgid=%d\n",msgid);
        return -EINVAL;
    }

    /*================================================================*/
    /*2.get a prepare msg by msgid*/
    /*================================================================*/
    pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_preparemsg,struct hawkeye_bug_msg,msg.msg_id,msgid,HAWKEYE_LST_NUMCMP);
    if(!pmsg){
        HAWKEYE_LOG_ERR("invalid msg id =%d\n",msgid);
        return -EINVAL;
    }
    HAWKEYE_LOG_DEBUG("msg_id=%d,msg_type=%d, page_id=%d,clt_id=%d\n", pmsg->msg.msg_id,pmsg->msg.msg_type,pmsg->msg.page_id,pmsg->msg.clt_id);
    /*================================================================*/
    /*3.add this msg to ready msg list*/
    /*================================================================*/
    msgtype = pmsg->msg.msg_type;
    pro = HAWKEYE_ERRNO_PRO(pmsg->msg.err_no);
    if(pro < ERRNOPRO_CRITCAL || pro >= ERRNOPRO_MAX){
        HAWKEYE_LOG_ERR("msg id=%d,msg_type=%d, errid=0X%X, leve=%d invalid\n",msgid,pmsg->msg.msg_type,pmsg->msg.err_no,pro);
        libhawkeye_destory_msg(pmsg);
        return -EINVAL;
    }
    if(pmsg->msg.msg_type == MSGTYPE_EVENT){
        HAWKEYE_LIST_ADD(&g_msgmgr.lst_eventreadymsg[pro - 1],pmsg);
    }else{
        HAWKEYE_LIST_ADD(&g_msgmgr.lst_bugreadymsg[pro - 1],pmsg);
    }

    /*================================================================*/
    /*4.wake poll wait*/
    /*================================================================*/
    hawkeye_bms_wakeup(msgtype);
    __hawkeye_show_lsts();
    return 0;
}

//get a valid msg from process list and put it to idle list
int libhawkeye_destory_msg(struct hawkeye_bug_msg *pmsg)//move msg node from busy to idle list
{
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(NULL == pmsg){
        HAWKEYE_LOG_ERR("invalid params\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.gc current msg page id*/
    /*================================================================*/
    HAWKEYE_LOG_DEBUG("msg_id=%d, page_id=%d,clt_id=%d\n", pmsg->msg.msg_id,pmsg->msg.page_id,pmsg->msg.clt_id);
    if(pmsg->msg.data_type == MSGDATATYPE_LONG){
        libhawkeye_bugfs_gc(pmsg->msg.page_id);
    }
    /*================================================================*/
    /*3.add this msg to idle msg list*/
    /*================================================================*/
    __hawkeye_release_idle_msg(pmsg);
    __hawkeye_show_lsts();
    return 0;
}

//check ready msg list
int libhawkeye_msg_empty(int msgtype)
{
    int pro = 0;
    int ret = 0;
    /*================================================================*/
    /*1.check ready list if empty, 0 means not empty*/
    /*================================================================*/
    if(msgtype == MSGTYPE_EVENT){
        for(pro = ERRNOPRO_CRITCAL; pro < ERRNOPRO_MAX; ++pro){
            ret = HAWKEYE_LST_ISEMPTY(&g_msgmgr.lst_eventreadymsg[pro - 1]);
            if(!ret){
                break;
            }
        }
    }else{
        for(pro = ERRNOPRO_CRITCAL; pro < ERRNOPRO_MAX; ++pro){
            ret = HAWKEYE_LST_ISEMPTY(&g_msgmgr.lst_bugreadymsg[pro - 1]);
            if(!ret){
                break;
            }
        }
    }
    return ret;
}

//get a valid msg from ready list and put it to process list
int libhawkeye_caputre_msg(int msgtype,struct hawkeye_param_capturemsg *pcpmsg)
{
    int pro = 0;
    int msgid = -1;
    struct hawkeye_bug_msg *pmsg = NULL;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!pcpmsg){
        HAWKEYE_LOG_ERR("invalid params\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.get first ready msg*/
    /*================================================================*/
    for(pro = ERRNOPRO_CRITCAL; pro < ERRNOPRO_MAX; ++pro){
        if(msgtype == MSGTYPE_EVENT){
            pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_eventreadymsg[pro - 1],struct hawkeye_bug_msg,msg.msg_id,msgid,HAWKEYE_LST_NUMCMP);
        }else{
            pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_bugreadymsg[pro - 1],struct hawkeye_bug_msg,msg.msg_id,msgid,HAWKEYE_LST_NUMCMP);
        }
        if(pmsg){
            break;
        }
    }
    if(NULL == pmsg){
        HAWKEYE_LOG_DEBUG("no msg type=%d\n",msgtype);
        return -ENOSPC;
    }
    /*================================================================*/
    /*3.check the level of msg*/
    /*================================================================*/
    if(!libhawkeye_config_check(msgtype,pmsg->msg.err_no)){
        libhawkeye_destory_msg(pmsg);
        return -EPERM;
    }
    /*================================================================*/
    /*4.get page id and msg id if need*/
    /*================================================================*/
    msgid = pmsg->msg.msg_id;
    memcpy(&(pcpmsg->msg),&(pmsg->msg), sizeof(struct hawkeye_msg));
    /*================================================================*/
    /*5.add this msg to process msg list and return msg id*/
    /*================================================================*/
    if(msgtype == MSGTYPE_EVENT){
        HAWKEYE_LIST_ADD(&g_msgmgr.lst_eventprocessmsg,pmsg);
    }else{
        HAWKEYE_LIST_ADD(&g_msgmgr.lst_bugprocessmsg,pmsg);
    }
    __hawkeye_show_lsts();
    return msgid;
}

int libhawkeye_release_msg(int msgtype,int msgid)
{
    struct hawkeye_bug_msg *pmsg = NULL;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if( msgid < 0 ){
        HAWKEYE_LOG_ERR("invalid params msgid=%d\n",msgid);
        return -EINVAL;
    }
    /*================================================================*/
    /*2.get process msg by msg id*/
    /*================================================================*/
    if(msgtype == MSGTYPE_EVENT){
        pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_eventprocessmsg,struct hawkeye_bug_msg,msg.msg_id,msgid,HAWKEYE_LST_NUMCMP);
    }else{
        pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_bugprocessmsg,struct hawkeye_bug_msg,msg.msg_id,msgid,HAWKEYE_LST_NUMCMP);
    }

    if(NULL == pmsg){
        HAWKEYE_LOG_ERR("get msg_id=%d type=%d,failed\n",msgid,msgtype);
        return -EINVAL;
    }
    /*================================================================*/
    /*3.recycling msg*/
    /*================================================================*/
    libhawkeye_destory_msg(pmsg);
    return 0;
}


int libhawkeye_msg_clean(int clientid)
{
    int pro = 0;
    int count = 0;
    struct hawkeye_bug_msg *pmsg = NULL;
    do{
        ++count;
        if(count > HAWKEYE_MSG_MAX_COUNT){
            HAWKEYE_LOG_ERR("clean client id %d msg failed\n",clientid);
            break;
        }

        /*================================================================*/
        /*1.get prepare msg by client id*/
        /*================================================================*/
        pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_preparemsg,struct hawkeye_bug_msg,msg.clt_id,clientid,HAWKEYE_LST_NUMCMP);
        if(!pmsg){
            break;
        }
        HAWKEYE_LOG_DEBUG("msg_id=%d, page_id=%d,clt_id=%d\n", pmsg->msg.msg_id,pmsg->msg.page_id,pmsg->msg.clt_id);

        /*================================================================*/
        /*2.if page id is valid ,add prepare msg to ready msg list else add to idle list*/
        /*================================================================*/
        if(pmsg->msg.page_id <= 0){
            __hawkeye_release_idle_msg(pmsg);
            continue;
        }
        pro = HAWKEYE_ERRNO_PRO(pmsg->msg.err_no);
        if(pro < ERRNOPRO_CRITCAL || pro >= ERRNOPRO_MAX){
            HAWKEYE_LOG_ERR("msg id=%d, errid=%d, leve=%d invalid\n",pmsg->msg.msg_id,pmsg->msg.err_no,pro);
            libhawkeye_destory_msg(pmsg);
        }else{
            if(pmsg->msg.msg_type == MSGTYPE_EVENT){
                HAWKEYE_LIST_ADD(&g_msgmgr.lst_eventreadymsg[pro - 1],pmsg);
            }else{
                HAWKEYE_LIST_ADD(&g_msgmgr.lst_bugreadymsg[pro - 1],pmsg);
            }
        }
        /*================================================================*/
        /*3.wake poll wait*/
        /*================================================================*/
        hawkeye_bms_wakeup(pmsg->msg.msg_type);

    }while(pmsg);

    HAWKEYE_LOG_DEBUG("clt_id=%d\n",clientid);
    __hawkeye_show_lsts();
    return 0;
}


int libhawkeye_msg_unprocess(int msgtype)
{
    int pro = 0;
    int count = 0;
    struct hawkeye_bug_msg *pmsg = NULL;

    do{
        ++count;
        if(count > HAWKEYE_MSG_MAX_COUNT){
            HAWKEYE_LOG_ERR("handle unprocess msg failed\n");
            break;
        }

        /*================================================================*/
        /*1.get unprocess msg */
        /*================================================================*/
        if(msgtype == MSGTYPE_EVENT){
            pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_eventprocessmsg,struct hawkeye_bug_msg,msg.msg_id,-1,HAWKEYE_LST_NUMCMP);
        }else{
            pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_bugprocessmsg,struct hawkeye_bug_msg,msg.msg_id,-1,HAWKEYE_LST_NUMCMP);
        }
        if(!pmsg){
            break;
        }
        HAWKEYE_LOG_DEBUG("msgid=%d,pageid=%d,clientid=%d\n",pmsg->msg.msg_id,pmsg->msg.page_id,pmsg->msg.clt_id);

        /*================================================================*/
        /*2.add unprocess msg to ready msg list*/
        /*================================================================*/
        pro = HAWKEYE_ERRNO_PRO(pmsg->msg.err_no);
        if(pro < ERRNOPRO_CRITCAL || pro >= ERRNOPRO_MAX){
            HAWKEYE_LOG_ERR("msg id=%d, errid=%d, leve=%d invalid\n",pmsg->msg.msg_id,pmsg->msg.err_no,pro);
            libhawkeye_destory_msg(pmsg);
        }else{
            if(pmsg->msg.msg_type == MSGTYPE_EVENT){
                HAWKEYE_LIST_ADD(&g_msgmgr.lst_eventreadymsg[pro - 1],pmsg);
            }else{
                HAWKEYE_LIST_ADD(&g_msgmgr.lst_bugreadymsg[pro - 1],pmsg);
            }
        }

        /*================================================================*/
        /*3.wake poll wait*/
        /*================================================================*/
        hawkeye_bms_wakeup(pmsg->msg.msg_type);

    }while(pmsg);

    __hawkeye_show_lsts();
    return 0;
}


int libhawkeye_msg_gc(int msgtype)
{
    struct hawkeye_bug_msg *pmsg = NULL;
    /*================================================================*/
    /*1.get long time unhandle msg from prepare list*/
    /*================================================================*/
    if(msgtype == MSGTYPE_EVENT){
        HAWKEYE_LOG_DEBUG("gc type=%d,count=%d\n",msgtype,HAWKEYE_LST_COUNT(&g_msgmgr.lst_eventprocessmsg));
        pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_eventprocessmsg,struct hawkeye_bug_msg,ktm,HAWKEYE_MAX_GCTIME,HAWKEYE_MSG_TIMECHK);
    }else{
        HAWKEYE_LOG_DEBUG("gc type=%d,count=%d\n",msgtype,HAWKEYE_LST_COUNT(&g_msgmgr.lst_bugprocessmsg));
        pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_bugprocessmsg,struct hawkeye_bug_msg,ktm,HAWKEYE_MAX_GCTIME,HAWKEYE_MSG_TIMECHK);
    }
    if(!pmsg){
       return -EINVAL;
    }
    HAWKEYE_LOG_DEBUG("msg_id=%d, page_id=%d,clt_id=%d\n", pmsg->msg.msg_id,pmsg->msg.page_id,pmsg->msg.clt_id);
    /*================================================================*/
    /*2.check the client of the msg if exist*/
    /*================================================================*/
    if(libhawkeye_bms_check_by_clientid(pmsg->msg.clt_id) > 0){
        return -EINVAL;
    }
    /*================================================================*/
    /*3.put long time unhandle msg to idle list*/
    /*================================================================*/
    __hawkeye_release_idle_msg(pmsg);
    __hawkeye_show_lsts();
    return 0;
}

int libhawkeye_msg_dbg(void)
{
    __hawkeye_show_lsts();
    return 0;
}


int libhawkeye_msg_get_rate(void)
{
    int rate = 0;
    int count = 0;
    static s64 msec = 0;
    s64 curcnt = 0;
    s64 cursec = ktime_to_ms(ktime_get_boottime());
    if(0 == msec){
        msec = cursec;
    }
    if(cursec != msec){
        count = atomic_read(&g_consume_count);
		curcnt = 1000 * count;

		curcnt = div_s64(curcnt,(cursec - msec));
        rate = (int)(curcnt);

        HAWKEYE_LOG_INFO("####count=%d,rate=%d,cursec=%lld,msec=%lld\n",count,rate,cursec,msec);

        msec = cursec;
        atomic_sub_return(count,&g_consume_count);
    }
    return rate;
}

int libhawkeye_alloc_msgs(int nm)
{
    int count = 0;
    int total_count = 0;
    struct hawkeye_bug_msg *pmsg = NULL;
    do{
        pmsg = kzalloc(sizeof(struct hawkeye_bug_msg), GFP_KERNEL);
        if(!pmsg){
            HAWKEYE_LOG_ERR("msg malloc failed");
            break;
        }
        __hawkeye_msg_clr(pmsg);
        HAWKEYE_LIST_ADD(&g_msgmgr.lst_idlemsg,pmsg);
    }while(++count < nm);

    total_count = atomic_add_return(count,&g_total_count);
    HAWKEYE_LOG_DEBUG("#### alloc %d,totalcount=%d\n",count,total_count);
    return count;
}

int libhawkeye_free_msgs(int nm)
{
    int count = 0;
    int total_count = 0;
    struct hawkeye_bug_msg *pmsg = NULL;
    do{
        pmsg = HAWKEYE_LST_GET(&g_msgmgr.lst_idlemsg,struct hawkeye_bug_msg,msg.msg_id,-1,HAWKEYE_LST_NUMCMP);
        if(!pmsg){
            HAWKEYE_LOG_ERR("msg get failed, nm=%d,count=%d",nm,count);
            break;
        }
        HAWKEYE_MSG_FREE(pmsg);
    }while(++count < nm);

    total_count = atomic_sub_return(count,&g_total_count);
    HAWKEYE_LOG_DEBUG("#### free %d,totalcount=%d\n",count,total_count);
    return count;
}

int libhawkeye_get_msg_totalcount()
{
    return atomic_read(&g_total_count);;
}

int libhawkeye_get_msg_idlecount()
{
    return HAWKEYE_LST_COUNT(&g_msgmgr.lst_idlemsg);
}
